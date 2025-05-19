# Testing Boot2Qt with the Hardware Pool

## Installing the Requirements

On macOS, you need to have Docker Desktop installed, including the Docker CLI.

Install the Boot to Qt Software Stack for the Qt version and device hardware
that you want to test with the Qt Online Installer

On macOS, the installer might complain about not being able to run docker.
If that is the case, copy the command line from the dialog into a Terminal
with the Docker CLI in PATH and execute it manually. Choose `Ignore` in the
installer dialog when ready.

## Reserving a Device

Log into https://hw-cloud.qt.io with your Qt Account. Select `Reserve a Device`,
choose the appropriate device hardware, and select `Reserve` for it.
Select the Qt version that matches what you installed with the Qt Online
Installer, choose a reservation duration, state a reason for your device use,
and select `Reserve device`.

The device is then provisioned and booted. When ready, make note of the device
IP address.

## Configuring Qt Creator

Start Qt Creator, open the device options, and add a `Boot to Qt Device`.
Give it a name, and provide the IP that you noted when reserving the device.
The user is `root`.

Now you can open or create a project, select the corresponding Boot to Qt kit,
and build, run & debug.

While an application is running on the device, you can use a VNC[^vnc] viewer to
connect to the apps UI with the device's IP.

[^vnc]: The creation of the VNC connection depends on setting
        `QT_QPA_PLATFORM=vnc` in `/etc/default/qt` on the device. That should
        be enabled by default for devices created with the web interface.

## Releasing the Device

When you are finished with testing, release the reservation of the device
in the web interface.

# Testing Boot2Qt setup without hardware ###

It is possible to test Boot2Qt without hardware on a plain Linux host system.

Note: You need an ssh-accessible "root" user on the machine, open X access,
and must be willing to use it. This is not meant for production environments!


## Prepare your machine

ssh-copy-id -i ~/.ssh/id_??????.pub root@localhost
xhost +

## Get appcontroller source and build

git clone ssh://codereview.qt-project.org/qt-apps/boot2qt-appcontroller

cd boot2qt-appcontroller
/path/to/qt-base/bin/qt-cmake ...
ninja ...

## Copy binary to "proper" location

sudo ln -s `pwd`/appcontroller /usr/bin/appcontroller


## Set up "Boot to Qt" Device in Creator

Ensure the "Boot to Qt" plugin is enabled

Edit -> Preferences -> Devices, "Add...", "Boot to Qt Device", "Start Wizard"
Device Name: LocalHostForBoot2Qt
Device Address: 127.0.0.1

Press "Apply"

## Create a suitable Kit

Clone your standard kit for normal local work
Change the "Run Device" to LocalHostForBoot2Qt
Keep the "Build Device" at "Desktop Device"

Press "Apply"

The kit will have a warning the "Device type is not supported by Qt version" - that's ok.

## Create a test project

File -> New Project -> Qt Quick Application

## Tweak project settings

Deployment method: "Deploy to Boot to Qt Target"

Run Command line arguments: Add  "-display :0"
Run Environment: Add LD_LIBRARY_PATH=/path/to/qt-base/lib

"Executable on Device" may be red at that stage, it shold get black after a successful build


## Build

Ctrl-B


## Test

At that stage, the program should be runnable, debuggable (C++, QML, and combined QML/C++),
and Qml-Profilable

