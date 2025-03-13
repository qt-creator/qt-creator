### Testing QNX setup without hardware ###

It is possible to test QNX without hardware on a plain Linux host system.

Note: This doc assumes you have:
1.1 An access to actual qnx target device available via IP address that has Qt installed.
1.2 A QNX SDP
1.3 A QNX license (placed under: $HOME/.qnx/license/licenses).

# Installations

2.1 Unpack qnx710-windows-linux-20240417.tar.xz in ~/qnx folder.
2.2 Install 'chrpath' on your linux host.
2.3 Install locally Qt for QNX via Qt installer / maintenance tool.
    Install matching version of the one that is on the remote device.
    Point Qt installer to ~/qnx/qnx710-windows-linux-20240417/qnx710 for the SDP dir.
2.4 Ensure you have cmake 3.21.1 at minimum installed on your host.

# Creator configuration

3.1 Ensure your installed Qt for QNX are detected:
    Preferences | Kits | Qt Versions should list installed versions.
    Preferences | Kits | Kits should list installed kits (won't be used!).
    If not, try: Preferences | Kits | Qt Versions | Link with Qt...
    and point to the installed Qt for QNX master dir.
3.2 Create QNX device, pointing to the IP address from 1.1:
    Preferences | Devices | Add | QNX Device
3.3 Run device Test - it should already pass.
3.4 Ensure the right "Access via" value for the created QNX device:
    it should be: Direct (not Local PC).
3.4 Add QNX SDK:
    Preferences | SDKs | QNX | Add...
    Point into ~/qnx/qnx710-windows-linux-20240417/qnx710/qnxsdp-env.sh
    This should fill the QNX page with content and you should see 3 new buttons:
    Create Kit for aarch64le, Create Kit for x86_64 and Create Kit for armle-v7.
3.5 Create a kit: press one of the buttons from the previous point.
    Ensure it matches the architecture of the remote device.
    You won't see any feedback, but when you switch to the Kits | Kits tab,
    you should see a new kit created.
3.6 It might happen that the created kit doesn't have matching
    compiler/debugger/Qt version selected. Ensure they have something like:
    Compiler: QCC for x86_64 - qnx7 (64-bit ARM)
    Debugger: Debugger for QNX 7.1.0 ARMv8
    Qt version: Qt 6.8.2 for QNX 7.1 ARMv8

# Create a qnx project

4.1 Try creating a new project:
    File | New Project... | Application (Qt) | Qt Console Application
    and select the Kit created in point 3.5
4.2 Go to project run settings and fill up the "Alternate executable on device:" field.
    Check "Use this command instead" on the right side first to enable the line.
    Fill it with "/tmp/bin/[your_target_name]" by typing.
    This should match the table in the "Deployment" section above.
    Don't worry that it stays red, indicating it's not an existing path.
4.3 The project should build, deploy and run on remote now.

