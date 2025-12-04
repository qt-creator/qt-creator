// Copyright (C) 2025 David M. Cotter
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

namespace Mcp::Constants {

const char ACTION_ID[] = "MCPServer.Action";
const char MENU_ID[] = "MCPServer.Menu";

// About action
const char ABOUT_ACTION_ID[] = "MCPServer.About";

// MCP Command actions
const char BUILD_ACTION_ID[] = "MCPServer.Build";
const char DEBUG_ACTION_ID[] = "MCPServer.Debug";
const char STOP_DEBUG_ACTION_ID[] = "MCPServer.StopDebug";
const char LIST_PROJECTS_ACTION_ID[] = "MCPServer.ListProjects";
const char LIST_BUILD_CONFIGS_ACTION_ID[] = "MCPServer.ListBuildConfigs";
const char QUIT_ACTION_ID[] = "MCPServer.Quit";
const char GET_CURRENT_PROJECT_ACTION_ID[] = "MCPServer.GetCurrentProject";
const char GET_CURRENT_BUILD_CONFIG_ACTION_ID[] = "MCPServer.GetCurrentBuildConfig";
const char RUN_PROJECT_ACTION_ID[] = "MCPServer.RunProject";
const char CLEAN_PROJECT_ACTION_ID[] = "MCPServer.CleanProject";
const char LIST_OPEN_FILES_ACTION_ID[] = "MCPServer.ListOpenFiles";
const char LIST_SESSIONS_ACTION_ID[] = "MCPServer.ListSessions";
const char GET_CURRENT_SESSION_ACTION_ID[] = "MCPServer.GetCurrentSession";
const char SAVE_SESSION_ACTION_ID[] = "MCPServer.SaveSession";
const char LIST_ISSUES_ACTION_ID[] = "MCPServer.ListIssues";
const char GET_METHOD_METADATA_ACTION_ID[] = "MCPServer.GetMethodMetadata";
const char SET_METHOD_METADATA_ACTION_ID[] = "MCPServer.SetMethodMetadata";

// MCP Protocol Discovery actions
const char MCP_INITIALIZE_ACTION_ID[] = "MCPServer.MCPInitialize";
const char MCP_TOOLS_LIST_ACTION_ID[] = "MCPServer.MCPToolsList";

} // namespace Mcp::Constants
