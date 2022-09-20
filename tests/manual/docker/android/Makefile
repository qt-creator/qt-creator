qt-android-sdk-31: Dockerfile-qt-6.3.2-android-ubuntu-22.04
	docker build --platform linux/x86_64 . -f Dockerfile-qt-6.3.2-android-ubuntu-22.04 -t qt-android-sdk:31 -t qt-android-sdk

qt-android-sdk-31-run: qt-android-sdk-31
	docker run --platform linux/x86_64 -it --rm qt-android-sdk:31 /bin/bash
