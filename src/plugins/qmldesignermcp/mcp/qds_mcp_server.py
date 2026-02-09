# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

from mcp.server import Server
from mcp.server.stdio import stdio_server
from mcp import types
from pathlib import Path
import asyncio
import json

# Create server instance
server = Server("qds-server")

# Project path will be set from command line
PROJECT_PATH = None

@server.list_resources()
async def list_resources() -> list[types.Resource]:
    """List available resources"""
    resources = [
        types.Resource(
            uri="qmlproject://structure",
            name="Project Structure",
            description="File tree of the QML project",
            mimeType="application/json"
        ),
    ]

    return resources

@server.read_resource()
async def read_resource(uri: types.AnyUrl) -> str:
    """Read a specific resource"""
    try:
        # Convert AnyUrl to string
        uri_str = str(uri)

        if uri_str == "qmlproject://structure":
            tree = get_file_tree()
            return json.dumps(tree, indent=2)
        elif uri_str.startswith("qmlproject://file/"):
            filepath = uri_str.replace("qmlproject://file/", "")
            full_path = PROJECT_PATH / filepath

            # Security check
            full_path.resolve().relative_to(PROJECT_PATH.resolve())

            if not full_path.exists():
                raise FileNotFoundError(f"File not found: {filepath}")

            with open(full_path, 'r', encoding='utf-8') as f:
                return f.read()
        else:
            raise ValueError(f"Unknown resource: {uri_str}")
    except Exception as e:
        raise ValueError(f"Error reading resource {uri}: {str(e)}")

@server.list_tools()
async def list_tools() -> list[types.Tool]:
    """List available tools"""
    return [
        types.Tool(
            name="search_files",
            description="Search for QML files matching a query",
            inputSchema={
                "type": "object",
                "properties": {
                    "query": {
                        "type": "string",
                        "description": "Search query for file names"
                    }
                },
                "required": ["query"]
            }
        ),
        types.Tool(
            name="read_qml",
            description="Read content of a QML file",
            inputSchema={
                "type": "object",
                "properties": {
                    "filepath": {
                        "type": "string",
                        "description": "Path to the QML file relative to project root"
                    }
                },
                "required": ["filepath"]
            }
        ),
        types.Tool(
            name="modify_qml",
            description="Modify a QML file",
            inputSchema={
                "type": "object",
                "properties": {
                    "filepath": {
                        "type": "string",
                        "description": "Path to the QML file"
                    },
                    "content": {
                        "type": "string",
                        "description": "New content for the file"
                    }
                },
                "required": ["filepath", "content"]
            }
        ),
        types.Tool(
            name="create_qml",
            description="Create a new QML file",
            inputSchema={
                "type": "object",
                "properties": {
                    "filepath": {
                        "type": "string",
                        "description": "Path for the new QML file"
                    },
                    "content": {
                        "type": "string",
                        "description": "Content for the new file"
                    }
                },
                "required": ["filepath", "content"]
            }
        ),
    ]

@server.call_tool()
async def call_tool(name: str, arguments: dict) -> list[types.TextContent]:
    """Handle tool calls"""

    try:
        if name == "search_files":
            query = arguments["query"]
            results = []
            for file in PROJECT_PATH.rglob("*.qml"):
                if query.lower() in file.name.lower():
                    results.append(str(file.relative_to(PROJECT_PATH)))
            return [types.TextContent(
                type="text",
                text=json.dumps(results, indent=2)
            )]

        elif name == "read_qml":
            filepath = arguments["filepath"]
            full_path = PROJECT_PATH / filepath

            # Security check
            full_path.resolve().relative_to(PROJECT_PATH.resolve())

            if not full_path.exists():
                raise FileNotFoundError(f"File not found: {filepath}")

            with open(full_path, 'r', encoding='utf-8') as f:
                content = f.read()
            return [types.TextContent(
                type="text",
                text=content
            )]

        elif name == "modify_qml":
            filepath = arguments["filepath"]
            content = arguments["content"]
            full_path = PROJECT_PATH / filepath

            # Security check
            full_path.resolve().relative_to(PROJECT_PATH.resolve())

            # Create parent directories if needed
            full_path.parent.mkdir(parents=True, exist_ok=True)

            with open(full_path, 'w', encoding='utf-8') as f:
                f.write(content)
            return [types.TextContent(
                type="text",
                text=f"Successfully modified {filepath}"
            )]

        elif name == "create_qml":
            filepath = arguments["filepath"]
            content = arguments["content"]
            full_path = PROJECT_PATH / filepath

            # Security check
            full_path.resolve().relative_to(PROJECT_PATH.resolve())

            if full_path.exists():
                raise FileExistsError(f"File already exists: {filepath}")

            # Create parent directories if needed
            full_path.parent.mkdir(parents=True, exist_ok=True)

            with open(full_path, 'w', encoding='utf-8') as f:
                f.write(content)
            return [types.TextContent(
                type="text",
                text=f"Successfully created {filepath}"
            )]

        else:
            raise ValueError(f"Unknown tool: {name}")

    except Exception as e:
        return [types.TextContent(
            type="text",
            text=f"Error: {str(e)}"
        )]

def get_file_tree() -> dict:
    """Get lightweight file tree structure"""
    tree = {"files": {}, "total_files": 0}

    if not PROJECT_PATH or not PROJECT_PATH.exists():
        return tree

    for file in PROJECT_PATH.rglob("*.qml"):
        rel_path = file.relative_to(PROJECT_PATH)
        tree["files"][str(rel_path)] = {
            "size": file.stat().st_size,
            "modified": file.stat().st_mtime
        }
        tree["total_files"] += 1

    return tree

async def main():
    global PROJECT_PATH

    # Get project path from command line
    import sys
    if len(sys.argv) > 1:
        PROJECT_PATH = Path(sys.argv[1]).resolve()
    else:
        PROJECT_PATH = Path.cwd()

    if not PROJECT_PATH.exists():
        import sys
        print(f"Error: Project path does not exist: {PROJECT_PATH}", file=sys.stderr, flush=True)
        return

    # Don't print to stdout - it interferes with JSON-RPC
    import sys
    print(f"Starting QDS MCP Server for project: {PROJECT_PATH}", file=sys.stderr, flush=True)

    # Run the server
    async with stdio_server() as (read_stream, write_stream):
        await server.run(
            read_stream,
            write_stream,
            server.create_initialization_options()
        )

if __name__ == "__main__":
    asyncio.run(main())
