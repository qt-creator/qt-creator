-- Copyright (C) 2024 The Qt Company Ltd.
-- SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
local LSP = require('LSP')
local Utils = require('Utils')
local S = require('Settings')
local Gui = require('Gui')
local a = require('async')
local TextDocument = require('TextDocument')

local function createCommand()
  local cmd = { Settings.binary.expandedValue:nativePath() }
  return cmd
end

local function installOrUpdateServer()
  -- TODO: Download/Update/Install
end

IsTryingToInstall = false

local function setupClient()
  Client = LSP.Client.create({
    name = 'AI Assistant Server',
    cmd = createCommand,
    transport = 'stdio',
    languageFilter = {
      patterns = { '*.*' },
    },
    settings = Settings,
    startBehavior = "AlwaysOn",
    onStartFailed = function()
      a.sync(function()
        if IsTryingToInstall == true then
          return
        end
        IsTryingToInstall = true
        installOrUpdateServer()
        IsTryingToInstall = false
      end)()
    end
  })
end

local function using(tbl)
  local result = _G
  for k, v in pairs(tbl) do result[k] = v end
  return result
end

local function layoutSettings()
  local _ENV = using(Gui)

  local layout = Form {
    Settings.binary, br,
    Row {
      PushButton {
        text = "Try to install AI Assistant Server",
        onClicked = function() a.sync(installOrUpdateServer)() end,
        br,
      },
      st
    }
  }

  return layout
end

local function setupAspect()
  Settings = S.AspectContainer.create({
    autoApply = false,
    layouter = layoutSettings,
  })

  Settings.binary = S.FilePathAspect.create({
    settingsKey = "AiAssistant.Binary",
    displayName = "Binary",
    labelText = "Binary:",
    toolTip = "The path to the AI Assistant Server",
    expectedKind = S.Kind.ExistingCommand,
    defaultPath = Utils.FilePath.fromUserInput("/path/to/server"),
  })

  return Settings
end

local function fetchSuggestions()
  print("Fetching suggestions ...")
end

local function fetchSuggestionsSafe()
  local ok, err = pcall(fetchSuggestions)
  if not ok then
    print("echo Error fetching: " .. err)
  end
end

Hooks = {}

function Hooks.onDocumentContentsChanged(document)
  print("onDocumentContentsChanged() called", document)
  -- TODO:
  -- All the necessary checks before sending the request
  -- Create request:
    -- Get/Set the current document
    -- Get/Set the current cursor position
    -- Get/Set the document version
    -- Set response callback to handle the response
 end

---Called when a document is opened.
---@param document TextDocument
function Hooks.onDocumentOpened(document)
  print("TextDocument found: ", document)
end

---Called when a document is closed.
---@param document TextDocument
function Hooks.onDocumentClosed(document)
  print("Document closed:", document)
  -- TODO: Cleanup the document references and requests
end

local function setup(parameters)
  setupAspect()
  setupClient()

  Action = require("Action")
  Action.create("Trigger.suggestions", {
    text = "Trigger AI suggestions",
    onTrigger = function() a.sync(fetchSuggestionsSafe)() end,
    defaultKeySequences = { "Meta+Shift+A", "Ctrl+Shift+Alt+A" },
  })
end

return {
  setup = function() a.sync(setup)() end,
  Hooks = Hooks,
}
