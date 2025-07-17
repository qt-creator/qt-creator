# Devcontainer Plugin for Qt Creator

## Custom configuration support

```json
// Example of a devcontainer.json customization for Qt Creator
{
    "customizations": {
        "qt-creator": {
            "device": {
                "auto-detect-kits": true
                "run-processes-in-terminal": false,
                "copy-cmd-bridge": false,
                "mount-libexec": true,
                "libexec-mount-point": "/devcontainer/libexec"
            }
        }
    }
}
```

| Key | Type | Description |
| --- | ---- | ----------- |
| `auto-detect-kits` | boolean | If set to true, the devcontainer will try to automatically detect a kit in the devcontainer. |
| `run-processes-in-terminal` | boolean | If set to true, some of the devcontainer setup processes will be run in a terminal window. Currently only used for `docker build`. |
| `copy-cmd-bridge` | boolean | If set to true, the cmd bridge script will be copied into the devcontainer instead of trying to mount it. This is useful if the devcontainer is not able to mount the host filesystem. |
| `mount-libexec` | boolean | If set to true, the libexec directory will be mounted into the devcontainer. This is used for the cmd bridge. |
| `libexec-mount-point` | string | The mount point for the libexec directory in the devcontainer. This is used for the cmd bridge. |

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
                    "QtSupport.QtInformation": "/6.7.0/gcc_64/bin/qmake6",
                    "PE.Profile.ToolChainsV3": {
                        "Cxx": "/usr/bin/c++",
                        "C": "/usr/bin/gcc"
                    },
                    "CMakeProjectManager.CMakeKitInformation": {
                        "binary": "/usr/bin/cmake",
                        "generator": "Unix Makefiles"
                    },
                    "Debugger.Information": "/usr/bin/lldb"
                }
            ]
        }
    }
}
```
