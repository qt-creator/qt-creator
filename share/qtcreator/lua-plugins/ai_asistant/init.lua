-- Copyright (C) 2024 The Qt Company Ltd.
-- SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
local LSP = require('LSP')
local Utils = require('Utils')
local S = require('Settings')
local Gui = require('Gui')
local a = require('async')
local TextEditor = require('TextEditor')

Hooks = {}

local function collectSuggestions(responseTable)
  local suggestions = {}

  if type(responseTable.result) == "table" and type(responseTable.result.completions) == "table" then
    for _, completion in pairs(responseTable.result.completions) do
      if type(completion) == "table" then
        local text = completion.text or ""
        local startLine = completion.range and completion.range.start and completion.range.start.line or 0
        local startCharacter = completion.range and completion.range.start and completion.range.start.character or 0
        local endLine = completion.range and completion.range["end"] and completion.range["end"].line or 0
        local endCharacter = completion.range and completion.range["end"] and completion.range["end"].character or 0

        local suggestion = TextEditor.Suggestion.create(startLine, startCharacter, endLine, endCharacter, text)
        table.insert(suggestions, suggestion)
      end
    end
  end

  return suggestions
end

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

local function buildRequest()
  local editor = TextEditor.currentEditor()
  if editor == nil then
    print("No editor found")
    return
  end

  local document = editor:document()
  local filePath = document:file()
  local doc_version = Client.documentVersion(filePath)
  if doc_version == -1 then
    print("No document version found")
    return
  end

  local doc_uri = Client.hostPathToServerUri(filePath)
  if doc_uri == nil or doc_uri == "" then
    print("No document uri found")
    return
  end

  local main_cursor = editor:cursor():mainCursor()
  local block = main_cursor:blockNumber()
  local column = main_cursor:columnNumber()

  local request_msg = {
    jsonrpc = "2.0",
    method = "getCompletionsCycling",
    params = {
      doc = {
        position = {
          character = column,
          line = block
        },
        uri = doc_uri,
        version = doc_version
      }
    }
  }

  return request_msg
end

local function completionResponseCallback(response)
  print("completionResponseCallback() called")

  local editor = TextEditor.currentEditor()
  if editor == nil then
    print("No editor found")
    return
  end

  local suggestions = collectSuggestions(response)
  if next(suggestions) == nil then
    print("No suggestions found")
    return
  end

  local document = editor:document()
  document:setSuggestions(suggestions)
end

local function sendRequest(request)
  print("sendRequest() called")

  if Client == nil then
    print("No client found")
    return
  end

  local editor = TextEditor.currentEditor()
  if editor == nil or editor == "" then
    print("No editor found")
    return
  end

  local document = editor:document()
  local result = a.wait(Client:sendMessageWithIdForDocument(document, request))
  completionResponseCallback(result)

end

local function requestSuggestions()
  local request_msg = buildRequest()
  if(request_msg == nil) then
    print("requestSuggestions() failed to build request message")
    return
  end

  sendRequest(request_msg)
end

local function requstSuggestionsSafe()
  local ok, err = pcall(requestSuggestions)
  if not ok then
    print("echo Error fetching: " .. err)
  end
end

function Hooks.onDocumentContentsChanged(document, position, charsRemoved, charsAdded)
  print("onDocumentcontentsChanged() called, position, charsRemoved, charsAdded:", position, charsRemoved, charsAdded)

  Position = position
  local line, char = document:blockAndColumn(position)
  print("Line: ", line, "Char: ", char)
end

local function setup(parameters)
  setupAspect()
  setupClient()

  Action = require("Action")
  Action.create("Trigger.suggestions", {
    text = "Trigger AI suggestions",
    onTrigger = function() a.sync(requstSuggestionsSafe)() end,
    defaultKeySequences = { "Meta+Shift+A", "Ctrl+Shift+Alt+A" },
  })
end

return {
  setup = function() a.sync(setup)() end,
  Hooks = Hooks,
}
