# Development Container Plugin for Qt Creator

The plugin provides support for development containers in the IDE.

It allows you to easily configure and manage containers for your projects.

You can find a full specification of and documentation about the configuration format at [https://containers.dev/](https://containers.dev/)

## Custom configuration support

Example of a devcontainer.json with customizations for Qt Creator:

```json
{
    "customizations": {
        "qt-creator": {
            "device": {
                "auto-detect-kits": true,
                "run-processes-in-terminal": false,
                "copy-cmd-bridge": false,
                "mount-libexec": true,
                "libexec-mount-point": "/devcontainer/libexec"
            }
        }
    }
}
```

The following table shows the available options for customizing Qt Creator in the `devcontainer.json` file:

| Key | Type | Description |
| --- | ---- | ----------- |
| `auto-detect-kits` | boolean | If set to true, the Development Container Support tries to automatically detect a kit in the development container. |
| `run-processes-in-terminal` | boolean | If set to true, some of the development container setup processes are run in a terminal window. Currently only used for `docker build`. |
| `copy-cmd-bridge` | boolean | If set to true, the command bridge helper is copied into the development container instead of trying to mount it. This is useful if the development container is not able to mount the host filesystem. |
| `mount-libexec` | boolean | If set to true, the libexec directory is mounted into the development container. This is used for the Command Bridge Helper. |
| `libexec-mount-point` | string | The mount point for the libexec directory in the development container. This is used for the Command Bridge Helper. |
| `kits` | array of objects | An array of custom kits to be used in the development container. See below for more details. |

## Custom Kits

Instead of having Qt Creator auto detect kits based on the PATH Environment variable, you can define custom kits in the `devcontainer.json` file. This is useful if you want to use a specific version of Qt or a specific compiler. It also allows you to define more than one kit, which can be useful for cross-compilation or different build configurations.

```json
{
    "customizations": {
        "qt-creator": {
            "auto-detect-kits": false,
            "kits": [
                {
                    "name": "My DevContainer Kit",
                    "qt": "/6.7.0/gcc_64/bin/qmake6",
                    "compiler": {
                        "Cxx": "/usr/bin/c++",
                        "C": "/usr/bin/gcc"
                    },
                    "cmake": {
                        "binary": "/usr/bin/cmake",
                        "generator": "Unix Makefiles"
                    },
                    "debugger": "/usr/bin/lldb"
                }
            ]
        }
    }
}
```

## JSON Language Server

When you open the `devcontainer.json` file, the JSON Language Server provides features such as validation, autocompletion, and hover documentation for the configuration options.
You may have to install the JSON Language Server to take full advantage of these features.
