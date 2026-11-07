"""Expose the TJG guide in the combined site from its canonical source folder."""

import os
from pathlib import Path

from mkdocs.structure.files import File

GUIDE_DIR = Path(__file__).resolve().parents[2] / "tjg" / "doc"


def on_files(files, config):
    for source in sorted(GUIDE_DIR.rglob("*")):
        if not source.is_file():
            continue
        route = "color/" + source.relative_to(GUIDE_DIR).as_posix()
        page = File.generated(config, route, abs_src_path=str(source))
        # MkDocs combines this with edit_uri, which is relative to docs_dir.
        page.edit_uri = Path(os.path.relpath(source, config.docs_dir)).as_posix()
        files.append(page)
    return files


def on_page_markdown(markdown, page, **kwargs):
    # Keep cross-folder Markdown links valid in the repository and in the site.
    if page.file.src_uri.startswith("color/"):
        return markdown.replace("](../../doc/", "](../")
    return markdown.replace("](../tjg/doc/", "](color/")


def on_serve(server, **kwargs):
    server.watch(str(GUIDE_DIR))
    return server
