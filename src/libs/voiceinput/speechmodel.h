// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QByteArray>
#include <QString>

#include <atomic>
#include <memory>
#include <vector>

namespace VoiceInput::Internal {

struct SpeechModel;

struct Transcription
{
    QString text;
    bool hasSpeech = false;
    bool endsInPause = false;
};

std::shared_ptr<SpeechModel> loadSpeechModel(const QString &modelPath);

Transcription transcribe(std::shared_ptr<SpeechModel> model,
                         std::vector<float> samples,
                         QByteArray language,
                         QByteArray prompt,
                         std::shared_ptr<std::atomic_bool> cancelled);

} // namespace VoiceInput::Internal
