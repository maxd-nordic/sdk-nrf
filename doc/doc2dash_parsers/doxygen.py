# doc2dash_parsers/doxygen.py
from __future__ import annotations
import re
from pathlib import Path
from typing import Any, Tuple, Iterator
from contextlib import contextmanager


from doc2dash.parsers.types import ParserEntry, EntryType


class DoxygenParser:
    """
    Minimal parser for old-style Doxygen `search/*.js` (var searchData = [...]).
    Compatible with doc2dash invocation that iterates `for entry in parser.parse():`.
    """

    name = "doxygen-old"

    def __init__(self, source: str | Path):
        self.source = Path(source)

    # -----------------------
    # small JS-array parser
    # -----------------------
    def _parse_js_array(self, s: str, i: int = 0) -> Tuple[Any, int]:
        """Parse a JS-style array/object structure starting at s[i].
        Returns (python_obj, new_index). Supports single/double-quoted strings,
        numbers, null/true/false and nested arrays [].
        """

        def skip_ws(idx: int) -> int:
            while idx < len(s) and s[idx].isspace():
                idx += 1
            return idx

        def parse_string(idx: int) -> Tuple[str, int]:
            quote = s[idx]
            idx += 1
            out = []
            while idx < len(s):
                ch = s[idx]
                if ch == "\\":
                    # basic escape handling
                    idx += 1
                    if idx < len(s):
                        out.append(s[idx])
                        idx += 1
                    continue
                if ch == quote:
                    idx += 1
                    break
                out.append(ch)
                idx += 1
            return "".join(out), idx

        def parse_number(idx: int) -> Tuple[Any, int]:
            m = re.match(r"-?\d+(\.\d+)?", s[idx:])
            if not m:
                return None, idx
            token = m.group(0)
            idx += len(token)
            if "." in token:
                return float(token), idx
            return int(token), idx

        def parse_identifier(idx: int) -> Tuple[Any, int]:
            m = re.match(r"[A-Za-z_]\w*", s[idx:])
            if not m:
                return None, idx
            tok = m.group(0)
            idx += len(tok)
            if tok == "null":
                return None, idx
            if tok == "true":
                return True, idx
            if tok == "false":
                return False, idx
            return tok, idx

        def parse_value(idx: int) -> Tuple[Any, int]:
            idx = skip_ws(idx)
            if idx >= len(s):
                return None, idx
            ch = s[idx]
            if ch == "[":
                arr, idx2 = parse_array(idx)
                return arr, idx2
            if ch == "'" or ch == '"':
                return parse_string(idx)
            if ch.isdigit() or ch == "-":
                return parse_number(idx)
            return parse_identifier(idx)

        def parse_array(idx: int) -> Tuple[list, int]:
            assert s[idx] == "["
            idx += 1
            arr = []
            while True:
                idx = skip_ws(idx)
                if idx >= len(s):
                    break
                if s[idx] == "]":
                    idx += 1
                    break
                val, idx = parse_value(idx)
                arr.append(val)
                idx = skip_ws(idx)
                # skip comma if present
                if idx < len(s) and s[idx] == ",":
                    idx += 1
                    continue
                # if next is closing bracket we'll finish in top loop
            return arr, idx

        # entry point: expect an array start
        idx = skip_ws(i)
        if idx < len(s) and s[idx] == "[":
            return parse_array(idx)
        # not an array
        return None, i

    # -----------------------
    # utilities
    # -----------------------
    def _extract_searchdata_array(self, text: str) -> list | None:
        """Find 'var searchData =' and parse the following JS array into Python lists."""
        m = re.search(r"\bvar\s+searchData\s*=\s*", text)
        if not m:
            return None
        start = m.end()
        # find first '[' after the assignment
        pos = text.find("[", start)
        if pos == -1:
            return None
        parsed, end = self._parse_js_array(text, pos)
        return parsed

    def _normalize_url(self, url: str) -> str:
        if not isinstance(url, str):
            return ""
        return url.lstrip("./")

    def _guess_type(self, fname: str, url: str, label: str = None) -> EntryType:
        base = Path(fname).name
        label = label or ""
        # Debug print for classification
        print(f"Classifying: file={base}, label={label}")
        # Macro: ALL_CAPS and underscores, or found in defines/group__
        if (base.startswith("group__") or base.startswith("defines")) and re.match(
            r"^[A-Z][A-Z0-9_]+$", label
        ):
            print("  -> Macro")
            return EntryType.MACRO
        # Function: starts with nrf_ and is not ALL_CAPS
        if re.match(r"^nrf_[a-z0-9_]+$", label):
            print("  -> Function")
            return EntryType.FUNCTION
        # Type: ends with _t, or found in typedefs/classes/enums
        if (
            label.endswith("_t")
            or label.endswith("_Type")
            or base.startswith("typedefs")
            or base.startswith("classes")
            or base.startswith("enums")
        ):
            print("  -> Type")
            return EntryType.TYPE
        # Attribute: contains a dot (struct member), or found in variables/enumvalues
        if (
            "." in label
            or base.startswith("variables")
            or base.startswith("enumvalues")
        ):
            print("  -> Attribute")
            return EntryType.ATTRIBUTE
        # Section: namespaces or group files with non-macro label
        if base.startswith("namespaces") or (
            base.startswith("group__") and not re.match(r"^[A-Z][A-Z0-9_]+$", label)
        ):
            print("  -> Section")
            return EntryType.SECTION
        # Guide: files or anything else
        print("  -> Guide (fallback)")
        return EntryType.GUIDE

    # -----------------------
    # public API expected by doc2dash
    # -----------------------
    @staticmethod
    def detect(path: str | Path) -> str | None:
        """Optional: detect if this converter applies to the given html tree.
        Return a name (string) if this looks like a Doxygen html tree; otherwise None.
        """
        p = Path(path)
        search_dir = p / "search"
        if not search_dir.exists():
            return None
        # quick check: any .js file containing var searchData
        for js in search_dir.glob("*.js"):
            txt = js.read_text(encoding="utf-8", errors="ignore")
            if "var searchData" in txt:
                # try to pick a doc title from index.html
                idx = p / "index.html"
                if idx.exists():
                    t = idx.read_text(encoding="utf-8", errors="ignore")
                    m = re.search(r"<title>([^<]+)</title>", t, re.I)
                    if m:
                        return m.group(1).strip()
                return "Doxygen"
        return None

    def parse(self) -> Iterator[Any]:
        """Yield ParserEntry (or tuple) items: name, type, path."""
        search_dir = self.source / "search"
        if not search_dir.exists() or not search_dir.is_dir():
            return

        for js in sorted(search_dir.glob("*.js")):
            print(f"Parsing: {js}")
            try:
                text = js.read_text(encoding="utf-8", errors="ignore")
            except Exception:
                print(f"Failed to read {js}")
                continue

            arr = self._extract_searchdata_array(text)
            if not arr:
                continue

            for top in arr:
                # Handle both old and new Doxygen formats
                if not isinstance(top, list) or len(top) < 2:
                    continue
                # If second element is a list and first is a string, treat as single entry
                if len(top) == 2 and isinstance(top[1], list):
                    sub = top[1]
                    if len(sub) < 2:
                        continue
                    label = sub[0]
                    url = sub[1]
                    # If url is a list, use its first element
                    if isinstance(url, list):
                        if url:
                            url = url[0]
                        else:
                            continue
                    if not isinstance(label, str) or not isinstance(url, str):
                        continue
                    url = self._normalize_url(url)
                    kind = self._guess_type(js.name, url, label)
                    print(f"Yielding: label={label}, kind={kind}, url={url}")
                    yield ParserEntry(label, kind, url)

                else:
                    # Fallback: iterate sublists as before
                    for sub in top[1:]:
                        if not isinstance(sub, list) or len(sub) < 2:
                            continue
                        label = sub[0]
                        url = sub[1]
                        if not isinstance(label, str) or not isinstance(url, str):
                            continue
                        url = self._normalize_url(url)
                        kind = self._guess_type(js.name, url, label)
                        print(f"Yielding: label={label}, kind={kind}, url={url}")
                        yield ParserEntry(label, kind, url)

    @contextmanager
    def make_patcher_for_file(self, file_path):
        def patch(name, type, anchor, ref):
            # No patching needed for Doxygen, always succeed
            return True

        yield patch
