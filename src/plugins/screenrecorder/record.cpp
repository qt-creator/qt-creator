// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "record.h"

#include "cropandtrim.h"
#include "ffmpegutils.h"
#include "screenrecordersettings.h"
#include "screenrecordertr.h"

#include <utils/environment.h>
#include <utils/fileutils.h>
#include <utils/layoutbuilder.h>
#include <utils/process.h>
#include <utils/qtcsettings.h>
#include <utils/styledbar.h>
#include <utils/utilsicons.h>

#include <coreplugin/icore.h>

#include <QAction>
#include <QDialogButtonBox>
#include <QGuiApplication>
#include <QLoggingCategory>
#include <QMessageBox>
#include <QScreen>
#include <QTimer>
#include <QToolButton>

using namespace Utils;

namespace ScreenRecorder {

using namespace  Internal;

struct RecordPreset {
    const QString fileExtension;
    const QStringList encodingParameters;
};

static const RecordPreset &recordPreset()
{
    static const RecordPreset preset = {
        ".mkv",
        {
            "-vcodec", "libx264rgb",
            "-crf", "0",
            "-preset", "ultrafast",
            "-tune", "zerolatency",
            "-reserve_index_space", "1M",
        }
    };
    // Valid alternatives:
    // ".mov", { "-vcodec", "qtrle" } // Slower encoding, faster seeking
    return preset;
}

class RecordOptionsDialog : public QDialog
{
public:
    explicit RecordOptionsDialog(QWidget *parent = nullptr);

private:
    QRect screenCropRect() const;
    void updateCropScene();
    void updateWidgets();

    static const int m_factor = 4;
    CropScene *m_cropScene;
    QImage m_thumbnail;
    SelectionAspect m_screenId;
    IntegerAspect m_recordFrameRate;
    QLabel *m_cropRectLabel;
    QToolButton *m_resetButton;
};

static QString optionNameForScreen(const QScreen *screen)
{
    const QSize pixelSize = screen->size() * screen->devicePixelRatio();
    QStringList nameElements = { screen->name(), screen->manufacturer(), screen->model() };
    nameElements.removeDuplicates(); // Model and name might be the same on some system
    const QString displayName = QLatin1String("%1 - %2x%3").arg(nameElements.join(" "))
                                    .arg(pixelSize.width()).arg(pixelSize.height());
    return displayName;
}

RecordOptionsDialog::RecordOptionsDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(Tr::tr("Screen Recording Options"));

    m_screenId.setLabelText(Tr::tr("Display:"));
    m_screenId.setDisplayStyle(SelectionAspect::DisplayStyle::ComboBox);
    for (const QScreen *screen : QGuiApplication::screens())
        m_screenId.addOption(optionNameForScreen(screen));

    m_cropScene = new CropScene;

    m_recordFrameRate.setLabelText(Tr::tr("FPS:"));

    m_resetButton = new QToolButton;
    m_resetButton->setIcon(Icons::RESET.icon());

    m_cropRectLabel = new QLabel;
    m_cropRectLabel->setAlignment(Qt::AlignVCenter | Qt::AlignRight);

    auto buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

    using namespace Layouting;
    Column {
        Row { m_screenId, st },
        Group {
            title(Tr::tr("Recorded screen area:")),
            Column {
                m_cropScene,
                Row { st, m_cropRectLabel, m_resetButton },
            }
        },
        Row { m_recordFrameRate, st },
        st,
        buttonBox,
    }.attachTo(this);
    layout()->setSizeConstraint(QLayout::SetFixedSize);

    connect(buttonBox, &QDialogButtonBox::accepted, this, [this] {
        const QRect cropRect = m_cropScene->fullySelected() ? QRect() : screenCropRect();
        settings().applyRecordSettings({int(m_screenId()), cropRect, int(m_recordFrameRate())});
        QDialog::accept();
    });
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(&m_screenId, &IntegerAspect::changed, this, [this] {
        updateCropScene();
        m_cropScene->setFullySelected();
    });
    connect(m_resetButton, &QToolButton::pressed, this, [this](){
        m_cropScene->setFullySelected();
    });
    connect(m_cropScene, &CropScene::cropRectChanged, this, &RecordOptionsDialog::updateWidgets);

    updateCropScene();

    const ScreenRecorderSettings::RecordSettings rs = settings().recordSettings();
    m_screenId.setValue(rs.screenId);
    if (!rs.cropRect.isNull()) {
        m_cropScene->setCropRect({rs.cropRect.x() / m_factor,
                                  rs.cropRect.y() / m_factor,
                                  rs.cropRect.width() / m_factor,
                                  rs.cropRect.height() / m_factor});
    } else {
        m_cropScene->setFullySelected();
    }
    m_recordFrameRate.setValue(rs.frameRate);
}

QRect RecordOptionsDialog::screenCropRect() const
{
    const QRect r = m_cropScene->cropRect();
    return {r.x() * m_factor, r.y() * m_factor, r.width() * m_factor, r.height() * m_factor};
}

void RecordOptionsDialog::updateCropScene()
{
    const ScreenRecorderSettings::RecordSettings rs = ScreenRecorderSettings
        ::sanitizedRecordSettings({int(m_screenId()), screenCropRect(), int(m_recordFrameRate())});
    const QList<QScreen*> screens = QGuiApplication::screens();
    m_thumbnail = QGuiApplication::screens().at(rs.screenId)->grabWindow().toImage();
    const qreal dpr = m_thumbnail.devicePixelRatio();
    m_thumbnail = m_thumbnail.scaled((m_thumbnail.deviceIndependentSize() / m_factor * dpr)
                                         .toSize(),
                                     Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    m_thumbnail.setDevicePixelRatio(dpr);
    m_cropScene->setImage(m_thumbnail);
    const static int lw = CropScene::lineWidth;
    m_cropScene->setFixedSize(m_thumbnail.deviceIndependentSize().toSize()
                                  .grownBy({lw, lw, lw, lw}));
    QTimer::singleShot(250, this, [this] {
        updateCropScene();
    });
    updateWidgets();
}

void RecordOptionsDialog::updateWidgets()
{
    const QRect r = screenCropRect();
    m_cropRectLabel->setText(QString("x:%1 y:%2 w:%3 h:%4")
                                 .arg(r.x()).arg(r.y()).arg(r.width()).arg(r.height()));
    m_resetButton->setEnabled(!m_cropScene->fullySelected());
}

RecordWidget::RecordWidget(const FilePath &recordFile, QWidget *parent)
    : StyledBar(parent)
    , m_recordFile(recordFile)
{
    setMinimumWidth(220);

    m_process = new Process(this);
    m_process->setUseCtrlCStub(true);
    m_process->setProcessMode(ProcessMode::Writer);

    auto settingsButton = new QToolButton;
    settingsButton->setIcon(Icons::SETTINGS_TOOLBAR.icon());

    auto recordButton = new QToolButton;
    recordButton->setIcon(Icon({{":/utils/images/filledcircle.png",
                                 Theme::IconsStopToolBarColor}}).icon());

    auto stopButton = new QToolButton;
    stopButton->setIcon(Icon({{":/utils/images/stop_small.png",
                               Theme::IconsStopToolBarColor}}).icon());
    stopButton->setEnabled(false);

    auto progressLabel = new TimeLabel(m_clipInfo);
    progressLabel->setEnabled(false);
    progressLabel->setAlignment(Qt::AlignVCenter | Qt::AlignRight);

    m_openClipAction = new QAction(Tr::tr("Open Mov/qtrle rgb24 File"), this);
    addAction(m_openClipAction);
    setContextMenuPolicy(Qt::ActionsContextMenu);

    using namespace Layouting;
    Row {
        settingsButton,
        recordButton,
        stopButton,
        st,
        progressLabel,
        Space(6),
        noMargin(), spacing(0),
    }.attachTo(this);

    connect(settingsButton, &QToolButton::clicked, this, [this] {
        m_optionsDialog = new RecordOptionsDialog(this);
        m_optionsDialog->setWindowModality(Qt::WindowModal);
        m_optionsDialog->setAttribute(Qt::WA_DeleteOnClose);
        m_optionsDialog->show();
    });
    connect(recordButton, &QToolButton::clicked, this, [this, progressLabel] {
        m_clipInfo.duration = 0;
        progressLabel->setFrame(0);
        m_clipInfo = {};
        m_clipInfo.file = m_recordFile;
        m_clipInfo.rFrameRate = qreal(Internal::settings().recordFrameRate());
        const CommandLine cl(Internal::settings().ffmpegTool(), ffmpegParameters(m_clipInfo));
        m_process->setCommand(cl);
        m_process->setWorkingDirectory(Internal::settings().ffmpegTool().parentDir());
        FFmpegUtils::logFfmpegCall(cl);
        m_process->start();
    });
    connect(stopButton, &QToolButton::clicked, this, [this] {
        FFmpegUtils::sendQuitCommand(m_process);
    });
    connect(m_process, &Process::started, this, [=] {
        progressLabel->setEnabled(true);
        recordButton->setEnabled(false);
        stopButton->setEnabled(true);
        settingsButton->setEnabled(false);
        m_openClipAction->setEnabled(false);
        emit started();
    });
    connect(m_process, &Process::done, this, [=] {
        recordButton->setEnabled(true);
        stopButton->setEnabled(false);
        settingsButton->setEnabled(true);
        m_openClipAction->setEnabled(true);
        if (m_process->exitCode() == 0)
            emit finished(FFmpegUtils::clipInfo(m_clipInfo.file));
        else
            FFmpegUtils::reportError(m_process->commandLine(), m_lastOutputChunk);
    });
    connect(m_process, &Process::readyReadStandardError, this, [this, progressLabel] {
        m_lastOutputChunk = m_process->readAllRawStandardError();
        const int frameProgress = FFmpegUtils::parseFrameProgressFromOutput(m_lastOutputChunk);
        if (frameProgress > 0) {
            m_clipInfo.duration = m_clipInfo.secondForFrame(frameProgress);
            progressLabel->setFrame(m_clipInfo.framesCount());
        }
    });
    connect(m_openClipAction, &QAction::triggered, this, [this, progressLabel] {
        const FilePath lastDir = Internal::settings().lastOpenDirectory();
        const FilePath file = FileUtils::getOpenFilePath(Core::ICore::dialogParent(),
                                                         m_openClipAction->text(), lastDir,
                                                         "Mov/qtrle rgb24 (*.mov)");
        if (!file.isEmpty()) {
            Internal::settings().lastOpenDirectory.setValue(file.parentDir());
            Internal::settings().lastOpenDirectory.apply();
            Internal::settings().lastOpenDirectory.writeToSettingsImmediatly();
            const ClipInfo clip = FFmpegUtils::clipInfo(file);
            if (clip.isNull()) {
                QMessageBox::critical(Core::ICore::dialogParent(),
                                      Tr::tr("Cannot Open Clip"),
                                      Tr::tr("FFmpeg cannot open %1.").arg(file.toUserOutput()));
            } else if (!clip.isLossless()) {
                QMessageBox::critical(Core::ICore::dialogParent(),
                                      Tr::tr("Clip Not Supported"),
                                      Tr::tr("Choose a clip with the \"qtrle\" codec and "
                                             "pixel format \"rgb24\"."));
            } else {
                m_clipInfo.duration = 0;
                progressLabel->setFrame(0);
                progressLabel->setEnabled(false);
                emit finished(clip);
            }
        }
    });
}

RecordWidget::~RecordWidget()
{
    FFmpegUtils::killFfmpegProcess(m_process);
}

QString RecordWidget::recordFileExtension()
{
    return recordPreset().fileExtension;
}

static QString sizeStr(const QSize &size)
{
    return QString("%1x%2").arg(size.width()).arg(size.height());
}

static QRect cropRectForContinuousMulitScreen(
    const Internal::ScreenRecorderSettings::RecordSettings &rS)
{
    const QScreen *screen = QGuiApplication::screens()[rS.screenId];
    const QPoint screenTopLeft = screen->geometry().topLeft();
    const QPoint cropTopLeft = rS.cropRect.translated(screenTopLeft).topLeft();
    const QSize cropSize = rS.cropRect.isNull()
                               ? screen->size() * screen->devicePixelRatio()
                               : rS.cropRect.size();
    const QRect cropRect(cropTopLeft, cropSize);
    return cropRect;
}

QStringList RecordWidget::ffmpegParameters(const ClipInfo &clipInfo) const
{
    const Internal::ScreenRecorderSettings::RecordSettings rS =
        Internal::settings().recordSettings();
    const QString frameRateStr = QString::number(rS.frameRate);
    const QString screenIdStr = QString::number(rS.screenId);
    const QString captureCursorStr = Internal::settings().captureCursor() ? "1" : "0";
    QStringList videoGrabParams;

    switch (Internal::settings().volatileScreenCaptureType()) {
    case Internal::CaptureType::X11grab: {
        const QRect cropRect = cropRectForContinuousMulitScreen(rS);
        const QString videoSize = sizeStr(cropRect.size());
        const QString x11display = qtcEnvironmentVariable("DISPLAY", ":0.0");
        videoGrabParams.append({"-f", "x11grab"});
        videoGrabParams.append({"-draw_mouse", captureCursorStr});
        videoGrabParams.append({"-framerate", frameRateStr});
        videoGrabParams.append({"-video_size", videoSize});
        videoGrabParams.append({"-i", QString("%1+%2,%3").arg(x11display)
                                          .arg(cropRect.x()).arg(cropRect.y())});
        break;
    }
    case Internal::CaptureType::Ddagrab: {
        QString filter = "ddagrab=output_idx=" + screenIdStr;
        if (!rS.cropRect.isNull()) {
            filter.append(":video_size=" + sizeStr(rS.cropRect.size()));
            filter.append(QString(":offset_x=%1:offset_y=%2").arg(rS.cropRect.x())
                              .arg(rS.cropRect.y()));
        }
        filter.append(":framerate=" + frameRateStr);
        filter.append(":draw_mouse=" + captureCursorStr);
        filter.append(",hwdownload");
        filter.append(",format=bgra");
        videoGrabParams = {
            "-ss", "00:00.25", // Skip few first frames which are black
            "-filter_complex", filter,
        };
        break;
    }
    case Internal::CaptureType::Gdigrab: {
        const QRect cropRect = cropRectForContinuousMulitScreen(rS);
        const QString videoSize = sizeStr(cropRect.size());
        videoGrabParams.append({"-f", "gdigrab"});
        videoGrabParams.append({"-draw_mouse", captureCursorStr});
        videoGrabParams.append({"-framerate", frameRateStr});
        videoGrabParams.append({"-video_size", videoSize});
        videoGrabParams.append({"-offset_x", QString::number(cropRect.x())});
        videoGrabParams.append({"-offset_y", QString::number(cropRect.y())});
        videoGrabParams.append({"-i", "desktop"});
        break;
    }
    case Internal::CaptureType::AVFoundation: {
        videoGrabParams = {
            "-f", "avfoundation",
            "-capture_cursor", captureCursorStr,
            "-capture_mouse_clicks", Internal::settings().captureMouseClicks() ? "1" : "0",
            "-framerate", frameRateStr,
            "-pixel_format", "bgr0",
            "-i", QString("Capture screen %1:none").arg(rS.screenId),
        };
        if (!rS.cropRect.isNull()) {
            videoGrabParams.append({
                "-filter_complex", QString("crop=x=%1:y=%2:w=%3:h=%4")
                    .arg(rS.cropRect.x()).arg(rS.cropRect.y())
                    .arg(rS.cropRect.width()).arg(rS.cropRect.height())
            });
        }
        break;
    }
    default:
        break;
    }

    QStringList args = {
        "-y",
        "-v", "error",
        "-stats",
    };
    args.append(videoGrabParams);
    if (Internal::settings().enableFileSizeLimit())
        args.append({"-fs", QString::number(Internal::settings().fileSizeLimit()) + "M"});
    if (Internal::settings().enableRtBuffer())
        args.append({"-rtbufsize", QString::number(Internal::settings().rtBufferSize()) + "M"});
    args.append(recordPreset().encodingParameters);
    args.append(clipInfo.file.toString());

    return args;
}

} // namespace ScreenRecorder
