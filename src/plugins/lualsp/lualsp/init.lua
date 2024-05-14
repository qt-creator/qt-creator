-- Copyright (C) 2024 The Qt Company Ltd.
-- SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
local LSP = require('LSP')
local mm = require('MessageManager')
local Utils = require('Utils')
local S = require('Settings')
local Layout = require('Layout')
local a = require('async')
local fetch = require('Fetch').fetch

Settings = {}

local function createCommand()
  local cmd = { Settings.binary.expandedValue:nativePath() }
  if Settings.showNode.value then
    table.insert(cmd, '--shownode=true')
  end
  if Settings.showSource.value then
    table.insert(cmd, '--showsource=true')
  end
  if Settings.developMode.value then
    table.insert(cmd, '--develop=true')
  end

  return cmd
end
local function filter(tbl, callback)
  for i = #tbl, 1, -1 do
    if not callback(tbl[i]) then
      table.remove(tbl, i)
    end
  end
end

local function installOrUpdateServer()
  local data = a.wait(fetch({
    url = "https://api.github.com/repos/LuaLS/lua-language-server/releases?per_page=1",
    convertToTable = true,
    headers = {
      Accept = "application/vnd.github.v3+json",
      ["X-GitHub-Api-Version"] = "2022-11-28"
    }
  }))

  if type(data) == "table" and #data > 0 then
    local r = data[1]
    Install = require('Install')
    local lspPkgInfo = Install.packageInfo("lua-language-server")
    if not lspPkgInfo or lspPkgInfo.version ~= r.tag_name then
      local osTr = { mac = "darwin", windows = "win32", linux = "linux" }
      local archTr = { unknown = "", x86 = "ia32", x86_64 = "x64", itanium = "", arm = "", arm64 = "arm64" }
      local os = osTr[Utils.HostOsInfo.os]
      local arch = archTr[Utils.HostOsInfo.architecture]

      local expectedFileName = "lua-language-server-" .. r.tag_name .. "-" .. os .. "-" .. arch

      filter(r.assets, function(asset)
        return string.find(asset.name, expectedFileName, 1, true) == 1
      end)

      if #r.assets == 0 then
        print("No assets found for this platform")
        return
      end
      local res, err = a.wait(Install.install(
        "Do you want to install the lua-language-server?", {
        name = "lua-language-server",
        url = r.assets[1].browser_download_url,
        version = r.tag_name
      }))

      if not res then
        mm.writeFlashing("Failed to install lua-language-server: " .. err)
        return
      end

      lspPkgInfo = Install.packageInfo("lua-language-server")
      print("Installed:", lspPkgInfo.name, "version:", lspPkgInfo.version, "at", lspPkgInfo.path)
    end

    local binary = "bin/lua-language-server"
    if Utils.HostOsInfo.isWindowsHost() then
      binary = "bin/lua-language-server.exe"
    end

    Settings.binary.defaultPath = lspPkgInfo.path:resolvePath(binary)
    Settings:apply()
    return
  end

  if type(data) == "string" then
    print("Failed to fetch:", data)
  else
    print("No lua-language-server release found.")
  end
end
IsTryingToInstall = false

local function setupClient()
  Client = LSP.Client.create({
    name = 'Lua Language Server',
    cmd = createCommand,
    transport = 'stdio',
    languageFilter = {
      patterns = { '*.lua' },
      mimeTypes = { 'text/x-lua' }
    },
    settings = Settings,
    startBehavior = "RequiresFile",
    onStartFailed = function()
      a.sync(function()
        if IsTryingToInstall == true then
          mm.writeFlashing("RECURSION!");
          return
        end
        IsTryingToInstall = true
        installOrUpdateServer()
        IsTryingToInstall = false
      end)()
    end
  })

  Client.on_instance_start = function()
    print("Instance has started")
  end

  Client:registerMessage("$/status/report", function(params)
    mm.writeFlashing(params.params.text .. ": " .. params.params.tooltip);
  end)
end

local function using(tbl)
  local result = _G
  for k, v in pairs(tbl) do result[k] = v end
  return result
end
local function layoutSettings()
  --- "using namespace Layout"
  local _ENV = using(Layout)

  local installButton = {}

  if Settings.binary.expandedValue:isExecutableFile() == false then
    installButton = {
      "Language server not found:",
      Row {
        PushButton {
          text("Try to install lua language server"),
          onClicked(function() a.sync(installOrUpdateServer)() end),
          br,
        },
        st
      }
    }
  end
  local layout = Form {
    Settings.binary, br,
    Settings.developMode, br,
    Settings.showSource, br,
    Settings.showNode, br,
    table.unpack(installButton)
  }

  return layout
end

local function setupAspect()
  ---@class Settings: AspectContainer
  Settings = S.AspectContainer.create({
    autoApply = false,
    layouter = layoutSettings,
  });

  Settings.binary = S.FilePathAspect.create({
    settingsKey = "LuaCopilot.Binary",
    displayName = "Binary",
    labelText = "Binary:",
    toolTip = "The path to the lua-language-server binary.",
    expectedKind = S.Kind.ExistingCommand,
    defaultPath = Utils.FilePath.fromUserInput("lua-language-server"),
  })
  -- Search for the binary in the PATH
  local serverPath = Settings.binary.defaultPath
  local absolute = a.wait(serverPath:searchInPath()):resolveSymlinks()
  if absolute:isExecutableFile() == true then
    Settings.binary.defaultPath = absolute
  end
  Settings.developMode = S.BoolAspect.create({
    settingsKey = "LuaCopilot.DevelopMode",
    displayName = "Enable Develop Mode",
    labelText = "Enable Develop Mode:",
    toolTip = "Turns on the develop mode of the language server.",
    defaultValue = false,
    labelPlacement = S.LabelPlacement.InExtraLabel,
  })

  Settings.showSource = S.BoolAspect.create({
    settingsKey = "LuaCopilot.ShowSource",
    displayName = "Show Source",
    labelText = "Show Source:",
    toolTip = "Display the internal data of the hovering token.",
    defaultValue = false,
    labelPlacement = S.LabelPlacement.InExtraLabel,
  })

  Settings.showNode = S.BoolAspect.create({
    settingsKey = "LuaCopilot.ShowNode",
    displayName = "Show Node",
    labelText = "Show Node:",
    toolTip = "Display the internal data of the hovering token.",
    defaultValue = false,
    labelPlacement = S.LabelPlacement.InExtraLabel,
  })
  return Settings
end
local function setup(parameters)
  setupAspect()
  setupClient()
end

return {
  setup = function() a.sync(setup)() end,
}
