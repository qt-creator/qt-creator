    // Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "export.h"

#include "ffmpegutils.h"
#include "screenrecordersettings.h"
#include "screenrecordertr.h"

#include <utils/algorithm.h>
#include <utils/fileutils.h>
#include <utils/layoutbuilder.h>
#include <utils/process.h>
#include <utils/styledbar.h>
#include <utils/utilsicons.h>

#include <coreplugin/progressmanager/futureprogress.h>
#include <coreplugin/progressmanager/progressmanager.h>

#include <QFutureWatcher>
#include <QToolButton>

using namespace Utils;

namespace ScreenRecorder {

const char screenRecordingExportId[] = "ScreenRecorder::screenRecordingExportTask";

static const QVector<ExportWidget::Format> &formats()
{
    static const QVector<ExportWidget::Format> result = {
        {
            ExportWidget::Format::AnimatedImage,
            ExportWidget::Format::Lossy,
            "GIF",
            ".gif",
            {
            },
        },
        {
            ExportWidget::Format::AnimatedImage,
            ExportWidget::Format::Lossless,
            "WebP",
            ".webp",
            {
                "-lossless", "1",
                "-compression_level", "6",
                "-qscale", "100",
            },
        },
        {
            ExportWidget::Format::AnimatedImage,
            ExportWidget::Format::Lossy,
            "WebP/VP8",
            ".webp",
            {
                "-pix_fmt", "yuv420p",
                "-compression_level", "6",
            },
        },
        {
            ExportWidget::Format::Video,
            ExportWidget::Format::Lossy,
            "MP4/H.264",
            ".mp4",
            {
                "-pix_fmt", "yuv420p", // 4:2:0 chroma subsampling for Firefox compatibility
                "-codec", "libx264",
                "-preset", "veryslow",
                "-level", "5.2",
                "-tune", "animation",
                "-movflags", "+faststart",
            },
        },
        {
            ExportWidget::Format::Video,
            ExportWidget::Format::Lossy,
            "WebM/VP9",
            ".webm",
            {
                "-pix_fmt", "yuv420p",
                "-codec", "libvpx-vp9",
                "-crf", "36", // Creates slightly smaller files than the "MP4/H.264" preset
                "-deadline", "best",
                "-row-mt", "1",
            },
        },
        {
            ExportWidget::Format::AnimatedImage,
            ExportWidget::Format::Lossless,
            "avif",
            ".avif",
            {
                "-lossless", "1",
            },
        },
        {
            ExportWidget::Format::Video,
            ExportWidget::Format::Lossy,
            "WebM/AV1",
            ".webm",
            {
                "-pix_fmt", "yuv422p",
                "-codec", "libaom-av1",
            },
        },
        {
            ExportWidget::Format::Video,
            ExportWidget::Format::Lossless,
            "Mov/qtrle",
            ".mov",
            {
                "-codec", "qtrle",
            },
        },
    };
    return result;
}

static QString fileDialogFilters()
{
    return transform(formats(), [] (const ExportWidget::Format &fp) {
               return fp.fileDialogFilter();
           }).join(";;");
}

QString ExportWidget::Format::fileDialogFilter() const
{
    return displayName
           + " - " + (kind == Video ? Tr::tr("Video") : Tr::tr("Animated image"))
           + " - " + (compression == Lossy ? Tr::tr("Lossy") : Tr::tr("Lossless"))
           + " (*" + fileExtension + ")";
}

ExportWidget::ExportWidget(QWidget *parent)
    : StyledBar(parent)
    , m_trimRange({-1, -1})
{
    m_process = new Process(this);
    m_process->setUseCtrlCStub(true);
    m_process->setProcessMode(ProcessMode::Writer);

    auto exportButton = new QToolButton;
    exportButton->setText(Tr::tr("Export..."));

    using namespace Layouting;
    Row { st, new StyledSeparator, exportButton, noMargin(), spacing(0) }.attachTo(this);

    connect(exportButton, &QToolButton::clicked, this, [this] {
        FilePathAspect &lastDir = Internal::settings().exportLastDirectory;
        StringAspect &lastFormat = Internal::settings().exportLastFormat;
        const Format &defaultFormat = formats().at(1);
        QTC_CHECK(defaultFormat.displayName == lastFormat.defaultValue());
        QString selectedFilter = findOr(formats(), defaultFormat,
                                        [&lastFormat] (const Format &f) {
                                            return f.displayName == lastFormat();
                                        }).fileDialogFilter();
        FilePath file = FileUtils::getSaveFilePath(nullptr, Tr::tr("Save As"), lastDir(),
                                                   fileDialogFilters(), &selectedFilter);
        if (!file.isEmpty()) {
            m_currentFormat = findOr(formats(), defaultFormat,
                                     [&selectedFilter] (const Format &fp) {
                                         return fp.fileDialogFilter() == selectedFilter;
                                     });
            if (!file.endsWith(m_currentFormat.fileExtension))
                file = file.stringAppended(m_currentFormat.fileExtension);
            m_outputClipInfo.file = file;
            lastDir.setValue(file.parentDir());
            lastDir.writeToSettingsImmediatly();
            lastFormat.setValue(m_currentFormat.displayName);
            lastFormat.writeToSettingsImmediatly();
            startExport();
        }
    });
    connect(m_process, &Process::started, this, [this] {
        emit started();
    });
    connect(m_process, &Process::done, this, [this] {
        m_futureInterface->reportFinished();
        if (m_process->exitCode() == 0) {
            emit finished(m_outputClipInfo.file);
        } else {
            FFmpegUtils::reportError(m_process->commandLine(), m_lastOutputChunk);
            emit finished({});
        }
    });
    connect(m_process, &Process::readyReadStandardError, this, [this] {
        m_lastOutputChunk = m_process->readAllRawStandardError();
        const int frameProgress = FFmpegUtils::parseFrameProgressFromOutput(m_lastOutputChunk);
        if (frameProgress >= 0)
            m_futureInterface->setProgressValue(frameProgress);
    });
}

ExportWidget::~ExportWidget()
{
    interruptExport();
}

void ExportWidget::startExport()
{
    m_futureInterface.reset(new QFutureInterface<void>);
    m_futureInterface->setProgressRange(0, m_trimRange.second - m_trimRange.first);
    Core::ProgressManager::addTask(m_futureInterface->future(),
                                   Tr::tr("Exporting Screen Recording"), screenRecordingExportId);
    m_futureInterface->setProgressValue(0);
    m_futureInterface->reportStarted();
    const auto watcher = new QFutureWatcher<void>(this);
    connect(watcher, &QFutureWatcher<void>::canceled, this, &ExportWidget::interruptExport);
    connect(watcher, &QFutureWatcher<void>::finished, this, [watcher] {
        watcher->disconnect();
        watcher->deleteLater();
    });
    watcher->setFuture(m_futureInterface->future());

    m_process->close();
    const CommandLine cl(Internal::settings().ffmpegTool(), ffmpegExportParameters());
    m_process->setCommand(cl);
    m_process->setWorkingDirectory(Internal::settings().ffmpegTool().parentDir());
    FFmpegUtils::logFfmpegCall(cl);
    m_process->start();
}

void ExportWidget::interruptExport()
{
    FFmpegUtils::killFfmpegProcess(m_process);
}

QStringList ExportWidget::ffmpegExportParameters() const
{
    const bool isGif = m_currentFormat.fileExtension == ".gif";
    const QString trimFilter =
        !m_inputClipInfo.isCompleteRange(m_trimRange)
            ? QString("[v:0]trim=start=%1:end=%2[trimmed]"
                      ";[trimmed]setpts=PTS-STARTPTS[setpts]")
                  .arg(m_inputClipInfo.secondForFrame(m_trimRange.first))
                  .arg(m_inputClipInfo.secondForFrame(m_trimRange.second))
            : QString("[v:0]null[setpts]");

    const QString cropFilter =
        !m_inputClipInfo.isCompleteArea(m_cropRect)
            ? QString(";[setpts]crop=w=%3:h=%4:x=%5:y=%6[cropped]")
                  .arg(m_cropRect.width()).arg(m_cropRect.height())
                  .arg(m_cropRect.left()).arg(m_cropRect.top())
            : QString(";[setpts]null[cropped]");

    const QString extraFilter =
        isGif
            ? QString(";[cropped]split[cropped1][cropped2]"
                      ";[cropped1]palettegen="
                      "reserve_transparent=false"
                      ":max_colors=256[pal]"
                      ";[cropped2][pal]paletteuse="
                      "diff_mode=rectangle[out]")
            : QString(";[cropped]null[out]");

    QStringList loop;
    if (m_currentFormat.kind == Format::AnimatedImage) {
        const bool doLoop = Internal::settings().animatedImagesAsEndlessLoop();
        // GIF muxer take different values for "don't loop" than WebP and avif muxer
        const QLatin1String dontLoopParam(isGif ? "-1" : "1");
        loop.append({"-loop", doLoop ? QLatin1String("0") : dontLoopParam});
    }

    const QStringList args =
        QStringList {
            "-y",
            "-v", "error",
            "-stats",
            "-stats_period", "0.25",
            "-i", m_inputClipInfo.file.toString(),
        }
        << "-filter_complex" << trimFilter + cropFilter + extraFilter << "-map" << "[out]"
        << m_currentFormat.encodingParameters
        << loop
        << m_outputClipInfo.file.toString();

    return args;
}

void ExportWidget::setClip(const ClipInfo &clip)
{
    if (!qFuzzyCompare(clip.duration, m_inputClipInfo.duration))
        m_trimRange = {0, clip.framesCount()};
    if (clip.dimensions != m_inputClipInfo.dimensions)
        m_cropRect = {QPoint(), clip.dimensions};
    m_inputClipInfo = clip;
}

void ExportWidget::setCropRect(const QRect &rect)
{
    m_cropRect = rect;
}

void ExportWidget::setTrimRange(FrameRange range)
{
    m_trimRange = range;
}

} // namespace ScreenRecorder
