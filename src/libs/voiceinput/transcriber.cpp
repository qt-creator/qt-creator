// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "transcriber.h"

#include "speechactivity.h"
#include "speechmodel.h"
#include "voiceinputtr.h"
#include "voicesettings.h"

#include <utils/algorithm.h>
#include <utils/appinfo.h>
#include <utils/async.h>
#include <utils/filepath.h>
#include <utils/globaltasktree.h>
#include <utils/networkaccessmanager.h>
#include <utils/qtcassert.h>

#include <QActionGroup>
#include <QApplication>
#include <QAudioDevice>
#include <QAudioFormat>
#include <QAudioSource>
#include <QDateTime>
#include <QDebug>
#include <QDesktopServices>
#include <QFile>
#include <QFutureWatcher>
#include <QLoggingCategory>
#include <QMediaDevices>
#include <QMenu>
#include <QMessageBox>
#include <QNetworkReply>
#include <QPermissions>
#include <QPointer>
#include <QRegularExpression>
#include <QSet>
#include <QTimer>

#include <QtTaskTree/QNetworkReplyWrapper>
#include <QtTaskTree/QTaskTree>

#include <whisper.h>

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstring>
#include <map>
#include <memory>
#include <mutex>
#include <thread>
#include <utility>
#include <vector>

using namespace QtTaskTree;
using namespace Utils;

namespace VoiceInput {

static Q_LOGGING_CATEGORY(voiceLog, "qtc.voiceinput", QtWarningMsg)


const int kSampleRate = WHISPER_SAMPLE_RATE;
// whisper decodes up to 30 s at once. Text is fixed and the audio dropped
// once this much has been transcribed, if no pause came before.
const size_t kCommitSamples = 25 * kSampleRate;
// What whisper is told was said before the audio, for consistent wording.
const qsizetype kPromptChars = 200;
const int kTranscribeIntervalMs = 1000;
// Kept of audio without speech, so that the beginning of a word is not lost.
const size_t kLeadInSamples = kSampleRate / 2;
const double kHighPassHz = 100;
// How much audio the level reported to a meter covers.
const int kLevelSamples = kSampleRate / 10;

using Internal::SpeechModel;
using Internal::Transcription;
using Internal::ModelInfo;
using Internal::modelInfo;
using Internal::models;

struct ModelFile
{
    QUrl url;
    FilePath path;
};

// Recordings and their transcripts, collected as training data.
static FilePath recordingDirectory()
{
    return Utils::appInfo().userResources.pathAppended("voiceinput/recordings");
}

static FilePath modelDirectory()
{
    return Utils::appInfo().userResources.pathAppended("whisper");
}

static QUrl repositoryPage(const ModelInfo &info)
{
    return QUrl("https://huggingface.co/" + info.repository);
}

// The files that make up a model.
static QList<ModelFile> modelFiles(const ModelInfo &info)
{
    const QString base = repositoryPage(info).toString() + "/resolve/main/";
    const QString file = "ggml-" + info.id + ".bin";
    return {{QUrl(base + file), modelDirectory().pathAppended(file)}};
}

static bool hasModelFiles(const ModelInfo &info)
{
    return Utils::allOf(modelFiles(info), [](const ModelFile &file) { return file.path.exists(); });
}

using Internal::settings;

static QString selectedModelId()
{
    return settings().model();
}

static std::map<QString, std::weak_ptr<SpeechModel>> &loadedModels()
{
    static std::map<QString, std::weak_ptr<SpeechModel>> models;
    return models;
}

static float sampleAt(const char *data, QAudioFormat::SampleFormat format)
{
    switch (format) {
    case QAudioFormat::UInt8:
        return (int(uchar(*data)) - 128) / 128.f;
    case QAudioFormat::Int16: {
        qint16 value;
        std::memcpy(&value, data, sizeof(value));
        return value / 32768.f;
    }
    case QAudioFormat::Int32: {
        qint32 value;
        std::memcpy(&value, data, sizeof(value));
        return float(value / 2147483648.);
    }
    case QAudioFormat::Float: {
        float value;
        std::memcpy(&value, data, sizeof(value));
        return value;
    }
    default:
        return 0.f;
    }
}

static std::function<void(QTaskTree *, const QString &)> &downloadProgressHandler()
{
    static std::function<void(QTaskTree *, const QString &)> handler;
    return handler;
}

// Downloads the files one after the other. Each goes to a ".part" file that replaces the file
// once it is complete, so that a failed or canceled download leaves nothing behind.
static Group downloadRecipe(const QList<ModelFile> &files, const std::shared_ptr<QString> &error)
{
    const ListIterator iterator(files);
    const Storage<std::unique_ptr<QFile>> partialFile;

    const auto onSetup = [iterator, partialFile, error](QNetworkReplyWrapper &query) {
        const FilePath partial = iterator->path.stringAppended(".part");
        if (const Result<> result = partial.parentDir().ensureWritableDir(); !result) {
            *error = result.error();
            return SetupResult::StopWithError;
        }
        auto file = std::make_unique<QFile>(partial.toFSPathString());
        if (!file->open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            *error = Tr::tr("Cannot write \"%1\": %2")
                         .arg(partial.toUserOutput(), file->errorString());
            return SetupResult::StopWithError;
        }
        QNetworkRequest request(iterator->url);
        request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                             QNetworkRequest::NoLessSafeRedirectPolicy);
        query.setRequest(request);
        query.setNetworkAccessManager(NetworkAccessManager::instance());
        // Models are large, so they go to the disk as they arrive.
        QFile *out = file.get();
        QObject::connect(&query, &QNetworkReplyWrapper::started, out, [&query, out] {
            QObject::connect(query.reply(), &QNetworkReply::readyRead, out, [&query, out] {
                out->write(query.reply()->readAll());
            });
        });
        *partialFile = std::move(file);
        return SetupResult::Continue;
    };
    const auto onDone = [iterator, partialFile, error](const QNetworkReplyWrapper &query,
                                                       DoneWith result) {
        QFile &file = **partialFile;
        QNetworkReply *reply = query.reply();
        if (reply)
            file.write(reply->readAll());
        file.close();
        const FilePath partial = iterator->path.stringAppended(".part");
        if (result != DoneWith::Success) {
            partial.removeFile();
            if (result == DoneWith::Error && reply)
                *error = Tr::tr("Downloading the model failed: %1").arg(reply->errorString());
            return DoneResult::Error;
        }
        iterator->path.removeFile();
        if (const Result<> renamed = partial.renameFile(iterator->path); !renamed) {
            *error = renamed.error();
            return DoneResult::Error;
        }
        return DoneResult::Success;
    };

    return Group {
        For (iterator) >> Do {
            partialFile,
            QNetworkReplyWrapperTask(onSetup, onDone),
        },
    };
}

static bool isDownloaded(const QString &id)
{
    const ModelInfo *info = modelInfo(id);
    return info && hasModelFiles(*info);
}

// Downloads what is missing of the speech recognition model, unless that is already being
// done.
static void downloadModel(const QString &id, QWidget *dialogParent,
                          const std::function<void(bool)> &done)
{
    const ModelInfo *info = modelInfo(id);
    QTC_ASSERT(info, done(false); return);

    QList<ModelFile> files = modelFiles(*info);
    Utils::erase(files, [](const ModelFile &file) { return file.path.exists(); });

    // Two downloads of the same model would write the same files.
    static QSet<QString> running;
    if (running.contains(id))
        return;
    running.insert(id);

    const auto error = std::make_shared<QString>();
    const QString title = Tr::tr("Downloading Voice Input Model \"%1\"").arg(info->name);
    const QPointer<QWidget> parent = dialogParent;
    GlobalTaskTree::start(
        downloadRecipe(files, error),
        [title](QTaskTree &taskTree) {
            if (const auto &handler = downloadProgressHandler())
                handler(&taskTree, title);
        },
        [id, error, parent, done](const QTaskTree &, DoneWith result) {
            running.remove(id);
            if (result == DoneWith::Error && !error->isEmpty())
                QMessageBox::warning(parent, Tr::tr("Voice Input"), *error);
            done(result == DoneWith::Success);
        });
}

static QString joined(const QString &first, const QString &second)
{
    if (first.isEmpty())
        return second;
    if (second.isEmpty())
        return first;
    return first + ' ' + second;
}

class TranscriberPrivate
{
public:
    explicit TranscriberPrivate(Transcriber *q)
        : q(q)
    {
        m_timer.setInterval(kTranscribeIntervalMs);
        QObject::connect(&m_timer, &QTimer::timeout, q, [this] { maybeTranscribe(); });
        QObject::connect(&m_modelWatcher, &QFutureWatcherBase::finished, q, [this] {
            modelLoaded();
        });
        QObject::connect(&m_jobWatcher, &QFutureWatcherBase::finished, q, [this] {
            transcribed();
        });
    }

    ~TranscriberPrivate() { m_cancelled->store(true); }

    void setState(Transcriber::State state)
    {
        if (m_state == state)
            return;
        m_state = state;
        if (state == Transcriber::State::Idle)
            saveRecording();
        emit q->stateChanged(state);
    }

    QString transcriptText() const
    {
        return joined(m_committedText, m_pendingText);
    }

    void saveRecording()
    {
        if (m_recording.empty())
            return;
        const std::vector<float> recording = std::exchange(m_recording, {});
        const FilePath directory = recordingDirectory();
        const FilePath base = directory.pathAppended(
            QDateTime::currentDateTime().toString("yyyyMMdd-HHmmss-zzz"));
        const FilePath audio = base.stringAppended(".wav");
        const FilePath text = base.stringAppended(".txt");
        QString error;
        if (const Result<> result = directory.ensureWritableDir(); !result)
            error = result.error();
        else if (const Result<qint64> result
                 = audio.writeFileContents(Internal::wavFile(recording, kSampleRate));
                 !result)
            error = result.error();
        else if (const Result<qint64> result
                 = text.writeFileContents(transcriptText().toUtf8());
                 !result)
            error = result.error();
        if (!error.isEmpty()) {
            qWarning().noquote() << "Voice input: cannot save the recording:" << error;
            return;
        }
        qDebug().noquote() << "Voice input: saved the recording to" << audio.toUserOutput()
                           << "and its transcript to" << text.toUserOutput();
    }

    void fail(const QString &message)
    {
        stopAudio();
        m_timer.stop();
        setState(Transcriber::State::Idle);
        emit q->errorOccurred(message);
    }

    void requestPermissionAndStart()
    {
#if QT_CONFIG(permissions)
        const QMicrophonePermission permission;
        switch (qApp->checkPermission(permission)) {
        case Qt::PermissionStatus::Undetermined:
            qApp->requestPermission(permission, q, [this](const QPermission &result) {
                if (result.status() == Qt::PermissionStatus::Granted)
                    startRecording();
                else
                    emit q->errorOccurred(Tr::tr("Access to the microphone was denied."));
            });
            return;
        case Qt::PermissionStatus::Denied:
            emit q->errorOccurred(Tr::tr("Access to the microphone was denied. Allow it in "
                                         "the system settings."));
            return;
        case Qt::PermissionStatus::Granted:
            break;
        }
#endif
        startRecording();
    }

    void startRecording()
    {
        if (m_state != Transcriber::State::Idle)
            return;

        const QAudioDevice device = QMediaDevices::defaultAudioInput();
        if (device.isNull()) {
            emit q->errorOccurred(Tr::tr("No microphone found."));
            return;
        }
        // Records at the device's rate by default, to work around QTBUG-151918: on
        // macOS, Qt repeats pieces of the audio when it converts the sample rate,
        // which crackles.
        QAudioFormat format = device.preferredFormat();
        if (settings().qtResampling()) {
            QAudioFormat converted;
            converted.setSampleRate(kSampleRate);
            converted.setChannelCount(1);
            converted.setSampleFormat(QAudioFormat::Float);
            if (device.isFormatSupported(converted))
                format = converted;
        }
        m_format = format;

        m_samples.clear();
        m_recording.clear();
        m_savesRecording = settings().saveRecordings();
        m_firstSample = 0;
        m_levelSum = 0;
        m_levelCount = 0;
        m_transcribedSamples = 0;
        m_committedText.clear();
        m_pendingText.clear();
        m_pendingBytes.clear();
        m_resampler = Internal::Resampler(format.sampleRate(), kSampleRate);
        m_highPass = Internal::HighPassFilter(kHighPassHz, kSampleRate);

        m_audioSource = new QAudioSource(device, format, q);
        m_audioDevice = m_audioSource->start();
        if (!m_audioDevice) {
            fail(Tr::tr("Cannot record from \"%1\".").arg(device.description()));
            return;
        }
        QObject::connect(m_audioDevice, &QIODevice::readyRead, q, [this] { readAudio(); });
        qCDebug(voiceLog) << "Recording from" << device.description() << format;

        setState(Transcriber::State::Recording);
        ensureModel();
        m_timer.start();
    }

    void stop()
    {
        if (m_state != Transcriber::State::Recording)
            return;
        readAudio();
        stopAudio();
        m_timer.stop();
        setState(Transcriber::State::Finishing);
        if (m_modelWatcher.isRunning())
            return;
        if (!m_model)
            setState(Transcriber::State::Idle);
        else
            maybeTranscribe();
    }

    void stopAudio()
    {
        if (!m_audioSource)
            return;
        m_audioSource->stop();
        m_audioSource->deleteLater();
        m_audioSource = nullptr;
        m_audioDevice = nullptr;
    }

    void readAudio()
    {
        if (!m_audioDevice)
            return;
        m_pendingBytes.append(m_audioDevice->readAll());

        const int frameBytes = m_format.bytesPerFrame();
        const int sampleBytes = m_format.bytesPerSample();
        const int channels = m_format.channelCount();
        QTC_ASSERT(frameBytes > 0 && channels > 0, return);

        const qsizetype frames = m_pendingBytes.size() / frameBytes;
        const char *data = m_pendingBytes.constData();
        for (qsizetype frame = 0; frame < frames; ++frame) {
            float mono = 0;
            for (int channel = 0; channel < channels; ++channel)
                mono += sampleAt(data + frame * frameBytes + channel * sampleBytes,
                                 m_format.sampleFormat());
            m_unresampled.push_back(mono / channels);
        }
        m_pendingBytes.remove(0, frames * frameBytes);
        m_resampled.clear();
        m_resampler.process(m_unresampled.data(), m_unresampled.size(), m_resampled);
        m_unresampled.clear();
        for (const float sample : m_resampled)
            appendSample(sample);
    }

    // The high-pass filter takes out rumble that is louder than it sounds.
    void appendSample(float unfiltered)
    {
        if (m_savesRecording)
            m_recording.push_back(unfiltered);
        const float sample = m_highPass.process(unfiltered);
        m_samples.push_back(sample);
        m_levelSum += double(sample) * sample;
        if (++m_levelCount == kLevelSamples) {
            const qint64 end = m_firstSample + qint64(m_samples.size());
            emit q->levelMeasured(end - kLevelSamples,
                                  float(std::sqrt(m_levelSum / kLevelSamples)));
            m_levelSum = 0;
            m_levelCount = 0;
        }
    }

    void dropSamples(size_t count)
    {
        m_samples.erase(m_samples.begin(), m_samples.begin() + count);
        m_firstSample += qint64(count);
    }

    void ensureModel()
    {
        const QString id = selectedModelId();
        const ModelInfo *info = modelInfo(id);
        QTC_ASSERT(info, return);
        const QList<ModelFile> files = modelFiles(*info);
        const QString path = files.first().path.toFSPathString();
        if (m_model && m_modelPath == path)
            return;
        m_model.reset();
        m_modelPath = path;
        if (std::shared_ptr<SpeechModel> loaded = loadedModels()[path].lock()) {
            m_model = loaded;
            return;
        }
        if (!m_modelWatcher.isRunning()) {
            m_modelWatcher.setFuture(
                Utils::asyncRun(&Internal::loadSpeechModel, path));
            emit q->loadingModelChanged(true);
        }
    }

    void modelLoaded()
    {
        emit q->loadingModelChanged(false);
        const QString path = m_modelPath;
        m_model = m_modelWatcher.result();
        if (!m_model) {
            if (m_state != Transcriber::State::Idle)
                fail(Tr::tr("Cannot load the speech recognition model \"%1\".")
                         .arg(FilePath::fromString(path).toUserOutput()));
            return;
        }
        loadedModels()[m_modelPath] = m_model;
        if (m_state == Transcriber::State::Finishing && m_samples.empty())
            setState(Transcriber::State::Idle);
        else
            maybeTranscribe();
    }

    void maybeTranscribe()
    {
        if (!m_model || m_jobWatcher.isRunning())
            return;
        if (m_samples.size() == m_transcribedSamples) {
            if (m_state == Transcriber::State::Finishing)
                setState(Transcriber::State::Idle);
            return;
        }
        const QString modelId = selectedModelId();
        const QByteArray language = modelId.endsWith(".en") ? "en" : "auto";
        const QByteArray prompt = m_committedText.right(kPromptChars).toUtf8();
        m_jobSamples = m_samples.size();
        m_jobWatcher.setFuture(
            Utils::asyncRun(&Internal::transcribe, m_model, m_samples, language, prompt,
                            m_cancelled));
    }

    void transcribed()
    {
        if (m_state == Transcriber::State::Idle)
            return;
        const Transcription result = m_jobWatcher.result();
        const QString &text = result.text;
        const bool pause = result.endsInPause;
        const bool last = m_state == Transcriber::State::Finishing
                          && m_jobSamples == m_samples.size();
        // A pass that heard nothing must not drop what earlier ones heard.
        const bool heard = !text.isEmpty() || m_pendingText.isEmpty();
        if (m_jobSamples >= kCommitSamples || ((pause || last) && heard)) {
            m_committedText = joined(m_committedText, text);
            m_pendingText.clear();
            dropSamples(m_jobSamples);
            m_transcribedSamples = 0;
        } else if (!result.hasSpeech && m_pendingText.isEmpty()) {
            const size_t dropped
                = m_jobSamples > kLeadInSamples ? m_jobSamples - kLeadInSamples : 0;
            dropSamples(dropped);
            m_transcribedSamples = m_jobSamples - dropped;
        } else {
            m_pendingText = text;
            m_transcribedSamples = m_jobSamples;
        }
        emit q->transcriptChanged(transcriptText());
        if (m_state == Transcriber::State::Finishing)
            maybeTranscribe();
    }

    Transcriber *q;
    Transcriber::State m_state = Transcriber::State::Idle;

    QAudioFormat m_format;
    QAudioSource *m_audioSource = nullptr;
    QIODevice *m_audioDevice = nullptr;
    QByteArray m_pendingBytes;
    std::vector<float> m_unresampled;
    std::vector<float> m_resampled;
    Internal::Resampler m_resampler{kSampleRate, kSampleRate};
    Internal::HighPassFilter m_highPass{kHighPassHz, kSampleRate};

    // 16 kHz mono audio whose transcript is not committed yet.
    std::vector<float> m_samples;
    // The whole recording at 16 kHz, before the high-pass filter, if it is saved.
    bool m_savesRecording = false;
    std::vector<float> m_recording;
    // Where m_samples starts in the recording.
    qint64 m_firstSample = 0;
    double m_levelSum = 0;
    int m_levelCount = 0;
    size_t m_transcribedSamples = 0;
    size_t m_jobSamples = 0;
    // The transcript of the dropped audio, and of the audio since.
    QString m_committedText;
    QString m_pendingText;

    QString m_modelPath;
    std::shared_ptr<SpeechModel> m_model;
    QFutureWatcher<std::shared_ptr<SpeechModel>> m_modelWatcher;
    QFutureWatcher<Transcription> m_jobWatcher;
    std::shared_ptr<std::atomic_bool> m_cancelled = std::make_shared<std::atomic_bool>(false);
    QTimer m_timer;
};

/*!
    \class VoiceInput::Transcriber
    \inmodule QtCreator

    \brief The Transcriber class records from the microphone and transcribes
    what the user says on this computer with whisper.cpp.

    While recording, it reports the whole transcript so far with
    transcriptChanged() about once a second.
*/

Transcriber::Transcriber(QObject *parent)
    : QObject(parent)
    , d(new TranscriberPrivate(this))
{}

Transcriber::~Transcriber()
{
    delete d;
}

Transcriber::State Transcriber::state() const
{
    return d->m_state;
}

bool Transcriber::isLoadingModel() const
{
    return d->m_modelWatcher.isRunning();
}

/*!
    Starts recording, or, without a model, shows the settings menu below
    \a dialogParent for selecting one.
*/
void Transcriber::start(QWidget *dialogParent)
{
    if (d->m_state != State::Idle)
        return;
    if (!hasModel()) {
        showSettingsMenu(dialogParent,
                         dialogParent->mapToGlobal(QPoint(0, dialogParent->height())));
        return;
    }
    d->requestPermissionAndStart();
}

/*!
    Sets the \a handler that shows the progress of downloading a model, which
    gets the task tree of the download and its title. Plugins that have a
    progress bar set it.
*/
void Transcriber::setDownloadProgressHandler(
    const std::function<void(QtTaskTree::QTaskTree *, const QString &)> &handler)
{
    downloadProgressHandler() = handler;
}

void Transcriber::stop()
{
    d->stop();
}

bool Transcriber::hasModel()
{
    const QString id = selectedModelId();
    return isDownloaded(id);
}

/*!
    Returns whether recording lasts as long as the user holds the button,
    instead of from one click to the next.
*/
bool Transcriber::holdsToRecord()
{
    return settings().holdToRecord();
}

QString Transcriber::modelName()
{
    const ModelInfo *info = modelInfo(selectedModelId());
    return info ? info->name : QString();
}

/*!
    Shows a menu at \a globalPos for selecting the model and the other voice
    input settings. Selecting a model that is not downloaded yet downloads it,
    with \a parent as the parent of the progress dialog. Calls \a onChanged
    after a setting changed.
*/
void Transcriber::showSettingsMenu(QWidget *parent,
                                   const QPoint &globalPos,
                                   const std::function<void()> &onChanged)
{
    auto menu = new QMenu(parent);
    menu->setAttribute(Qt::WA_DeleteOnClose);
    menu->setToolTipsVisible(true);
    // The download may outlive the menu and its parent.
    const QPointer<QWidget> guardedParent = parent;
    const auto changed = [onChanged] {
        if (onChanged)
            onChanged();
    };

    QMenu *modelMenu = menu->addMenu(Tr::tr("Model"));
    modelMenu->setToolTipsVisible(true);
    const QString current = selectedModelId();
    for (const ModelInfo &info : models()) {
        const QString &id = info.id;
        const bool downloaded = hasModelFiles(info);
        const bool inUse = id == current && downloaded;
        QMenu *modelEntry = modelMenu->addMenu(
            downloaded ? info.name
                       : Tr::tr("%1 (%2 MB)").arg(info.name).arg(info.sizeMb));
        modelEntry->setToolTipsVisible(true);
        // Only the model in use shows a check mark, not an empty box for every other one.
        modelEntry->menuAction()->setCheckable(inUse);
        modelEntry->menuAction()->setChecked(inUse);

        QAction *use = modelEntry->addAction(
            downloaded ? Tr::tr("Use") : Tr::tr("Download (%1 MB)").arg(info.sizeMb));
        use->setEnabled(!inUse);
        QObject::connect(use, &QAction::triggered, [id, guardedParent, changed] {
            const auto select = [id, changed] {
                settings().model.setValue(id);
                changed();
            };
            if (isDownloaded(id)) {
                select();
                return;
            }
            downloadModel(id, guardedParent, [select](bool ok) {
                if (ok)
                    select();
            });
        });
        modelEntry->addSeparator();

        QAction *licenseAction = modelEntry->addAction(
            Tr::tr("License: %1, by %2").arg(info.license.name, info.license.author));
        licenseAction->setToolTip(info.license.url.toString());
        QObject::connect(licenseAction, &QAction::triggered, [url = info.license.url] {
            QDesktopServices::openUrl(url);
        });
        QAction *source = modelEntry->addAction(Tr::tr("Source: %1").arg(info.repository));
        source->setToolTip(repositoryPage(info).toString());
        QObject::connect(source, &QAction::triggered, [url = repositoryPage(info)] {
            QDesktopServices::openUrl(url);
        });
    }

    QMenu *resamplerMenu = menu->addMenu(Tr::tr("Resampler"));
    resamplerMenu->setToolTipsVisible(true);
    auto resamplers = new QActionGroup(resamplerMenu);
    const auto addResampler = [&](const QString &text, const QString &toolTip, bool qt) {
        QAction *action = resamplerMenu->addAction(text);
        action->setCheckable(true);
        action->setChecked(settings().qtResampling() == qt);
        action->setToolTip(toolTip);
        resamplers->addAction(action);
        QObject::connect(action, &QAction::triggered, [qt, changed] {
            settings().qtResampling.setValue(qt);
            changed();
        });
    };
    addResampler(Tr::tr("Voice Input"),
                 Tr::tr("Records at the rate of the microphone and converts the recording to "
                        "16 kHz for the model."),
                 false);
    addResampler(Tr::tr("Qt Multimedia"),
                 Tr::tr("Lets Qt Multimedia record at 16 kHz. On macOS, this crackles with Qt "
                        "versions that have QTBUG-151918."),
                 true);

    menu->addSeparator();

    // The actions belong to the aspects and outlive the menu, so the menu is the context.
    const auto addSetting = [&](BoolAspect &aspect) {
        QAction *action = aspect.action();
        menu->addAction(action);
        QObject::connect(action, &QAction::triggered, menu, changed);
        return action;
    };
    addSetting(settings().holdToRecord);
    addSetting(settings().saveRecordings)
        ->setToolTip(Tr::tr("Saves each recording and its transcript to \"%1\", for example to "
                            "collect speech for testing or training models.")
                         .arg(recordingDirectory().toUserOutput()));

    QAction *openRecordings = menu->addAction(Tr::tr("Open Recordings Folder"));
    openRecordings->setEnabled(recordingDirectory().exists());
    QObject::connect(openRecordings, &QAction::triggered, [] {
        QDesktopServices::openUrl(QUrl::fromLocalFile(recordingDirectory().toFSPathString()));
    });

    menu->popup(globalPos);
}

} // namespace VoiceInput
