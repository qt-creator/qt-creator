// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

namespace ScreenRecorder::Constants {

inline constexpr char TOOLSSETTINGSPAGE_ID[] = "Z.ScreenRecorder";
inline constexpr char LOGGING_CATEGORY[] = "qtc.screenrecorder";
inline constexpr char ACTION_ID[] = "ScreenRecorder.Action";
inline constexpr char FFMPEG_COMMAND[] = "ffmpeg";
inline constexpr char FFPROBE_COMMAND[] = "ffprobe";
inline constexpr char FFMPEG_DOWNLOAD_URL[] = "https://ffmpeg.org/download.html";
inline constexpr char GSTLAUNCH_COMMAND[] = "gst-launch-1.0";
inline constexpr char GSTREAMER_INSTALL_COMMAND[] =
    "sudo apt install gstreamer1.0-tools gstreamer1.0-pipewire gstreamer1.0-plugins-base";
inline constexpr int MIN_RECORD_FRAME_RATE = 1;
inline constexpr int MAX_RECORD_FRAME_RATE = 60;

} // namespace ScreenRecorder::Constants
