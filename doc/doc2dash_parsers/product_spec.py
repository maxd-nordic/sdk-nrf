# doc2dash_parsers/product_spec.py
from __future__ import annotations
import re
from pathlib import Path
from typing import Any, Tuple, Iterator
from contextlib import contextmanager

from doc2dash.parsers.types import ParserEntry, EntryType
from bs4 import BeautifulSoup


class ProductSpecParser:
    """
    Parser for Product Specification files.
    Compatible with doc2dash invocation that iterates `for entry in parser.parse():`.
    """

    name = "product-spec"

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

            yield ParserEntry(name, EntryType.SECTION, href)

    @staticmethod
    def detect(path: str | Path) -> str | None:
        # check if there is an index.html file in the path
        p = Path(path)
        if not p.is_dir():
            p = p.parent
        if (p / "index.html").is_file():
            with open(p / "index.html", "r", encoding="utf-8") as f:
                content = f.read()
                if (
                    "Product Specification</title>" in content
                    or "Datasheet</title>" in content
                ):
                    return "product-spec"

    @contextmanager
    def make_patcher_for_file(self, file_path):
        def patch(name, type, anchor, ref):
            # No patching needed for Doxygen, always succeed
            return True

        yield patch
