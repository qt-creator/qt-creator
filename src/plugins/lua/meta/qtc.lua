---@meta

---@class PluginSpec
---@field name string The name of the plugin.
---@field pluginDirectory FilePath The directory of the plugin.
PluginSpec = {}
---The global qtc object defined in the Lua plugin.
---@class qtc
Qtc = {}

---@class (exact) QtcPlugin
---@field Id string The id of the plugin.
---@field Name string? The name of the plugin. ( Default: `Id` )
---@field Version string The version of the plugin. (`major.minor.patch`)
---@field CompatVersion string The lowest previous version of the plugin that this one is compatible to. (`major.minor.patch`)
---@field VendorId string
---@field Vendor string? The display name of the vendor of the plugin. ( Default: `VendorId` )
---@field Category string The category of the plugin.
---@field Dependencies? QtcPluginDependency[] The dependencies of the plugin.
---@field Description? string A short one line description of the plugin.
---@field LongDescription? string A long description of the plugin. Can contain newlines.
---@field Url? string The url of the plugin.
---@field DocumentationUrl? string The url of the online documentation for the plugin.
---@field License? string The license text of the plugin.
---@field Revision? string The revision of the plugin.
---@field Copyright? string The copyright of the plugin.
---@field Experimental? boolean Whether the plugin is experimental or not. ( Default: true )
---@field DisabledByDefault? boolean Whether the plugin is disabled by default or not. ( Default: true )
---@field setup function The setup function of the plugin.
---@field hooks? Hooks The hooks of the plugin.
---@field Mimetypes? string XML MIME-info for registering additional or adapting built-in MIME types.
---@field JsonWizardPaths? string[] A list of paths relative to the plugin location or paths to the Qt resource system that are searched for template-based wizards.
---@field printToOutputPane? boolean Whether the `print(...)` function should print to the output pane or not. ( Default: false )
---@field languages? string[] A list of languages that the plugin supports.
QtcPlugin = {}

---@class QtcPluginDependency
---@field Id string The name of the dependency.
---@field Version string The version of the dependency. (`major.minor.patch`)
---@field Required? "required"|"optional"|"test" Whether the dependency is required or not. (Default: "required")
QtcPluginDependency = {}


---@class EditorHooks
---@field documentOpened? function function(document)
---@field documentClosed? function function(document)
---@field text? TextEditorHooks
EditorHooks = {}

---@class TextEditorHooks
---@field currentChanged? function function(editor: TextEditor)
---@field contentsChanged? function function(document: TextDocument, position: integer, charsRemoved: integer, charsAdded: integer)
---@field cursorChanged? function function(editor: TextEditor, cursor: MultiTextCursor)

---@class ProjectHooks
---@field startupProjectChanged? function function(project: Project)
---@field projectAdded? function function(project: Project)
---@field projectRemoved? function function(project: Project)
---@field aboutToRemoveProject? function function(project: Project)
---@field runActionsUpdated? function function() Called when Project.canRunStartupProject() might have changed.

---@class Hooks
---@field editors? EditorHooks
---@field projects? ProjectHooks
Hooks = {}
