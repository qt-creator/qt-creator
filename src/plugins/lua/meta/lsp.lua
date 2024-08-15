---@meta LSP
local lsp = {}

---@module "TextEditor"

---@class ClientOptions
---@field name string The name under which to register the language server.
---@field cmd function|string[] The command to start the language server, or a function returning a string[].
---@field transport? "stdio"|"localsocket" Defaults to stdio.
---@field serverName? string The socket path when transport == "localsocket".
---@field languageFilter LanguageFilter The language filter deciding which files to open with the language server.
---@field startBehavior? "AlwaysOn"|"RequiresFile"|"RequiresProject"
---@field initializationOptions? function|table|string The initialization options to pass to the language server, either a JSON string, a table, or a function that returns either.
---@field settings? AspectContainer The settings object to associate with the language server.
---@field onStartFailed? function This callback is called when client failed to start.
---@field showInSettings? boolean Whether the client should show up in the general Language Server list.
local ClientOptions = {}

---@class LanguageFilter
---@field patterns? string[] The file patterns supported by the language server.
---@field mimeTypes? string[] The mime types supported by the language server.
local LanguageFilter = {}

---@class Client
---@field on_instance_start function The callback to call when a language client starts.
lsp.Client = {}

---@param msg string The name of the message to handle.
---@param callback function The callback to call when the message is received.
---Registers a message handler for the message named 'msg'.
function lsp.Client:registerMessage(msg, callback) end

---@param msg table the message to send.
---Sends a message to the language server.
function lsp.Client:sendMessage(msg) end

---Sends a message to the language server for a specific document.
---@param document TextDocument The document for which to send the message
---@param msg table The message to send.
function lsp.Client:sendMessageForDocument(document, msg) end

---@async
---Sends a message with an auto generated unique id to the language server for a specific document. Use a.wait(...) to wait for the response.
---@param document TextDocument The document for which to send the message
---@param msg table The message to send.
function lsp.Client:sendMessageWithIdForDocument(document, msg) end

---@param filePath FilePath to get the version of.
---@return integer Returns -1 on error, otherwise current document version.
function lsp.Client:documentVersion(filePath) end
---
---@param filePath table file path to get the uri of.
---@return string Returns empty string on error, otherwise the server URI string.
function lsp.Client:hostPathToServerUri(filePath) end

---Creates a new Language Client.
---@param options ClientOptions
---@return Client
function lsp.Client.create(options) end

return lsp
