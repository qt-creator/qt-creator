This folder contains various files used to build the Mac installer.

# Entitlements

The entitlements here will be picked up by [scripts/build.py](../../../scripts/build.py)
via [scripts/common.py](../../../scripts/common.py) `codesign_executable()` based on their name.
If you need a new application to be signed with specific entitlements, you can simply add a file
called `your-app-name.entitlements` to this folder.
