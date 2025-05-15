all: help

help:
	@grep -E '^[0-9a-zA-Z._-]+:.*?## .*$$' $(MAKEFILE_LIST) | awk 'BEGIN {FS = ":.*?## "}; {printf "\033[36m%-30s\033[0m %s\n", $$1, $$2}'

default_run_args = --env="DISPLAY=host.docker.internal:0" --cap-add=SYS_PTRACE --security-opt seccomp=unconfined -v $(HOME):$(HOME) --workdir=$(HOME)

base: Dockerfile-base ## Build Ubuntu 25.10, docker-cli image, build Qt manually
	docker build . -f Dockerfile-base -t qt-linux-base -t qt-linux-base:6

base-run: ## Build & run the base image
	docker run -it --rm -v /var/run/docker.sock:/var/run/docker.sock $(default_run_args) qt-linux-base /bin/bash

qt-linux-x86: Dockerfile ## Build Ubuntu 25.10, Qt 6, docker-cli image (For x86_64)
	docker build . --platform=linux/amd64 -t qt-linux-x86_64 -t qt-linux-x86_64:6

qt-linux: Dockerfile ## Build Ubuntu 25.10, Qt 6, docker-cli image
	docker build . -t qt-linux -t qt-linux:6

qt-linux-run: qt-linux ## Build & run the image
	docker run -it --rm -v /var/run/docker.sock:/var/run/docker.sock $(default_run_args) qt-linux:6 /bin/bash

qt-linux-with-qtc: DockerFile-with-creator ## Build Ubuntu 25.10, Qt 6, docker-cli image, Includes Qt Creator
	docker build . -f DockerFile-with-creator -t qt-linux-qtc -t qt-linux-qtc:6

qt-linux-with-qtc-run: qt-linux-with-qtc ## Build & run the image, Includes Qt Creator
	docker run -it --rm  $(default_run_args) qt-linux-qtc:6 /bin/bash	

qt-linux-with-llvm: Dockerfile-llvm ## Build Ubuntu 25.10, Qt 6, docker-cli image, With Clang
	docker build . -f Dockerfile-llvm -t qt-linux-llvm -t qt-linux-llvm:6

qt-linux-with-llvm-run: qt-linux-with-llvm ## Build & run the image, With Clang
	docker run -it --rm  $(default_run_args) qt-linux-llvm:6 /bin/bash

qt-linux-with-llvm-conan: Dockerfile-llvm-conan ## Build Ubuntu 25.10, Qt 6, docker-cli image, With Clang, Conan
	docker build . -f Dockerfile-llvm-conan -t qt-linux-llvm-conan -t qt-linux-llvm-conan:6

qt-linux-with-llvm-conan-run: qt-linux-with-llvm-conan ## Build & run the image, With Clang, Conan
	docker run -it --rm  $(default_run_args) qt-linux-llvm-conan:6 /bin/bash

qt-linux-with-llvm-vcpkg: Dockerfile-llvm-vcpkg ## Build Ubuntu 25.10, Qt 6, docker-cli image, With Clang, Vcpkg
	docker build . -f Dockerfile-llvm-vcpkg -t qt-linux-llvm-vcpkg -t qt-linux-llvm-vcpkg:6

qt-linux-with-llvm-vcpkg-run: qt-linux-with-llvm-vcpkg ## Build & run the image, With Clang, Vcpkg
	docker run -it --rm  $(default_run_args) qt-linux-llvm-vcpkg:6 /bin/bash

qt-linux-with-gcc-10: Dockerfile-gcc-10 ## Build Ubuntu 25.10, Qt 6, docker-cli image, With GCC 10
	docker build . -f Dockerfile-gcc-10 -t qt-linux-gcc-10 -t qt-linux-gcc-10:6
qt-linux-with-gcc-10-run: qt-linux-with-gcc-10 ## Build & run the image, With GCC 10
	docker run -it --rm  $(default_run_args) qt-linux-gcc-10:6 /bin/bash

pyside6-ubuntu: Dockerfile-pyside6-ubuntu ## Build Ubuntu latest, PySide6 image
	docker build . -f Dockerfile-pyside6-ubuntu -t pyside6-ubuntu

qt-5-ubuntu-16.04-build: Dockerfile-qt-5-ubuntu-16.04-build ## Build-Image, Ubuntu 16.04, Qt 5 image
	docker build . -f Dockerfile-qt-5-ubuntu-16.04-build -t qt-5-ubuntu-16.04-build

qt-5-ubuntu-20.04-build: Dockerfile-qt-5-ubuntu-20.04-build ## Build-Image, Ubuntu 20.04, Qt 5 image
	docker build . -f Dockerfile-qt-5-ubuntu-20.04-build -t qt-5-ubuntu-20.04-build

qt-5-ubuntu-20.04-run: Dockerfile-qt-5-ubuntu-20.04-run ## Run-Image, Ubuntu 20.04, Qt 5 image
	docker build . -f Dockerfile-qt-5-ubuntu-20.04-run -t qt-5-ubuntu-20.04-run

qt-5-ubuntu-20.04-clang-lldb-build: Dockerfile-qt-5-ubuntu-20.04-clang-lldb-build ## Build-Image, Ubuntu 20.04, Qt 5 image, with Clang and LLDB
	docker build . -f Dockerfile-qt-5-ubuntu-20.04-clang-lldb-build -t qt-5-ubuntu-20.04-clang-lldb-build

qt-6-fedora-37-build: Dockerfile-qt-6-fedora-37-build ## Build-Image, Fedora 37, Qt 6 image
	docker build . -f Dockerfile-qt-6-fedora-37-build -t qt-6-fedora-37-build

qt-6-ubuntu-24.04-build: Dockerfile-qt-6-ubuntu-24.04-build ## Build-Image, Ubuntu 24.04, Qt 6 image, With qbs
	docker build . -f Dockerfile-qt-6-ubuntu-24.04-build -t qt-6-ubuntu-24.04-build

build: qt-5-ubuntu-16.04-build qt-5-ubuntu-20.04-build qt-5-ubuntu-20.04-run qt-5-ubuntu-20.04-clang-lldb-build qt-6-fedora-37-build qt-6-ubuntu-24.04-build ## Old build.sh targets
