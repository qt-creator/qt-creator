// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/filepath.h>

#include <QLabel>
#include <QSize>
#include <QVersionNumber>

QT_BEGIN_NAMESPACE
class QTimer;
QT_END_NAMESPACE

namespace Utils {
class CommandLine;
class FilePath;
class Process;
}

namespace ScreenRecorder {

using FrameRange = std::pair<int, int>;

struct ClipInfo {
    Utils::FilePath file;

    // ffmpeg terminology
    QSize dimensions;
    QString codec;
    qreal duration = -1; // seconds
    qreal rFrameRate = -1; // frames per second
    QString pixFmt;
    int streamIdex = -1;

    int framesCount() const;
    qreal secondForFrame(int frame) const;
    QString timeStamp(int frame) const;
    bool isNull() const;
    bool isCompleteArea(const QRect &rect) const;
    bool isCompleteRange(FrameRange range) const;
    bool isLossless() const;
};

class TimeLabel : public QLabel
{
public:
    explicit TimeLabel(const ClipInfo &clipInfo, QWidget *parent = nullptr);

    void setFrame(int frame);
    int frame() const;

private:
    const ClipInfo &m_clipInfo;
    int m_frame = -1;
};

class CropSizeWarningIcon : public QWidget
{
public:
    enum IconVariant {
        StandardVariant,
        ToolBarVariant,
    };

    explicit CropSizeWarningIcon(IconVariant backgroundType, QWidget *parent = nullptr);

    void setCropSize(const QSize &size);

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void updateVisibility();
    bool needsWarning() const;

    QSize m_cropSize;
    const IconVariant m_iconVariant;
    QTimer *m_updateTimer;
};

namespace FFmpegUtils {

QVersionNumber toolVersion();
ClipInfo clipInfo(const Utils::FilePath &path);
int parseFrameProgressFromOutput(const QByteArray &output);
void sendQuitCommand(Utils::Process *proc);
void killFfmpegProcess(Utils::Process *proc);
void logFfmpegCall(const Utils::CommandLine &cmdLn);
void reportError(const Utils::CommandLine &cmdLn, const QByteArray &error);

} // namespace FFmpegUtils
} // namespace ScreenRecorder
