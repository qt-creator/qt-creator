---@meta Settings

---@module 'Qt'
---@module 'SimpleTypes'

local settings = {}

---The base class of all aspects
---@class BaseAspect
settings.BaseAspect = {}

---Applies the changes from its volatileValue to its value.
function settings.BaseAspect:apply() end

---@class AspectCreate
---@field settingsKey? string The settings key of the aspect. If not set, the aspect will not be saved to the settings persistently.
---@field displayName? string The display name of the aspect.
---@field labelText? string The label text of the aspect.
---@field toolTip? string The tool tip of the aspect.
---@field enabler? BoolAspect Enable / Disable this aspect based on the state of the `enabler`.
---@field onValueChanged? function () Called when the value of the aspect changes.
---@field onVolatileValueChanged? function () Called when the volatile value of the aspect changes.
---@field macroExpander? MacroExpander|NullType The macro expander to use, or nil to disable macro expansion.
local AspectCreate = {}

---The base class of most typed aspects.
---@generic T
---@class TypedAspect<T> : BaseAspect
---@field value `T` The value of the aspect.
---@field volatileValue `T` The temporary value of the aspect.
---@field defaultValue `T` The default value of the aspect.
local TypedAspect = {}

---@generic T
---@class TypedAspectCreate<T> : AspectCreate
---@field defaultValue `T` The default value of the aspect.
local TypedAspectCreate = {}

---A container for aspects.
---@class AspectContainer : BaseAspect
settings.AspectContainer = {}

---Options for creating an AspectContainer.
---@class AspectContainerCreate
---@field autoApply? boolean Whether the aspects should be applied automatically or not.
---@field onApplied? function Called when the aspects are applied.
---@field layouter? function The layouter of the aspect container.
---@field settingsGroup? string The settings group of the aspect container.
AspectContainerCreate = {}

---Create a new AspectContainer.
---@param options AspectContainerCreate
---@return AspectContainer
function settings.AspectContainer.create(options) end

---A aspect containing a boolean value.
---@class BoolAspect : TypedAspect<boolean>
settings.BoolAspect = {}

---@enum LabelPlacement
settings.LabelPlacement = {
    AtCheckBox = 0,
    Compact = 0,
    InExtraLabel = 0
};
---@class BoolAspectCreate : TypedAspectCreate<boolean>
---@field labelPlacement? LabelPlacement:
BoolAspectCreate = {}

---Create a new BoolAspect.
---@param options BoolAspectCreate
---@return BoolAspect
function settings.BoolAspect.create(options) end

settings.ColorAspect = {}
function settings.ColorAspect.create(options) end

---@class SelectionAspect : TypedAspect<int>
---@field stringValue string The string value of the aspect.
---@field dataValue any The data value of the aspect.
settings.SelectionAspect = {}

---@enum SelectionDisplayStyle
settings.SelectionDisplayStyle = {
    RadioButtons = 0,
    ComboBox = 0
};

---@class SelectionOption
---@field name string The name of the option.
---@field tooltip? string The tooltip of the option.
---@field data? any The data of the option.
SelectionOption = {}

---@class SelectionAspectCreate : TypedAspectCreate<int>
---@field displayStyle? SelectionDisplayStyle The display type of the aspect.
---@field options? string[]|SelectionOption[] The available options.
SelectionAspectCreate = {}

---Creates a new SelectionAspect
---@param options SelectionAspectCreate
---@return SelectionAspect aspect The Aspect
function settings.SelectionAspect.create(options) end

function settings.SelectionAspect:addOption(option) end

function settings.SelectionAspect:addOption(option, tooltip) end
settings.MultiSelectionAspect = {}
function settings.MultiSelectionAspect.create(options) end

---@enum StringDisplayStyle
settings.StringDisplayStyle = {
    Label = 0,
    LineEdit = 0,
    TextEdit = 0,
    PasswordLineEdit = 0,
};

---@class StringAspectCreate : TypedAspectCreate<string>
---@field displayStyle? StringDisplayStyle The display type of the aspect.
---@field historyId? string The history id of the aspect.
---@field valueAcceptor? function string (oldvalue: string, newValue: string)
---@field showToolTipOnLabel? boolean
---@field displayFilter? function string (value: string)
---@field placeHolderText? string
---@field acceptRichText? boolean
---@field autoApplyOnEditingFinished? boolean
---@field elideMode? Qt.TextElideMode The elide mode of the aspect.
StringAspectCreate = {}

---@class StringAspect : TypedAspect<string>
settings.StringAspect = {}

---Create a new StringAspect
---@param options StringAspectCreate
function settings.StringAspect.create(options) end

---@enum Kind
settings.Kind = {
    ExistingDirectory = 0,
    Directory = 0,
    File = 0,
    SaveFile = 0,
    ExistingCommand = 0,
    Command = 0,
    Any = 0
};

---@class FilePathAspectCreate
---@field expectedKind? Kind The kind of path we want to select.
---@field historyId? string The history id of the aspect.
---@field defaultPath? FilePath The default path of the aspect.
---@field promptDialogFilter? string
---@field promptDialogTitle? string
---@field commandVersionArguments? string[]
---@field allowPathFromDevice? boolean
---@field validatePlaceHolder? boolean
---@field openTerminalHandler? function
---@field environment? Environment
---@field baseFileName? FilePath
---@field valueAcceptor? function string (oldvalue: string, newValue: string)
---@field showToolTipOnLabel?  boolean
---@field autoApplyOnEditingFinished? boolean
---@field validationFunction? function
---@field displayFilter? function string (value: string)
---@field placeHolderText? string
FilePathAspectCreate = {}

---@class FilePathAspect
---@field expandedValue FilePath The expanded value of the aspect.
---@field defaultPath FilePath The default path of the aspect.
settings.FilePathAspect = {}

---Create a new FilePathAspect
---@param options FilePathAspectCreate : TypedAspectCreate<string>
---@return FilePathAspect
function settings.FilePathAspect.create(options) end

---Set the value of the aspect
---@param value string|FilePath The value to set.
function settings.FilePathAspect:setValue(value) end

settings.IntegerAspect = {}
function settings.IntegerAspect.create(options) end

settings.DoubleAspect = {}
function settings.DoubleAspect.create(options) end

settings.StringListAspect = {}
function settings.StringListAspect.create(options) end

settings.FilePathListAspect = {}
function settings.FilePathListAspect.create(options) end

settings.IntegersAspect = {}
function settings.IntegersAspect.create(options) end

settings.StringSelectionAspect = {}
function settings.StringSelectionAspect.create(options) end

---@class OptionsPage
settings.OptionsPage = {}

---@class OptionsPageCreate
---@field id string
---@field displayName string
---@field categoryId string
---@field displayCategory string
---@field categoryIconPath string|FilePath
---@field aspectContainer AspectContainer
OptionsPageCreate = {}

---Creates a new OptionsPage.
---@param options OptionsPageCreate
---@return OptionsPage
function settings.OptionsPage.create(options) end

---Shows options page.
function settings.OptionsPage:show() end

---@class ExtensionOptionsPage
settings.extensionOptionsPage = {}

---Create an ExtensionOptionsPage.
---@param aspectContainer AspectContainer
---@return ExtensionOptionsPage
function settings.extensionOptionsPage.create(aspectContainer) end

---Show the options page.
function settings.extensionOptionsPage:show() end
return settings
