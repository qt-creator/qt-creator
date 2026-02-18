# ACP Library — Type Overview

## File Structure

```
acpobject.h     Base class + JSON conversion helpers
acputils.h      fromJsonValue<T> template
acpkeys.h       constexpr Key constants (90 keys)
acptypes.h      Enums, aliases, shared object types
acpcontent.h    Content model types
acpmessages.h   Request/response/notification types
```

## Inheritance Hierarchy

All object types inherit from `AcpObject` (a thin wrapper around `QJsonObject`).

```
AcpObject
 |
 |-- acptypes.h ─────────────────────────────────────────────────────
 |   |
 |   |-- SessionCapabilities         Agent session capabilities
 |   |-- PromptCapabilities          Audio, image, embedded context support
 |   |-- McpCapabilities             HTTP/SSE MCP support flags
 |   |-- AgentCapabilities           Aggregates Session/Prompt/McpCapabilities
 |   |
 |   |-- Implementation              Client or agent name + version
 |   |-- AuthMethod                  Authentication method (id, name)
 |   |-- Error                       JSON-RPC error (code, message, data)
 |   |
 |   |-- SessionMode                 A mode the agent can operate in
 |   |-- SessionModeState            Available modes + current mode id
 |   |-- CurrentModeUpdate           Mode change notification payload
 |   |
 |   |-- SessionConfigSelectOption   A selectable config value
 |   |-- SessionConfigSelectGroup    A group of config options
 |   |-- SessionConfigSelect         Dropdown config (current value + options)
 |   |-- SessionConfigOption         Config option (discriminated: select)
 |   |-- ConfigOptionUpdate          Batch config option update
 |   |
 |   |-- PlanEntry                   Single plan step (content, priority, status)
 |   |-- Plan                        Execution plan (list of PlanEntry)
 |   |
 |   |-- UnstructuredCommandInput    Command input hint
 |   |-- AvailableCommand            Command (name, description, input)
 |   |-- AvailableCommandsUpdate     Available commands notification payload
 |   |
 |   |-- EnvVariable                 Environment variable (name, value)
 |   |-- HttpHeader                  HTTP header (name, value)
 |   |-- McpServerHttp               MCP server via HTTP
 |   |-- McpServerStdio              MCP server via stdio (command + args + env)
 |   |-- McpServerSse                MCP server via SSE
 |   |
 |   |-- PermissionOption            Permission choice (id, name, kind)
 |   |
 |   |-- ClientCapabilities          Client FS + terminal capabilities
 |   |-- FileSystemCapability        Read/write text file support
 |   |-- TerminalExitStatus          Exit code + signal
 |   |
 |-- acpcontent.h ───────────────────────────────────────────────────
 |   |
 |   |-- Annotations                 Audience, priority, last modified
 |   |-- TextContent                 Text block
 |   |-- ImageContent                Base64 image (data + mimeType)
 |   |-- AudioContent                Base64 audio (data + mimeType)
 |   |-- ResourceLink                URI-based resource reference
 |   |-- TextResourceContents        Text resource (uri + text)
 |   |-- BlobResourceContents        Binary resource (uri + blob)
 |   |-- EmbeddedResource            Wraps EmbeddedResourceResource variant
 |   |
 |   |-- ContentBlock                Discriminated union (text|image|audio|resource_link|resource)
 |   |-- ContentChunk                Streamed content wrapper
 |   |-- Content                     Standard content wrapper
 |   |
 |   |-- ToolCallLocation            File path + optional line number
 |   |-- Diff                        File diff (path, oldText, newText)
 |   |-- Terminal                    Terminal reference (terminalId)
 |   |-- ToolCallContent             Discriminated union (content|diff|terminal)
 |   |-- ToolCallUpdate              Partial update for a tool call
 |   |-- ToolCall                    Full tool call (id, title, kind, status, content)
 |   |
 |-- acpmessages.h ──────────────────────────────────────────────────
 |   |
 |   |-- SessionUpdate               Discriminated union (9 update types, see below)
 |   |-- SessionNotification         Session update notification (sessionId + update)
 |   |
 |   |-- InitializeRequest           protocolVersion, clientInfo, clientCapabilities
 |   |-- InitializeResponse          protocolVersion, agentInfo, agentCapabilities, authMethods
 |   |-- AuthenticateRequest         methodId
 |   |-- AuthenticateResponse        (empty)
 |   |
 |   |-- NewSessionRequest           cwd, mcpServers
 |   |-- NewSessionResponse          sessionId, modes, configOptions
 |   |-- LoadSessionRequest          sessionId, cwd, mcpServers
 |   |-- LoadSessionResponse         modes, configOptions
 |   |
 |   |-- SetSessionModeRequest       sessionId, modeId
 |   |-- SetSessionModeResponse      (empty)
 |   |-- SetSessionConfigOptionRequest  sessionId, configId, value
 |   |-- SetSessionConfigOptionResponse configOptions
 |   |
 |   |-- PromptRequest               sessionId, prompt (list of ContentBlock)
 |   |-- PromptResponse              stopReason
 |   |-- CancelNotification          sessionId
 |   |
 |   |-- CreateTerminalRequest       sessionId, command, args, cwd, env
 |   |-- CreateTerminalResponse      terminalId
 |   |-- TerminalOutputRequest       sessionId, terminalId
 |   |-- TerminalOutputResponse      output, truncated, exitStatus
 |   |-- WaitForTerminalExitRequest  sessionId, terminalId
 |   |-- WaitForTerminalExitResponse exitCode, signal
 |   |-- KillTerminalCommandRequest  sessionId, terminalId
 |   |-- KillTerminalCommandResponse (empty)
 |   |-- ReleaseTerminalRequest      sessionId, terminalId
 |   |-- ReleaseTerminalResponse     (empty)
 |   |
 |   |-- ReadTextFileRequest         sessionId, path, line, limit
 |   |-- ReadTextFileResponse        content
 |   |-- WriteTextFileRequest        sessionId, path, content
 |   |-- WriteTextFileResponse       (empty)
 |   |
 |   |-- RequestPermissionRequest    sessionId, toolCall, options
 |   |-- RequestPermissionResponse   outcome
 |   |-- RequestPermissionOutcome    Discriminated union (cancelled|selected)
 |   |-- SelectedPermissionOutcome   optionId
```

## Enumerations

```
Role                 { Assistant, User }
ToolKind             { Read, Edit, Delete, Move, Search, Execute, Think, Fetch, SwitchMode, Other }
ToolCallStatus       { Pending, InProgress, Completed, Failed }
PlanEntryStatus      { Pending, InProgress, Completed }
PlanEntryPriority    { High, Medium, Low }
PermissionOptionKind { AllowOnce, AllowAlways, RejectOnce, RejectAlways }
StopReason           { EndTurn, MaxTokens, MaxTurnRequests, Refusal, Cancelled }
ErrorCode : int      { ParseError=-32700, InvalidRequest=-32600, MethodNotFound=-32601,
                       InvalidParams=-32602, InternalError=-32603,
                       AuthenticationRequired=-32000, ResourceNotFound=-32002 }
```

## String Aliases

```
SessionId, SessionModeId, SessionConfigId, SessionConfigValueId,
SessionConfigGroupId, ToolCallId, PermissionOptionId  →  QString
ProtocolVersion                                        →  int
```

## Variant Types

```
RequestId                    = std::variant<nullptr_t, qint64, QString>     (special class)
McpServer                    = std::variant<McpServerHttp, McpServerSse, McpServerStdio>
EmbeddedResourceResource     = std::variant<TextResourceContents, BlobResourceContents>
SessionConfigSelectOptions   = std::variant<QList<SessionConfigSelectOption>,
                                            QList<SessionConfigSelectGroup>>
AvailableCommandInput        = UnstructuredCommandInput   (single-type alias)
SessionConfigOptionCategory  = QString                    (deduplicated)
```

## Discriminated Unions

These types wrap a `QJsonObject` and dispatch via a discriminator property:

```
ContentBlock            discriminator: "type"
  ├─ "text"             → TextContent
  ├─ "image"            → ImageContent
  ├─ "audio"            → AudioContent
  ├─ "resource_link"    → ResourceLink
  └─ "resource"         → EmbeddedResource

ToolCallContent         discriminator: "type"
  ├─ "content"          → Content
  ├─ "diff"             → Diff
  └─ "terminal"         → Terminal

SessionUpdate           discriminator: "sessionUpdate"
  ├─ "user_message_chunk"         → ContentChunk
  ├─ "agent_message_chunk"        → ContentChunk
  ├─ "agent_thought_chunk"        → ContentChunk
  ├─ "tool_call"                  → ToolCall
  ├─ "tool_call_update"           → ToolCallUpdate
  ├─ "plan"                       → Plan
  ├─ "available_commands_update"  → AvailableCommandsUpdate
  ├─ "current_mode_update"        → CurrentModeUpdate
  └─ "config_option_update"       → ConfigOptionUpdate

SessionConfigOption     discriminator: "type"
  └─ "select"           → SessionConfigSelect

RequestPermissionOutcome discriminator: "outcome"
  └─ "selected"          → SelectedPermissionOutcome
```

## Protocol Methods (from meta.json)

### Agent-side (client → agent)

| Method                        | Request                       | Response                        |
|-------------------------------|-------------------------------|---------------------------------|
| `initialize`                  | InitializeRequest             | InitializeResponse              |
| `authenticate`                | AuthenticateRequest           | AuthenticateResponse            |
| `session/new`                 | NewSessionRequest             | NewSessionResponse              |
| `session/load`                | LoadSessionRequest            | LoadSessionResponse             |
| `session/prompt`              | PromptRequest                 | PromptResponse                  |
| `session/cancel`              | CancelNotification            | *(notification, no response)*   |
| `session/set_mode`            | SetSessionModeRequest         | SetSessionModeResponse          |
| `session/set_config_option`   | SetSessionConfigOptionRequest | SetSessionConfigOptionResponse  |

### Client-side (agent → client)

| Method                     | Request                     | Response                      |
|----------------------------|-----------------------------|-------------------------------|
| `session/notification`     | SessionNotification         | *(notification, no response)* |
| `terminal/create`          | CreateTerminalRequest       | CreateTerminalResponse        |
| `terminal/output`          | TerminalOutputRequest       | TerminalOutputResponse        |
| `terminal/wait_for_exit`   | WaitForTerminalExitRequest  | WaitForTerminalExitResponse   |
| `terminal/kill_command`    | KillTerminalCommandRequest  | KillTerminalCommandResponse   |
| `terminal/release`         | ReleaseTerminalRequest      | ReleaseTerminalResponse       |
| `fs/read_text_file`        | ReadTextFileRequest         | ReadTextFileResponse          |
| `fs/write_text_file`       | WriteTextFileRequest        | WriteTextFileResponse         |
| `permissions/request`      | RequestPermissionRequest    | RequestPermissionResponse     |
