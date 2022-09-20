# Android Builder in a Docker

## Introduction

This was a one-day hackathon where we tried to put a Qt for Android build environment into a Docker container.

## Prerequisites

Things you need:
* Docker

## Building the Docker image

The accompanying Makefile contains the target `qt-android-sdk-31` which will build the Docker image. It will take a while to download the neceesary packages.

To build it simply run `make qt-android-sdk-31`.

## Running the Docker image

The Makefile also contains a target to run the docker image called `qt-android-sdk-31-run`.

## Limitations

* The Docker image is not optimized for size. It is currently 5.5 GB.
* The Docker image is only available for x86_64, this makes it rather useless on macos M1 machines.
* Qt Creator will not detect the Android SDK, Qt, compilers and other tools.

## Future work


### Integration with Qt Creator

* Create a mechanism that allows Qt Creator to better find components inside the Docker image.
  * Our current thinking is to create some sort of .json file in a known location that contains information on where components can be found.

### Deployment

We need to come up with a way to deploy either to a Hardware device or the Emulator.
Current thinking is to run ADB inside the Docker container and forward the ports to the host.

### Emulator

We see two options for the emulator:

1. Run the emulator on the host

    This would mean that we still might have to take care of installing and maintaining the Emulator ourselves on the Host.

2. Run the emulator inside the docker and pipe its UI to the Host

    Simple on Linux via X11, but hard / impossible that way on macOS and Windows.
    Android studio seems to have a way to pipe the output into their own app, further research is needed to find a way that would allow us to do the same.

### Debugging

Probably we can run the debugger inside the container, and tunnel trough the host to the device/emulator.

### ADB Tunnel

We would want a generic way to connect to the device/emulator. The current thinking is that this might be possible by using a tunnel from the docker to the host.

