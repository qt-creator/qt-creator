# GitHub Actions & Workflows

The `build_qmake.yml` in this directory adds a [GitHub action][1] and workflow that builds
your plugin anytime you push commits to GitHub on Windows, Linux and macOS.

The build artifacts can be downloaded from GitHub and be installed into an existing Qt Creator
installation.

When you push a tag, the workflow also creates a new release on GitHub.

## Keeping it up to date

Near the top of the file you find a section starting with `env:`.

The value for `QT_VERSION` specifies the Qt version to use for building the plugin.

The value for `QT_CREATOR_VERSION` specifies the Qt Creator version to use for building the plugin.

You need to keep these two values updated for different versions of your plugin, and take care
that the Qt version and Qt Creator version you specify are compatible.

## What it does

The build job consists of several steps:

* Install required packages on the build host
* Download, unpack and install the binary for the Qt version
* Download and unpack the binary for the Qt Creator version
* Build the plugin and upload the plugin libraries to GitHub
* If a tag is pushed, create a release on GitHub for the tag, including zipped plugin libraries
  for download

## Limitations

If your plugin requires additional resources besides the plugin library, you need to adapt the
script accordingly.

Only released versions of Qt and Qt Creator are supported. Building against Qt Creator snapshots
is currently not supported.

[1]: https://help.github.com/en/actions/automating-your-workflow-with-github-actions/about-github-actions
