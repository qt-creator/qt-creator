---@meta LSP

local lsp = {}

---@class ClientOptions
---@field name string The name under which to register the language server.
---@field cmd function|string[] The command to start the language server, or a function returning a string[].
---@field transport? "stdio"|"localsocket" Defaults to stdio
---@field serverName? string The socket path when transport == "localsocket"
---@field languageFilter LanguageFilter The language filter deciding which files to open with the language server
---@field startBehavior? "AlwaysOn"|"RequiresFile"|"RequiresProject"
---@field initializationOptions? table|string The initialization options to pass to the language server, either a json string, or a table
---@field settings? AspectContainer
local ClientOptions = {}

---@class LanguageFilter
---@field patterns? string[] The file patterns supported by the language server
---@field mimeTypes? string[] The mime types supported by the language server
local LanguageFilter = {}

---@class Client
---@field on_instance_start function The callback to call when a language client starts
lsp.Client = {}

---@param msg string The name of the message to handle
---@param callback function The callback to call when the message is received
---Registers a message handler for the message named 'msg'
function lsp.Client:registerMessage(msg, callback) end

---Creates a new Language Client
---@param options ClientOptions
---@return Client
function lsp.Client.create(options) end

return lsp
