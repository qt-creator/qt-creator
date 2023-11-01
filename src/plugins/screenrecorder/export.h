// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "ffmpegutils.h"

#include <utils/styledbar.h>

#include <QFutureInterface>

namespace Utils {
class Process;
}

namespace ScreenRecorder {

class CropSizeWarningIcon;

class ExportWidget : public Utils::StyledBar
{
    Q_OBJECT

public:
    struct Format {
        enum Kind {
            AnimatedImage,
            Video,
        } kind;

        enum Compression {
            Lossy,
            Lossless,
        } compression;

        QString displayName;
        QString fileExtension;
        QStringList encodingParameters;

        QString fileDialogFilter() const;
    };

    explicit ExportWidget(QWidget *parent = nullptr);
    ~ExportWidget();

    void setClip(const ClipInfo &clip);
    void setCropRect(const QRect &rect);
    void setTrimRange(FrameRange range);

signals:
    void started();
    void finished(const Utils::FilePath &clip);

private:
    void startExport();
    void interruptExport();
    QStringList ffmpegExportParameters() const;

    ClipInfo m_inputClipInfo;
    ClipInfo m_outputClipInfo;
    Format m_currentFormat;
    Utils::Process *m_process;
    QByteArray m_lastOutputChunk;
    std::unique_ptr<QFutureInterface<void>> m_futureInterface;

    QRect m_cropRect;
    FrameRange m_trimRange;
};

} // namespace ScreenRecorder
