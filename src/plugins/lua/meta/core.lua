---@meta Core

Core = {}

---@enum Attribute
Core.GeneratedFile.Attribute = {
    OpenEditorAttribute = 0,
    OpenProjectAttribute = 0,
    CustomGeneratorAttribute = 0,
    KeepExistingFileAttribute = 0,
    ForceOverwrite = 0,
    TemporaryFile = 0,
}

---@class GeneratedFile
---@field filePath FilePath
---@field contents string
---@field isBinary boolean
---@field attributes Attribute A combination of Attribute.
Core.GeneratedFile = {}

---Create a new GeneratedFile.
---@return GeneratedFile
function Core.GeneratedFile.new() end

return Core;
