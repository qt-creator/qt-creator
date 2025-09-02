all: help

help:
	@grep -E '^[0-9a-zA-Z._-]+:.*?## .*$$' $(MAKEFILE_LIST) | awk 'BEGIN {FS = ":.*?## "}; {printf "\033[36m%-30s\033[0m %s\n", $$1, $$2}'

default_run_args = --env="DISPLAY=host.docker.internal:0" --cap-add=SYS_PTRACE --security-opt seccomp=unconfined -v $(HOME):$(HOME) --workdir=$(HOME)

base: Dockerfile-base ## Build Ubuntu 22.04, docker-cli image, build Qt manually
	docker build . -f Dockerfile-base -t qt-linux-base -t qt-linux-base:6

base-run: ## Build & run the base image
	docker run -it --rm -v /var/run/docker.sock:/var/run/docker.sock $(default_run_args) qt-linux-base /bin/bash

qt-linux: Dockerfile ## Build Ubuntu 22.04, Qt 6, docker-cli image
	docker build . -t qt-linux -t qt-linux:6

qt-linux-run: qt-linux ## Build & run the image
	docker run -it --rm -v /var/run/docker.sock:/var/run/docker.sock $(default_run_args) qt-linux:6 /bin/bash

qt-linux-with-qtc: DockerFile-with-creator ## Build Ubuntu 22.04, Qt 6, docker-cli image, Includes Qt Creator
	docker build . -f DockerFile-with-creator -t qt-linux-qtc -t qt-linux-qtc:6

qt-linux-with-qtc-run: qt-linux-with-qtc ## Build & run the image, Includes Qt Creator
	docker run -it --rm  $(default_run_args) qt-linux-qtc:6 /bin/bash	

qt-linux-with-llvm: Dockerfile-llvm ## Build Ubuntu 22.04, Qt 6, docker-cli image, With Clang
	docker build . -f Dockerfile-llvm -t qt-linux-llvm -t qt-linux-llvm:6

qt-linux-with-llvm-run: qt-linux-with-llvm ## Build & run the image, With Clang
	docker run -it --rm  $(default_run_args) qt-linux-llvm:6 /bin/bash

qt-linux-with-llvm-conan: Dockerfile-llvm-conan ## Build Ubuntu 22.04, Qt 6, docker-cli image, With Clang, Conan
	docker build . -f Dockerfile-llvm-conan -t qt-linux-llvm-conan -t qt-linux-llvm-conan:6

qt-linux-with-llvm-conan-run: qt-linux-with-llvm-conan ## Build & run the image, With Clang, Conan
	docker run -it --rm  $(default_run_args) qt-linux-llvm-conan:6 /bin/bash

qt-linux-with-llvm-vcpkg: Dockerfile-llvm-vcpkg ## Build Ubuntu 22.04, Qt 6, docker-cli image, With Clang, Vcpkg
	docker build . -f Dockerfile-llvm-vcpkg -t qt-linux-llvm-vcpkg -t qt-linux-llvm-vcpkg:6

qt-linux-with-llvm-vcpkg-run: qt-linux-with-llvm-vcpkg ## Build & run the image, With Clang, Vcpkg
	docker run -it --rm  $(default_run_args) qt-linux-llvm-vcpkg:6 /bin/bash

qt-linux-with-gcc-10: Dockerfile-gcc-10 ## Build Ubuntu 22.04, Qt 6, docker-cli image, With GCC 10
	docker build . -f Dockerfile-gcc-10 -t qt-linux-gcc-10 -t qt-linux-gcc-10:6
qt-linux-with-gcc-10-run: qt-linux-with-gcc-10 ## Build & run the image, With GCC 10
	docker run -it --rm  $(default_run_args) qt-linux-gcc-10:6 /bin/bash
