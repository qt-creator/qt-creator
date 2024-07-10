# Lua Plugin

## Introduction

The Lua plugin provides support for writing plugins using the Lua scripting language.

## Usage

The plugin scans the normal plugin folders of Qt Creator
`ExtensionSystem::PluginManager::pluginPaths()`. It loads scripts from any folder that contains
a .lua script named the same as the folder.
Whether or not the script is enabled is determined by the `disabledByDefault` field in the plugin
table and the settings configured via the "About Plugins" dialog in Qt Creator.

## Basic Lua plugin

A Lua script needs to provide the following table to be considered a plugin:

```lua
-- myluaplugin/myluaplugin.lua
return {
    name = "MyLuaPlugin",
    version = "1.0.0",
    compatVersion = "1.0.0",
    vendor = "The Qt Company",
    category = "Language Client",
    setup = function() print("Hello World!") end,

    --- The following fields are optional
    description = "My first lua plugin",
    longDescription = [[
A long description.
Can contain newlines.
    ]],
    url = "https://www.qt.io",
    license = "MIT",
    revision = "rev1",
    copyright = "2024",
    experimental = true,
    disabledByDefault = false,

    dependencies = {
        { name="Core", version = "14.0.0" }
    },
} --[[@as QtcPlugin]]
```

Your base file needs to be named the same as the folder its contained in.
It must only return the plugin specification table and not execute or require any other code.
Use `require` to load other files from within the setup function.

```lua
-- myluaplugin/myluaplugin.lua
return {
    -- ... required fields omitted ..
    setup = function() require 'init'.setup() end,
} --[[@as QtcPlugin]]

-- myluaplugin/init.lua
local function setup()
    print("Hello from Lua!")
end

return {
    setup = setup,
}
```

The `require` function will search for files as such:

```
my-lua-plugin/?.lua
```

## Lua <=> C++ bindings

The Lua plugin provides the [sol2](https://github.com/ThePhD/sol2) library to bind C++ code to Lua.
sol2 is well [documented here](https://sol2.rtfd.io).

## Lua Language Server

To make developing plugins easier, we provide a meta file [qtc.lua](meta/qtc.lua) that describes
what functions and classes are available in the Lua plugin. If you add bindings yourself
please add them to this file. The [.luarc.json](../../.luarc.json) file contains the configuration
for the [Lua Language Server](https://luals.github.io/) and will automatically load the `qtc.lua` file.

## Coroutines

A lot of Qt Creator functions will take some time to complete. To not block the main thread during
that time, we make heavy use of lua coroutines. Functions that need a coroutine to work are documented
as such, lets take for example `qtc.waitms(ms)`. This function will wait for `ms` milliseconds and
then return. To use it you need to call it using the async module from a running coroutine:

```lua
local a = require 'async'
local function myFunction()
    a.wait(qtc.waitms(1000))
    print("Hello from Lua!")
end

local function setup()
    a.sync(myFunction)()
end
```

The waitms function will immediately yield, which will suspend the execution of `myFunction` **AND**
make the `a.sync(myFunction)()` return.

Once the internal timer is triggered, the C++ code will resume `myFunction` and it will continue to
print the message. `myFunction` will then return and the coroutine will be "dead", meaning it cannot
be resumed again. You can of course create a new coroutine and call `myFunction` again.

## Contributing

Contributions to the Lua plugin are welcome. Please read the contributing guide for more information.
