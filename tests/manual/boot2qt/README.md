

### Testing Boot2Qt setup without hardware ###

It is possible to test Boot2Qt without hardware on a plain Linux host system.

Note: You need an ssh-accessible "root" user on the machine, open X access,
and must be willing to use it. This is not meant for production environments!


# Prepare your machine

ssh-copy-id -i ~/.ssh/id_??????.pub root@localhost
xhost +

# Get appcontroller source and build

git clone ssh://codereview.qt-project.org/qt-apps/boot2qt-appcontroller

cd boot2qt-appcontroller
/path/to/qt-base/bin/qt-cmake ...
ninja ...

# Copy binary to "proper" location

sudo ln -s `pwd`/appcontroller /usr/bin/appcontroller


# Set up "Boot to Qt" Device in Creator

Ensure the "Boot to Qt" plugin is enabled

Edit -> Preferences -> Devices, "Add...", "Boot to Qt Device", "Start Wizard"
Device Name: LocalHostForBoot2Qt
Device Address: 127.0.0.1

Press "Apply"

# Create a suitable Kit

Clone your standard kit for normal local work
Change the "Run Device" to LocalHostForBoot2Qt
Keep the "Build Device" at "Desktop Device"

Press "Apply"

The kit will have a warning the "Device type is not supported by Qt version" - that's ok.

# Create a test project

File -> New Project -> Qt Quick Application

# Tweak project settings

Deployment method: "Deploy to Boot to Qt Target"

Run Command line arguments: Add  "-display :0"
Run Environment: Add LD_LIBRARY_PATH=/path/to/qt-base/lib

"Executable on Device" may be red at that stage, it shold get black after a successful build


# Build

Ctrl-B


# Test

At that stage, the program should be runnable, debuggable (C++, QML, and combined QML/C++),
and Qml-Profilable

