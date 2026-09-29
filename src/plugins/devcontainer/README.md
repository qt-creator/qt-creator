# Development Container Plugin for Qt Creator

The plugin provides support for development containers in the IDE.

It allows you to easily configure and manage containers for your projects.

You can find a full specification of and documentation about the configuration format at [https://containers.dev/](https://containers.dev/)

## Features

[Features](https://containers.dev/implementors/features/) listed in the `features` property
are installed on top of the image, for `image`, `build` and `dockerComposeFile` configurations:

```json
{
    "image": "ubuntu:24.04",
    "features": {
        "ghcr.io/devcontainers/features/common-utils:2": { "username": "automatic" },
        "https://example.com/features/devcontainer-feature-mytool.tgz": {},
        "./local-feature": {}
    },
    "overrideFeatureInstallOrder": [ "ghcr.io/devcontainers/features/common-utils" ],
    "remoteUser": "dev"
}
```

A feature can come from an OCI registry, from an HTTPS URL to a `devcontainer-feature-<id>.tgz` file,
or from a folder next to the `devcontainer.json`. The features a feature `dependsOn` are
installed as well, and the order respects `dependsOn`, `installsAfter` and
`overrideFeatureInstallOrder`. What the features declare for the container (`containerEnv`,
`mounts`, `capAdd`, `securityOpt`, `init`, `privileged`, `entrypoint`, `customizations` and the
lifecycle commands) is merged with the configuration. The lifecycle commands of the features run
before the ones of the configuration.

For registries that require credentials, log in with `docker login <registry>`. The credentials
are taken from the Docker configuration (`$DOCKER_CONFIG/config.json` or
`~/.docker/config.json`), including credential helpers (`credHelpers` and `credsStore`).

Downloaded features are cached, so they are not downloaded again while they are current, and a
cached copy is used when the registry or server cannot be reached.

## Lifecycle commands

`onCreateCommand`, `updateContentCommand` and `postCreateCommand` run when the container is
created, `postStartCommand` each time it is started, and `postAttachCommand` each time Qt Creator
connects to it. `initializeCommand` runs on the host before the container is built.

## User IDs

On Linux hosts, the UID and GID of the `remoteUser` (or the `containerUser`) are changed to the
ones of your user, so files in the mounted workspace have the right owner. Set
`"updateRemoteUserUID": false` to keep the IDs of the image.

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
