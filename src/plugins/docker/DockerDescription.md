Turn Docker images into devices, and build, run, and debug your
applications inside containers.

![A Docker device in the device preferences](https://qtccache.qt.io/images/qtcreator-extension-docker.webp)

*A Docker device based on an Ubuntu image*

## Features

- Select an image from your local Docker installation to create a device
- Detect the compilers, debuggers, CMake, and Qt versions in the image
  automatically
- Build your application in the container with the toolchain of the image
- Run and debug the application in the container
- Configure the container with mounted paths, network, environment
  variables, and additional arguments for Docker

## You also need:

- The Docker CLI, installed and configured on the development host

We recommend CMake for building applications in Docker containers.
