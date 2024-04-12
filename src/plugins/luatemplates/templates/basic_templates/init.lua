-- Copyright (C) 2024 The Qt Company Ltd.
-- SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
local Wizard = require('Wizard')
local Layout = require('Layout')
local Settings = require('Settings')
local Core = require('Core')
local Utils = require('Utils')

---@param settings AspectContainer
local function generateFiles(settings)
    local mainFile = Core.GeneratedFile.new()
    mainFile.filePath = settings.path.expandedValue:resolvePath(settings.fileName.value)
    mainFile.contents = [[print("Hello world!")]]
    mainFile.attributes = Core.GeneratedFile.Attribute.OpenEditorAttribute
    return { mainFile }
end

local function createWizard(path)
    ---@class AspectContainer
    local settings = Settings.AspectContainer.create({
        autoApply = true,
    })
    settings.fileName = Settings.StringAspect.create({
        defaultValue = "script.lua",
        displayStyle = Settings.StringDisplayStyle.LineEdit,
        historyId = "BasicTemplate.FileName",
    })
    settings.path = Settings.FilePathAspect.create({
        defaultPath = path,
        expectedKind = Settings.Kind.ExistingDirectory,
    })

    local wizard = Wizard.create({
        fileFactory = function() return generateFiles(settings) end,
    })

    wizard:addPage({
        title = "Location",
        layout = Layout.Form {
            "File name:", settings.fileName, Layout.br,
            "Path:", settings.path, Layout.br,
        },
    })

    wizard:addSummaryPage({
        initializePage = function(page)
            page:setFiles(generateFiles(settings))
        end
    })
    return wizard
end

local function setup()
    Wizard.registerFactory({
        id = "org.qtproject.Qt.QtCreator.Plugin.LuaTemplates.BasicTemplates",
        displayName = "Basic Templates",
        description = "Basic Template for Lua",
        category = "Lua",
        displayCategory = "Lua",
        iconText = "lua",
        factory = createWizard,
    })
end

return { setup = setup }
