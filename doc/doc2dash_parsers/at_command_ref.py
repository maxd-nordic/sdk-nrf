# doc2dash_parsers/at_command_ref.py
from __future__ import annotations
import re
from pathlib import Path
from typing import Any, Tuple, Iterator
from contextlib import contextmanager

from doc2dash.parsers.types import ParserEntry, EntryType
from bs4 import BeautifulSoup


class ATCommandRefParser:
    """
    Parser for AT Command Reference files.
    Compatible with doc2dash invocation that iterates `for entry in parser.parse():`.
    """

    name = "at-command-ref"

    def __init__(self, source: str | Path):
        self.source = Path(source)

    def parse(self) -> Iterator[ParserEntry]:
        index_file = self.source / "index.html"
        if not index_file.is_file():
            raise FileNotFoundError(f"Index file not found: {index_file}")

        with open(index_file, "r", encoding="utf-8") as f:
            soup = BeautifulSoup(f, "html.parser")

        body = soup.find("body")
        if not body:
            raise ValueError("No <body> found in index.html")

        for li in body.find_all("li", recursive=True):
            a = li.find("a", href=True)
            if not a:
                continue
            name = a.get_text(strip=True)
            href = a["href"]

            if (
                name.startswith("Set command")
                or name.startswith("Read command")
                or name.startswith("Test command")
            ):
                continue  # Skip these entries

            # yield as Section
            yield ParserEntry(name, EntryType.SECTION, href)

            # check if the li has a nested set command li
            for sub_li in li.find_all("li", recursive=True):
                sub_a = sub_li.find("a", href=True)
                if not sub_a:
                    continue
                sub_name = sub_a.get_text(strip=True)
                sub_href = sub_a["href"]
                if (
                    sub_name.startswith("Set command")
                    or sub_name.startswith("Read command")
                    or sub_name.startswith("Test command")
                ):
                    # detected AT command
                    command_name = name.split()[-1]
                    at_command = "AT" + command_name
                    if re.match(r"[\W]{1}[A-Z]+", command_name):
                        yield ParserEntry(at_command, EntryType.FUNCTION, href)
                    break

    @staticmethod
    def detect(path: str | Path) -> str | None:
        # check if there is an index.html file in the path
        p = Path(path)
        if not p.is_dir():
            p = p.parent
        if (p / "index.html").is_file():
            with open(p / "index.html", "r", encoding="utf-8") as f:
                content = f.read()
                if "AT Commands</title>" in content:
                    return "at-command-ref"

    @contextmanager
    def make_patcher_for_file(self, file_path):
        def patch(name, type, anchor, ref):
            # No patching needed for Doxygen, always succeed
            return True

        yield patch
