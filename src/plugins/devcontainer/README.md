# Devcontainer Plugin for Qt Creator

## Custom configuration support

```json
// Example of a devcontainer.json customization for Qt Creator
{
    "customizations": {
        "qt-creator": {
            "device": {
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
| `run-processes-in-terminal` | boolean | If set to true, some of the devcontainer setup processes will be run in a terminal window. Currently only used for `docker build`. |
| `copy-cmd-bridge` | boolean | If set to true, the cmd bridge script will be copied into the devcontainer instead of trying to mount it. This is useful if the devcontainer is not able to mount the host filesystem. |
| `mount-libexec` | boolean | If set to true, the libexec directory will be mounted into the devcontainer. This is used for the cmd bridge. |
| `libexec-mount-point` | string | The mount point for the libexec directory in the devcontainer. This is used for the cmd bridge. |
