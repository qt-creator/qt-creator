-- Copyright (C) 2024 The Qt Company Ltd.
-- SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
local LSP = require('LSP')
local mm = require('MessageManager')
local Utils = require('Utils')
local S = require('Settings')
local Gui = require('Gui')
local a = require('async')
local fetch = require('Fetch').fetch
local Install = require('Install')

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
    url = "https://qtccache.qt.io/LuaLanguageServer/LatestRelease",
    convertToTable = true
  }))

  if type(data) == "table" and #data > 0 then
    local r = data[1]
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

    Settings.binary:setValue(lspPkgInfo.path:resolvePath(binary))
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
    startBehavior = "RequiresProject",
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
  --- "using namespace Gui"
  local _ENV = using(Gui)

  local layout = Form {
    Settings.binary, br,
    Settings.developMode, br,
    Settings.showSource, br,
    Settings.showNode, br,
      Row {
        PushButton {
        text = "Update Lua Language Server",
          onClicked = function() a.sync(installOrUpdateServer)() end,
        },
        st
    }
  }
  return layout
end

local function binaryFromPkg()
  local lspPkgInfo = Install.packageInfo("lua-language-server")
  if lspPkgInfo then
    local binary = "bin/lua-language-server"
    if Utils.HostOsInfo.isWindowsHost() then
      binary = "bin/lua-language-server.exe"
    end
    local binaryPath = lspPkgInfo.path:resolvePath(binary)
    if binaryPath:isExecutableFile() == true then
      return binaryPath
    end
  end

  return nil
end

local function findBinary()
  local binary = binaryFromPkg()
  if binary then
    return binary
  end

  -- Search for the binary in the PATH
  local serverPath = Utils.FilePath.fromUserInput("lua-language-server")
  local absolute = a.wait(serverPath:searchInPath()):resolveSymlinks()
  if absolute:isExecutableFile() == true then
    return absolute
  end
  return serverPath
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
    defaultPath = findBinary(),
  })

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
