# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

from mcp.server import Server
from mcp.server.stdio import stdio_server
from mcp import types
from pathlib import Path
import asyncio
import sys

# Create server instance
server = Server("qml_server")

# Project path will be set from command line
PROJECT_PATH: Path | None = None


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def _resolve_path(filepath: str) -> Path:
    """
    Resolve *filepath* relative to PROJECT_PATH and raise ValueError if the
    result escapes the project root (path-traversal guard).
    """
    full_path = (PROJECT_PATH / filepath).resolve()
    project_root = PROJECT_PATH.resolve()
    try:
        full_path.relative_to(project_root)
    except ValueError:
        raise ValueError(f"Access denied: '{filepath}' is outside the project root.")
    return full_path


def _build_tree(root: Path) -> str:
    """
    Return an indented text tree of all .qml files, e.g.:

        MyProject/
          Main.qml
          components/
            Button.qml
            Card.qml
          screens/
            HomeScreen.qml
    """
    lines: list[str] = []

    def _recurse(path: Path, indent: int) -> None:
        prefix = "  " * indent
        if path.is_dir():
            # Collect only dirs and .qml files, skip everything else
            children = sorted(
                c for c in path.iterdir()
                if c.is_dir() or c.suffix == ".qml"
            )
            if not children:
                return
            lines.append(f"{prefix}{path.name}/")
            for child in children:
                _recurse(child, indent + 1)
        elif path.suffix == ".qml":
            lines.append(f"{prefix}{path.name}")

    _recurse(root, 0)
    return "\n".join(lines)


# ---------------------------------------------------------------------------
# Resources
# ---------------------------------------------------------------------------

@server.list_resources()
async def list_resources() -> list[types.Resource]:
    return [
        types.Resource(
            uri="qmlproject://structure",
            name="Project Structure",
            description=(
                "Indented text tree of all QML files in the project. "
                "Use this to understand the project layout before reading or editing files."
            ),
            mimeType="text/plain",
        ),
    ]


@server.read_resource()
async def read_resource(uri: types.AnyUrl) -> str:
    uri_str = str(uri)

    if uri_str == "qmlproject://structure":
        return _build_tree(PROJECT_PATH)

    raise ValueError(f"Unknown resource URI: {uri_str}")


# ---------------------------------------------------------------------------
# Tools
# ---------------------------------------------------------------------------

@server.list_tools()
async def list_tools() -> list[types.Tool]:
    return [
        types.Tool(
            name="read_qml",
            description=(
                "Read the full content of a QML file. "
                "Use the project structure resource first to find the correct path."
            ),
            inputSchema={
                "type": "object",
                "properties": {
                    "filepath": {
                        "type": "string",
                        "description": "Path to the QML file relative to the project root (e.g. 'components/Button.qml').",
                    }
                },
                "required": ["filepath"],
            },
        ),
        types.Tool(
            name="create_qml",
            description=(
                "Create a new QML file at the given path. "
                "Fails if the file already exists — use modify_qml to overwrite."
            ),
            inputSchema={
                "type": "object",
                "properties": {
                    "filepath": {
                        "type": "string",
                        "description": "Destination path relative to the project root (e.g. 'components/Card.qml').",
                    },
                    "content": {
                        "type": "string",
                        "description": "Full QML source to write into the new file.",
                    },
                },
                "required": ["filepath", "content"],
            },
        ),
        types.Tool(
            name="modify_qml",
            description=(
                "Overwrite the entire content of an existing QML file. "
                "Always read the file first with read_qml so you can make targeted edits."
            ),
            inputSchema={
                "type": "object",
                "properties": {
                    "filepath": {
                        "type": "string",
                        "description": "Path to the QML file relative to the project root.",
                    },
                    "content": {
                        "type": "string",
                        "description": "New full content for the file.",
                    },
                },
                "required": ["filepath", "content"],
            },
        ),
        types.Tool(
            name="delete_qml",
            description=(
                "Permanently delete a QML file from the project. "
                "This cannot be undone — confirm the path with read_qml before deleting."
            ),
            inputSchema={
                "type": "object",
                "properties": {
                    "filepath": {
                        "type": "string",
                        "description": "Path to the QML file to delete, relative to the project root.",
                    }
                },
                "required": ["filepath"],
            },
        ),
        types.Tool(
            name="move_qml",
            description=(
                "Move or rename a QML file within the project. "
                "Useful for refactoring component locations or fixing naming conventions."
            ),
            inputSchema={
                "type": "object",
                "properties": {
                    "source": {
                        "type": "string",
                        "description": "Current path of the file relative to the project root.",
                    },
                    "destination": {
                        "type": "string",
                        "description": "New path for the file relative to the project root.",
                    },
                },
                "required": ["source", "destination"],
            },
        ),
    ]


@server.call_tool()
async def call_tool(name: str, arguments: dict) -> list[types.TextContent]:

    def ok(msg: str) -> list[types.TextContent]:
        return [types.TextContent(type="text", text=msg)]

    def err(msg: str) -> list[types.TextContent]:
        return [types.TextContent(type="text", text=f"Error: {msg}")]

    try:
        if name == "read_qml":
            full_path = _resolve_path(arguments["filepath"])
            if not full_path.exists():
                return err(f"File not found: {arguments['filepath']}")
            return ok(full_path.read_text(encoding="utf-8"))

        elif name == "create_qml":
            filepath = arguments["filepath"]
            full_path = _resolve_path(filepath)
            if full_path.exists():
                return err(
                    f"File already exists: {filepath}. Use modify_qml to overwrite it."
                )
            full_path.parent.mkdir(parents=True, exist_ok=True)
            full_path.write_text(arguments["content"], encoding="utf-8")
            return ok(f"Created {filepath}")

        elif name == "modify_qml":
            filepath = arguments["filepath"]
            full_path = _resolve_path(filepath)
            if not full_path.exists():
                return err(
                    f"File not found: {filepath}. Use create_qml to make a new file."
                )
            full_path.write_text(arguments["content"], encoding="utf-8")
            return ok(f"Modified {filepath}")

        elif name == "delete_qml":
            filepath = arguments["filepath"]
            full_path = _resolve_path(filepath)
            if not full_path.exists():
                return err(f"File not found: {filepath}")
            full_path.unlink()
            # Remove empty parent directories up to (but not including) project root
            for parent in full_path.parents:
                if parent == PROJECT_PATH.resolve():
                    break
                if parent.is_dir() and not any(parent.iterdir()):
                    parent.rmdir()
                else:
                    break
            return ok(f"Deleted {filepath}")

        elif name == "move_qml":
            src_path = _resolve_path(arguments["source"])
            dst_path = _resolve_path(arguments["destination"])
            if not src_path.exists():
                return err(f"Source file not found: {arguments['source']}")
            if dst_path.exists():
                return err(
                    f"Destination already exists: {arguments['destination']}. "
                    "Delete or choose a different path."
                )
            dst_path.parent.mkdir(parents=True, exist_ok=True)
            src_path.rename(dst_path)
            return ok(f"Moved {arguments['source']} → {arguments['destination']}")

        else:
            return err(f"Unknown tool: {name}")

    except Exception as exc:
        return err(str(exc))


# ---------------------------------------------------------------------------
# Entry point
# ---------------------------------------------------------------------------

async def main():
    global PROJECT_PATH

    if len(sys.argv) > 1:
        PROJECT_PATH = Path(sys.argv[1]).resolve()
    else:
        PROJECT_PATH = Path.cwd()

    if not PROJECT_PATH.exists():
        print(f"Error: Project path does not exist: {PROJECT_PATH}", file=sys.stderr, flush=True)
        return

    print(f"Starting QDS MCP Server for project: {PROJECT_PATH}", file=sys.stderr, flush=True)

    async with stdio_server() as (read_stream, write_stream):
        await server.run(
            read_stream,
            write_stream,
            server.create_initialization_options(),
        )


if __name__ == "__main__":
    asyncio.run(main())
