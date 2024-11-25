# lupdate.lua

lupdate.lua allows you to update your .ts files from your .lua files.

## Installation

You need to install lua, luarocks and luafilesystem.

On macOS:

```sh
$ brew install lua luarocks
$ luarocks install luafilesystem
```

For other platforms see: [Download luarocks](https://github.com/luarocks/luarocks/wiki/Download)

## Usage

You need to add a "languages" key to your plugin spec file.

```lua
--- In your plugin.lua file
return {
    Name = "MyPlugin",
    Version = "1.0.0",
    languages = {"de", "fr", "en"},
    --- ....
} --[[@as QtcPlugin]]
```

Then run the lupdate.lua script in your plugin directory. Make sure that lupdate is in your PATH.

```sh
$ # export PATH=$PATH:/path/to/Qt/bin
$ cd my-plugin
$ lua lupdate.lua
```

Once you have the .ts files you can use Qt Linguist to translate your strings.

After translation you can run lrelease to generate the .qm files.

```sh
$ cd ts
$ lrelease *.ts
```

## Background

Since Qt's lupdate does not currently support lua files, the lupdate.lua script uses a trick
to make it work. It creates a temporary file for each lua file and adds a comment at the start
and end of the file:

```lua
--- class Plugin { Q_OBJECT

print(tr("Hello World"))

--- }
```

That way lupdate thinks its inside a C++ Plugin and takes as context the name of the class, in this case "Plugin".

It then starts the actual lupdate tool to update the .ts files.
