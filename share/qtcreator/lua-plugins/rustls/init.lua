-- Copyright (C) 2024 The Qt Company Ltd.
-- SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
local LSP = require('LSP')
local mm = require('MessageManager')
local Utils = require('Utils')
local Process = require('Process')
local S = require('Settings')
local Gui = require('Gui')
local a = require('async')
local fetch = require('Fetch').fetch
local Install = require('Install')

Settings = {}

local function createCommand()
  local cmd = { Settings.binary.expandedValue:nativePath() }
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
    url = "https://qtccache.qt.io/RustLanguageServer/LatestReleases",
    convertToTable = true,
  }))

  if type(data) == "table" and #data > 1 then
    local r = data[1]
    if r.prerelease then
      r = data[2]
    end
    local lspPkgInfo = Install.packageInfo("rust-analyzer")
    if not lspPkgInfo or lspPkgInfo.version ~= r.tag_name then
      local osTr = { mac = "apple-darwin", windows = "pc-windows-msvc", linux = "unknown-linux-gnu" }
      local archTr = { unknown = "", x86 = "", x86_64 = "x86_64", itanium = "", arm = "", arm64 = "aarch64" }
      local extTr = { mac = "gz", windows = "zip", linux = "gz" }
      local os = osTr[Utils.HostOsInfo.os]
      local arch = archTr[Utils.HostOsInfo.architecture]
      local ext = extTr[Utils.HostOsInfo.os]

      local expectedFileName = "rust-analyzer-" .. arch .. "-" .. os .. "." .. ext

      filter(r.assets, function(asset)
        return string.find(asset.name, expectedFileName, 1, true) == 1
      end)

      if #r.assets == 0 then
        print("No assets found for this platform")
        return
      end
      local res, err = a.wait(Install.install(
        "Do you want to install the rust-analyzer?", {
        name = "rust-analyzer",
        url = r.assets[1].browser_download_url,
        version = r.tag_name
      }))

      if not res then
        mm.writeFlashing("Failed to install rust-analyzer: " .. err)
        return
      end

      lspPkgInfo = Install.packageInfo("rust-analyzer")
      print("Installed:", lspPkgInfo.name, "version:", lspPkgInfo.version, "at", lspPkgInfo.path)
    end

    local binary = "rust-analyzer"
    if Utils.HostOsInfo.isWindowsHost() then
      binary = "rust-analyzer.exe"
    end

    Settings.binary:setValue(lspPkgInfo.path:resolvePath(binary))
    Settings:apply()
    return
  end

  if type(data) == "string" then
    print("Failed to fetch:", data)
  else
    print("No rust-analyzer release found.")
  end
end
IsTryingToInstall = false

local function setupClient()
  Client = LSP.Client.create({
    name = 'Rust Language Server',
    cmd = createCommand,
    transport = 'stdio',
    languageFilter = {
      patterns = { '*.rs' },
      mimeTypes = { 'text/rust' }
    },
    settings = Settings,
    startBehavior = "RequiresProject",
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
  --- "using namespace Gui"
  local _ENV = using(Gui)

  local layout = Form {
    Settings.binary, br,
    Row {
      PushButton {
        text = "Try to install Rust language server",
        onClicked = function() a.sync(installOrUpdateServer)() end,
        br,
      },
      st
    }
  }

  return layout
end

local function binaryFromPkg()
  local lspPkgInfo = Install.packageInfo("rust-analyzer")
  if lspPkgInfo then
    local binary = "rust-analyzer"
    if Utils.HostOsInfo.isWindowsHost() then
      binary = "rust-analyzer.exe"
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
  local serverPath = Utils.FilePath.fromUserInput("rust-analyzer")
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
    settingsKey = "Rustls.Binary",
    displayName = "Binary",
    labelText = "Binary:",
    toolTip = "The path to the rust analyzer binary.",
    expectedKind = S.Kind.ExistingCommand,
    defaultPath = findBinary(),
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
