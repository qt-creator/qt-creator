// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/aspects.h>

namespace ScreenRecorder::Internal {

enum CaptureType {
    X11grab,
    Ddagrab,
    Gdigrab,
    AVFoundation,
};

class ScreenRecorderSettings : public Utils::AspectContainer
{
public:
    ScreenRecorderSettings();

    bool toolsRegistered() const;

    struct RecordSettings {
        const int screenId;
        const QRect cropRect;
        const int frameRate;
    };
    static RecordSettings sanitizedRecordSettings(const RecordSettings &settings);
    RecordSettings recordSettings() const;
    void applyRecordSettings(const RecordSettings &settings);
    CaptureType volatileScreenCaptureType() const;

    // Visible in Settings page
    Utils::FilePathAspect ffmpegTool{this};
    Utils::FilePathAspect ffprobeTool{this};
    Utils::SelectionAspect screenCaptureType{this};
    Utils::BoolAspect captureCursor{this};
    Utils::BoolAspect captureMouseClicks{this};
    Utils::BoolAspect enableFileSizeLimit{this};
    Utils::IntegerAspect fileSizeLimit{this}; // in MB
    Utils::BoolAspect enableRtBuffer{this};
    Utils::IntegerAspect rtBufferSize{this}; // in MB
    Utils::BoolAspect logFfmpegCommandline{this};
    Utils::BoolAspect animatedImagesAsEndlessLoop{this};

    // Used in other places
    Utils::FilePathAspect lastOpenDirectory{this};
    Utils::FilePathAspect exportLastDirectory{this};
    Utils::StringAspect exportLastFormat{this};
    Utils::FilePathAspect lastSaveImageDirectory{this};
    Utils::IntegerAspect recordFrameRate{this};
    Utils::IntegerAspect recordScreenId{this};
    Utils::StringListAspect recordScreenCropRect{this};
};

ScreenRecorderSettings &settings();

} // ScreenRecorder::Internal
