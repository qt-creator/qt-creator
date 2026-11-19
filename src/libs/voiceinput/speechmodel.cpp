// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "speechmodel.h"

#include "speechactivity.h"

#include <utils/filepath.h>

#include <QCoreApplication>
#include <QFile>
#include <QLoggingCategory>
#include <QRegularExpression>
#include <QStringList>

#include <ggml-backend.h>
#include <whisper.h>

#include <algorithm>
#include <mutex>
#include <optional>
#include <thread>

namespace VoiceInput::Internal {

static Q_LOGGING_CATEGORY(voiceLog, "qtc.voiceinput", QtWarningMsg)

const int kSampleRate = WHISPER_SAMPLE_RATE;
// whisper.cpp refuses input shorter than a second, so shorter input is padded.
const size_t kMinSamples = kSampleRate + kSampleRate / 10;

// A whisper model.
struct SpeechModel
{
    ~SpeechModel()
    {
        if (whisper)
            whisper_free(whisper);
    }

    whisper_context *whisper = nullptr;
    // A context runs one transcription at a time.
    std::mutex mutex;
};


static void logFromWhisper(ggml_log_level level, const char *text, void *)
{
    if (level == GGML_LOG_LEVEL_ERROR)
        qCWarning(voiceLog).noquote() << QString::fromUtf8(text).trimmed();
    else
        qCDebug(voiceLog).noquote() << QString::fromUtf8(text).trimmed();
}

// Loads the backends that ggml was built with as modules. One whose
// requirements this system lacks, such as a Vulkan driver, is skipped.
static void loadBackends()
{
#ifdef GGML_BACKEND_DL
    // Only Qt Creator's own directory: ggml would also search the current one.
    const Utils::FilePath backendPath = Utils::FilePath::fromString(
                                            QCoreApplication::applicationDirPath())
                                            .pathAppended(QTC_GGML_BACKEND_PATH);
    ggml_backend_load_all_from_path(backendPath.nativePath().toUtf8().constData());
#endif
    for (size_t i = 0; i < ggml_backend_dev_count(); ++i) {
        ggml_backend_dev_t device = ggml_backend_dev_get(i);
        qCDebug(voiceLog) << "Device:" << ggml_backend_dev_name(device)
                          << ggml_backend_dev_description(device);
    }
}

// The index of the first discrete GPU, counted the way whisper.cpp counts
// gpu_device. An integrated GPU can be slower than the CPU it sits next to.
static std::optional<int> discreteGpuIndex()
{
    int gpu = 0;
    for (size_t i = 0; i < ggml_backend_dev_count(); ++i) {
        switch (ggml_backend_dev_type(ggml_backend_dev_get(i))) {
        case GGML_BACKEND_DEVICE_TYPE_GPU:
            return gpu;
        case GGML_BACKEND_DEVICE_TYPE_IGPU:
            ++gpu;
            break;
        default:
            break;
        }
    }
    return {};
}

/*!
    \internal

    Loads the whisper model at \a modelPath.
*/
std::shared_ptr<SpeechModel> loadSpeechModel(const QString &modelPath)
{
    static std::once_flag initialized;
    std::call_once(initialized, [] {
        whisper_log_set(&logFromWhisper, nullptr);
        loadBackends();
    });

    auto model = std::make_shared<SpeechModel>();
    whisper_context_params params = whisper_context_default_params();
    const std::optional<int> gpu = discreteGpuIndex();
    params.use_gpu = gpu.has_value();
    params.gpu_device = gpu.value_or(0);
    model->whisper
        = whisper_init_from_file_with_params(QFile::encodeName(modelPath).constData(), params);
    if (!model->whisper)
        return {};
    return model;
}

static bool isNonSpeech(const QString &segment)
{
    // whisper marks silence and noise as "[BLANK_AUDIO]", "(music)" and the like.
    static const QRegularExpression annotation(R"(^\s*[\[\(][^\]\)]*[\]\)]\s*$)");
    return segment.isEmpty() || annotation.match(segment).hasMatch();
}

static bool isCancelled(void *cancelled)
{
    return static_cast<std::atomic_bool *>(cancelled)->load();
}

static QString transcribeWithWhisper(whisper_context *context,
                                     const std::vector<float> &samples,
                                     const QByteArray &language,
                                     const QByteArray &prompt,
                                     std::atomic_bool *cancelled)
{
    whisper_full_params params = whisper_full_default_params(WHISPER_SAMPLING_GREEDY);
    params.n_threads = std::clamp(int(std::thread::hardware_concurrency()) / 2, 1, 8);
    params.print_progress = false;
    params.print_realtime = false;
    params.print_special = false;
    params.print_timestamps = false;
    params.no_context = true;
    params.no_timestamps = true;
    params.suppress_nst = true;
    params.language = language.constData();
    if (!prompt.isEmpty())
        params.initial_prompt = prompt.constData();
    params.abort_callback = &isCancelled;
    params.abort_callback_user_data = cancelled;

    if (whisper_full(context, params, samples.data(), int(samples.size())) != 0)
        return {};

    QStringList segments;
    const int count = whisper_full_n_segments(context);
    for (int i = 0; i < count; ++i) {
        const QString segment
            = QString::fromUtf8(whisper_full_get_segment_text(context, i)).trimmed();
        if (!isNonSpeech(segment))
            segments.append(segment);
    }
    return segments.join(' ');
}

/*!
    \internal

    Transcribes the 16 kHz mono \a samples with \a model, if they contain
    speech. Whisper takes the \a language and the \a prompt.
*/
Transcription transcribe(std::shared_ptr<SpeechModel> model,
                         std::vector<float> samples,
                         QByteArray language,
                         QByteArray prompt,
                         std::shared_ptr<std::atomic_bool> cancelled)
{
    const std::lock_guard lock(model->mutex);

    if (samples.empty())
        return {};
    // Speech recognition makes up text for noise, so only ask it about speech.
    Transcription result;
    const Internal::SpeechActivity activity
        = Internal::loudnessActivity(samples.data(), samples.size(), kSampleRate);
    result.hasSpeech = activity.hasSpeech;
    result.endsInPause = activity.endsInPause;
    if (!result.hasSpeech)
        return result;

    if (samples.size() < kMinSamples)
        samples.resize(kMinSamples, 0.f);

    result.text = transcribeWithWhisper(model->whisper, samples, language, prompt,
                                        cancelled.get());
    return result;
}

} // namespace VoiceInput::Internal
