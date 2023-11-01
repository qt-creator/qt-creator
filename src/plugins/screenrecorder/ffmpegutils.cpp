// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "ffmpegutils.h"

#include "screenrecordersettings.h"
#include "screenrecordertr.h"

#ifdef WITH_TESTS
#include "screenrecorder_test.h"
#include <QTest>
#endif // WITH_TESTS

#include <utils/layoutbuilder.h>
#include <utils/process.h>
#include <utils/utilsicons.h>

#include <coreplugin/messagemanager.h>

#include <QBuffer>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPainter>
#include <QTimer>
#include <QVersionNumber>

using namespace Utils;

namespace ScreenRecorder {

int ClipInfo::framesCount() const
{
    return int(duration * rFrameRate);
}

qreal ClipInfo::secondForFrame(int frame) const
{
    return frame / rFrameRate;
}

QString ClipInfo::timeStamp(int frame) const
{
    const qreal seconds = secondForFrame(frame);
    const QString format = QLatin1String(seconds >= 60 * 60 ? "HH:mm:ss.zzz" : "mm:ss.zzz");
    return QTime::fromMSecsSinceStartOfDay(int(seconds * 1000)).toString(format);
}

bool ClipInfo::isNull() const
{
    return qFuzzyCompare(duration, -1);
}

bool ClipInfo::isCompleteArea(const QRect &rect) const
{
    return rect == QRect(QPoint(), dimensions);
}

bool ClipInfo::isCompleteRange(FrameRange range) const
{
    return (range.first == 0 && (range.second == 0 || range.second == framesCount()));
}

bool ClipInfo::isLossless() const
{
    return codec == "qtrle" && pixFmt == "rgb24";
    // TODO: Find out how to properly determine "lossless" via ffprobe
}

TimeLabel::TimeLabel(const ClipInfo &clipInfo, QWidget *parent)
    : QLabel(parent)
    , m_clipInfo(clipInfo)
{
    setFrame(0);
}

void TimeLabel::setFrame(int frame)
{
    m_frame = frame;
    const QString timeStamp = m_clipInfo.timeStamp(m_frame);
    const int maxFrameDigits = qCeil(log10(double(m_clipInfo.framesCount() + 1)));
    const QString label = QString("<b>%1</b> (%2)")
                              .arg(m_frame, maxFrameDigits, 10, QLatin1Char('0'))
                              .arg(timeStamp);
    setText(label);
}

int TimeLabel::frame() const
{
    return m_frame;
}

constexpr QSize warningIconSize(16, 16);

CropSizeWarningIcon::CropSizeWarningIcon(IconVariant backgroundType, QWidget *parent)
    : QWidget(parent)
    , m_iconVariant(backgroundType)
{
    setMinimumSize(warningIconSize);
    setToolTip(Tr::tr("Width and height are not both divisible by 2. "
                      "The video export for some of the lossy formats will not work."));
    m_updateTimer = new QTimer(this);
    m_updateTimer->setInterval(350);
    m_updateTimer->setSingleShot(true);
    m_updateTimer->callOnTimeout(this, &CropSizeWarningIcon::updateVisibility);
}

void CropSizeWarningIcon::setCropSize(const QSize &size)
{
    m_cropSize = size;
    m_updateTimer->stop();
    if (needsWarning())
        m_updateTimer->start();
    else
        setVisible(false);
}

void CropSizeWarningIcon::paintEvent(QPaintEvent*)
{
    static const QIcon standardIcon = Icons::WARNING.icon();
    static const QIcon toolBarIcon = Icons::WARNING_TOOLBAR.icon();
    QRect iconRect(QPoint(), warningIconSize);
    iconRect.moveCenter(rect().center());
    QPainter p(this);
    (m_iconVariant == StandardVariant ? standardIcon : toolBarIcon).paint(&p, iconRect);
}

void CropSizeWarningIcon::updateVisibility()
{
    setVisible(needsWarning());
}

bool CropSizeWarningIcon::needsWarning() const
{
    return (m_cropSize.width() % 2 == 1) || (m_cropSize.height() % 2 == 1);
}

namespace FFmpegUtils {

static QVersionNumber parseVersionNumber(const QByteArray &toolOutput)
{
    QVersionNumber result;
    const QJsonObject jsonObject = QJsonDocument::fromJson(toolOutput).object();
    if (const QJsonObject program_version = jsonObject.value("program_version").toObject();
        !program_version.isEmpty()) {
        if (const QJsonValue version = program_version.value("version"); !version.isUndefined())
            result = QVersionNumber::fromString(version.toString());
    }
    return result;
}

QVersionNumber toolVersion()
{
    Process proc;
    const CommandLine cl = {
        Internal::settings().ffprobeTool(),
        {
            "-v", "quiet",
            "-print_format", "json",
            "-show_versions",
        }
    };
    proc.setCommand(cl);
    proc.runBlocking();
    const QByteArray output = proc.allRawOutput();
    return parseVersionNumber(output);
}

static ClipInfo parseClipInfo(const QByteArray &toolOutput)
{
    ClipInfo result;
    const QJsonObject jsonObject = QJsonDocument::fromJson(toolOutput).object();
    if (const QJsonArray streams = jsonObject.value("streams").toArray(); !streams.isEmpty()) {
        // With more than 1 video stream, the first one is often just a 1-frame thumbnail
        const int streamIndex = int(qMin(streams.count() - 1, 1));
        const QJsonObject stream = streams.at(streamIndex).toObject();
        if (const QJsonValue index = stream.value("index"); !index.isUndefined())
            result.streamIdex = index.toInt();
        if (const QJsonValue width = stream.value("width"); !width.isUndefined())
            result.dimensions.setWidth(width.toInt());
        if (const QJsonValue height = stream.value("height"); !height.isUndefined())
            result.dimensions.setHeight(height.toInt());
        if (const QJsonValue rFrameRate = stream.value("r_frame_rate"); !rFrameRate.isUndefined()) {
            const QStringList frNumbers = rFrameRate.toString().split('/');
            result.rFrameRate = frNumbers.count() == 2 ? frNumbers.first().toDouble()
                                                             / qMax(1, frNumbers.last().toInt())
                                                       : frNumbers.first().toInt();
        }
        if (const QJsonValue codecName = stream.value("codec_name"); !codecName.isUndefined())
            result.codec = codecName.toString();
        if (const QJsonValue pixFmt = stream.value("pix_fmt"); !pixFmt.isUndefined())
            result.pixFmt = pixFmt.toString();
    }
    if (const QJsonObject format = jsonObject.value("format").toObject(); !format.isEmpty()) {
        if (const QJsonValue duration = format.value("duration"); !duration.isUndefined())
            result.duration = duration.toString().toDouble();
    }
    return result;
}

ClipInfo clipInfo(const FilePath &path)
{
    Process proc;
    const CommandLine cl = {
        Internal::settings().ffprobeTool(),
        {
            "-v", "quiet",
            "-print_format", "json",
            "-show_format",
            "-show_streams",
            "-select_streams", "V",
            path.toUserOutput()
        }
    };
    proc.setCommand(cl);
    proc.runBlocking();
    const QByteArray output = proc.rawStdOut();
    ClipInfo result = parseClipInfo(output);
    result.file = path;
    return result;
}

int parseFrameProgressFromOutput(const QByteArray &output)
{
    static const QRegularExpression re(R"(^frame=\s*(?<frame>\d+))");
    const QRegularExpressionMatch match = re.match(QString::fromUtf8(output));
    if (match.hasMatch())
        if (const QString frame = match.captured("frame"); !frame.isEmpty())
            return frame.toInt();
    return -1;
}

void sendQuitCommand(Process *proc)
{
    if (proc && proc->processMode() == ProcessMode::Writer && proc->isRunning())
        proc->writeRaw("q");
}

void killFfmpegProcess(Process *proc)
{
    sendQuitCommand(proc);
    if (proc->isRunning())
        proc->kill();
}

void reportError(const CommandLine &cmdLn, const QByteArray &error)
{
    if (!Internal::settings().logFfmpegCommandline())
        Core::MessageManager::writeSilently(cmdLn.toUserOutput());
    Core::MessageManager::writeDisrupting("\n" + QString::fromUtf8(error));
}

void logFfmpegCall(const CommandLine &cmdLn)
{
    if (Internal::settings().logFfmpegCommandline())
        Core::MessageManager::writeSilently(cmdLn.toUserOutput());
}

} // namespace FFmpegUtils
} // namespace ScreenRecorder

#ifdef WITH_TESTS

using namespace ScreenRecorder::FFmpegUtils;

namespace ScreenRecorder::Internal {

void FFmpegOutputParserTest::testVersionParser_data()
{
    QTest::addColumn<QByteArray>("ffprobeVersionOutput");
    QTest::addColumn<QVersionNumber>("versionNumber");

    QTest::newRow("4.2.3")
        << QByteArray(
R"_({
    "program_version": {
        "version": "4.4.2-0ubuntu0.22.04.1",
        "copyright": "Copyright (c) 2007-2021 the FFmpeg developers",
        "compiler_ident": "gcc 11 (Ubuntu 11.2.0-19ubuntu1)",
        "configuration": "--prefix=/usr --extra-version=0ubuntu0.22.04.1 --toolchain=hardened --libdir=/usr/lib/x86_64-linux-gnu --incdir=/usr/include/x86_64-linux-gnu --arch=amd64 --enable-gpl --disable-stripping --enable-gnutls --enable-ladspa --enable-libaom --enable-libass --enable-libbluray --enable-libbs2b --enable-libcaca --enable-libcdio --enable-libcodec2 --enable-libdav1d --enable-libflite --enable-libfontconfig --enable-libfreetype --enable-libfribidi --enable-libgme --enable-libgsm --enable-libjack --enable-libmp3lame --enable-libmysofa --enable-libopenjpeg --enable-libopenmpt --enable-libopus --enable-libpulse --enable-librabbitmq --enable-librubberband --enable-libshine --enable-libsnappy --enable-libsoxr --enable-libspeex --enable-libsrt --enable-libssh --enable-libtheora --enable-libtwolame --enable-libvidstab --enable-libvorbis --enable-libvpx --enable-libwebp --enable-libx265 --enable-libxml2 --enable-libxvid --enable-libzimg --enable-libzmq --enable-libzvbi --enable-lv2 --enable-omx --enable-openal --enable-opencl --enable-opengl --enable-sdl2 --enable-pocketsphinx --enable-librsvg --enable-libmfx --enable-libdc1394 --enable-libdrm --enable-libiec61883 --enable-chromaprint --enable-frei0r --enable-libx264 --enable-shared"
    }
})_")
        << QVersionNumber(4, 4, 2);
}

void FFmpegOutputParserTest::testVersionParser()
{
    QFETCH(QByteArray, ffprobeVersionOutput);
    QFETCH(QVersionNumber, versionNumber);

    const QVersionNumber v = parseVersionNumber(ffprobeVersionOutput);

    QCOMPARE(v, versionNumber);
}

void FFmpegOutputParserTest::testClipInfoParser_data()
{
    QTest::addColumn<QByteArray>("ffmpegVersionOutput");
    QTest::addColumn<ClipInfo>("clipInfo");

    // ffprobe -v quiet -print_format json -show_format -show_streams -select_streams V <video file>
    QTest::newRow("10.623s, 28.33 fps, 640x480, h264, yuv444p")
        << QByteArray(
R"({
    "streams": [
        {
            "index": 0,
            "codec_name": "h264",
            "codec_long_name": "H.264 / AVC / MPEG-4 AVC / MPEG-4 part 10",
            "profile": "High 4:4:4 Predictive",
            "codec_type": "video",
            "codec_tag_string": "[0][0][0][0]",
            "codec_tag": "0x0000",
            "width": 640,
            "height": 480,
            "coded_width": 640,
            "coded_height": 480,
            "closed_captions": 0,
            "film_grain": 0,
            "has_b_frames": 2,
            "pix_fmt": "yuv444p",
            "level": 30,
            "chroma_location": "left",
            "field_order": "progressive",
            "refs": 1,
            "is_avc": "true",
            "nal_length_size": "4",
            "r_frame_rate": "85/3",
            "avg_frame_rate": "85/3",
            "time_base": "1/1000",
            "start_pts": 0,
            "start_time": "0.000000",
            "bits_per_raw_sample": "8",
            "extradata_size": 41,
            "disposition": {
                "default": 1,
                "dub": 0,
                "original": 0,
                "comment": 0,
                "lyrics": 0,
                "karaoke": 0,
                "forced": 0,
                "hearing_impaired": 0,
                "visual_impaired": 0,
                "clean_effects": 0,
                "attached_pic": 0,
                "timed_thumbnails": 0,
                "captions": 0,
                "descriptions": 0,
                "metadata": 0,
                "dependent": 0,
                "still_image": 0
            },
            "tags": {
                "ENCODER": "Lavc58.54.100 libx264",
                "DURATION": "00:00:10.623000000"
            }
        }
    ],
    "format": {
        "filename": "out.mkv",
        "nb_streams": 1,
        "nb_programs": 0,
        "format_name": "matroska,webm",
        "format_long_name": "Matroska / WebM",
        "start_time": "0.000000",
        "duration": "10.623000",
        "size": "392136",
        "bit_rate": "295310",
        "probe_score": 100,
        "tags": {
            "ENCODER": "Lavf58.29.100"
        }
    }
})")
        << ClipInfo{ {}, {640, 480}, "h264", 10.623, 28.33333333333, "yuv444p", 0};
}

void FFmpegOutputParserTest::testClipInfoParser()
{
    QFETCH(QByteArray, ffmpegVersionOutput);
    QFETCH(ClipInfo, clipInfo);

    const ClipInfo ci = parseClipInfo(ffmpegVersionOutput);

    QCOMPARE(ci.duration, clipInfo.duration);
    QCOMPARE(ci.rFrameRate, clipInfo.rFrameRate);
    QCOMPARE(ci.dimensions, clipInfo.dimensions);
    QCOMPARE(ci.codec, clipInfo.codec);
    QCOMPARE(ci.pixFmt, clipInfo.pixFmt);
}

void FFmpegOutputParserTest::testFfmpegOutputParser_data()
{
    QTest::addColumn<QByteArray>("ffmpegRecordingLogLine");
    QTest::addColumn<int>("frameProgress");

    typedef QByteArray _;

    // ffmpeg -y -video_size vga -f x11grab -i :0.0+100,200 -vcodec qtrle /tmp/QtCreator-VMQjhs/AeOjep.mov
    QTest::newRow("skip 01")
        << _("ffmpeg version 4.4.2-0ubuntu0.22.04.1 Copyright (c) 2000-2021 the FFmpeg developers\n  built with gcc 11 (Ubuntu 11.2.0-19ubuntu1)\n  configuration: --prefix=/usr --extra-version=0ubuntu0.22.04.1 --toolchain=hardened --libdir=/usr/lib/x86_64-linux-gnu --incdir=/usr/include/x86_64-linux-gnu --arch=amd64 --enable-gpl --disable-stripping --enable-gnutls --enable-ladspa --enable-libaom --enable-libass --enable-libbluray --enable-libbs2b --enable-libcaca --enable-libcdio --enable-libcodec2 --enable-libdav1d --enable-libflite --enable-libfontconfig --enable-libfreetype --enable-libfribidi --enable-libgme --enable-libgsm --enable-libjack --enable-libmp3lame --enable-libmysofa --enable-libopenjpeg --enable-libopenmpt --enable-libopus --enable-libpulse --enable-librabbitmq --enable-librubberband --enable-libshine --enable-libsnappy --enable-libsoxr --enable-libspeex --enable-libsrt --enable-libssh --enable-libtheora --enable-libtwolame --enable-libvidstab --enable-libvorbis --enable-libvpx --enable-libwebp --enable-libx265 --enable-libxml2 --enable-libxvid --enable-libzimg --enable-libzmq --enable-libzvbi --enable-lv2 --enable-omx --enable-openal --enable-opencl --enable-opengl --enable-sdl2 --enable-pocketsphinx --enable-librsvg --enable-libmfx --enable-libdc1394 --enable-libdrm --enable-libiec61883 --enable-chromaprint --enable-frei0r --enable-libx264 --enable-shared\n")
        << -1;
    QTest::newRow("skip 02")
        << _("  libavutil      56. 70.100 / 56. 70.100\n  libavcodec     58.134.100 / 58.134.100\n  libavformat    58. 76.100 / 58. 76.100\n  libavdevice    58. 13.100 / 58. 13.100\n  libavfilter     7.110.100 /  7.110.100\n")
        << -1;
    QTest::newRow("skip 03")
        << _("  libswscale      5.  9.100 /  5.  9.100\n  libswresample   3.  9.100 /  3.  9.100\n  libpostproc    55.  9.100 / 55.  9.100\n")
        << -1;
    QTest::newRow("skip 04")
        << _("Input #0, x11grab, from ':0.0+100,200':\n  Duration: N/A, start: 1691512344.764131, bitrate: 294617 kb/s\n")
        << -1;
    QTest::newRow("skip 05")
        << _("  Stream #0:0: Video: rawvideo (BGR[0] / 0x524742), bgr0, 640x480, 294617 kb/s, ")
        << -1;
    QTest::newRow("skip 06")
        << _("29.97 fps, 30 tbr, 1000k tbn, 1000k tbc\n")
        << -1;
    QTest::newRow("skip 07")
        << _("Stream mapping:\n  Stream #0:0 -> #0:0 (rawvideo (native) -> qtrle (native))\n")
        << -1;
    QTest::newRow("skip 08")
        << _("Press [q] to stop, [?] for help\n")
        << -1;
    QTest::newRow("skip 09")
        << _("Output #0, mov, to '/tmp/QtCreator-VMQjhs/AeOjep.mov':\n  Metadata:\n    encoder         : Lavf58.76.100\n")
        << -1;
    QTest::newRow("skip 10")
        << _("  Stream #0:0: Video: qtrle (rle  / 0x20656C72), rgb24(pc, gbr/unknown/unknown, progressive), 640x480, q=2-31, 200 kb/s")
        << -1;
    QTest::newRow("skip 11")
        << _(", 30 fps, ")
        << -1;
    QTest::newRow("skip 12")
        << _("15360 tbn\n    Metadata:\n      encoder         : Lavc58.134.100 qtrle")
        << -1;
    QTest::newRow("skip 13")
        << _("\n")
        << -1;
    QTest::newRow("frame 1")
        << _("frame=    1 fps=0.0 q=-0.0 size=       0kB time=00:00:00.00 bitrate=4430.8kbits/s speed=N/A    \r")
        << 1;
    QTest::newRow("frame 21")
        << _("frame=   21 fps=0.0 q=-0.0 size=     256kB time=00:00:00.66 bitrate=3145.9kbits/s speed=1.33x    \r")
        << 21;
    QTest::newRow("frame 36")
        << _("frame=   36 fps= 36 q=-0.0 size=     256kB time=00:00:01.16 bitrate=1797.7kbits/s speed=1.17x    \r")
        << 36;
    QTest::newRow("frame 51")
        << _("frame=   51 fps= 34 q=-0.0 size=     512kB time=00:00:01.66 bitrate=2516.7kbits/s speed=1.11x    \r")
        << 51;
    QTest::newRow("skip 14")
        << _("\r\n\r\n[q] command received. Exiting.\r\n\r\n")
        << -1;
    QTest::newRow("frame 65 - final log line")
        << _("frame=   65 fps= 32 q=-0.0 Lsize=     801kB time=00:00:02.13 bitrate=3074.4kbits/s speed=1.07x    \nvideo:800kB audio:0kB subtitle:0kB other streams:0kB global headers:0kB muxing overhead: 0.125299%\n")
        << 65;
}

void FFmpegOutputParserTest::testFfmpegOutputParser()
{
    QFETCH(QByteArray, ffmpegRecordingLogLine);
    QFETCH(int, frameProgress);

    const int parsedFrameProgress = parseFrameProgressFromOutput(ffmpegRecordingLogLine);

    QCOMPARE(parsedFrameProgress, frameProgress);
}

} // namescace ScreenRecorder::Internal
#endif // WITH_TESTS
