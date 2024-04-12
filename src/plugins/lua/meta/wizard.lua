---@meta Wizard

---@module "Layout"
local Layout = require("Layout")

---@module "Core"
local Core = require("Core")

local wizard = {}

---@class Factory

---@class (exact) WizardFactoryOptions
---@field id string
---@field displayName string
---@field description string
---@field category string
---@field displayCategory string
---@field icon? string
---@field iconText? string
---@field factory function A function returning a Wizard

--- Registers a wizard factory.
---@param options WizardFactoryOptions
---@return Factory
function wizard.registerFactory(options) end

---@class Wizard
Wizard = {}

---@class (exact) WizardPageOptions
---@field title string
---@field layout LayoutItem
---@field initializePage? function The function called before showing the page

---Add a page to the wizard
---@param options WizardPageOptions
function Wizard:addPage(options) end

---@class SummaryPage
Wizard.SummaryPage = {}

---Set the files to be shown on the summary page
---@param generatedFiles Core.GeneratedFile[]
function Wizard.SummaryPage:setFiles(generatedFiles) end

---@class SummaryPageOptions
---@field title? string
---@field initializePage? function The function called before showing the page

---Add a summary page to the wizard
---@param options SummaryPageOptions
---@return SummaryPage
function Wizard:addSummaryPage(options) end

---@class WizardOptions
---@field fileFactory function A function returning a GeneratedFile[]

---Create a wizard
---@param options WizardOptions
---@return Wizard
function wizard.create(options) end

return wizard
