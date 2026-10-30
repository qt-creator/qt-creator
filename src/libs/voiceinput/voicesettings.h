// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/aspects.h>

#include <QList>
#include <QString>
#include <QUrl>

namespace VoiceInput::Internal {

// The license of the weights, which conversions for whisper.cpp keep.
struct ModelLicense
{
    QUrl url;
    QString name;
    QString author;
};

struct ModelInfo
{
    QString id;
    QString name;
    int sizeMb = 0;
    // The Hugging Face repository the model is downloaded from.
    QString repository;
    ModelLicense license;
};

const QList<ModelInfo> &models();
const ModelInfo *modelInfo(const QString &id);

class VoiceSettings final : public Utils::AspectContainer
{
public:
    VoiceSettings();

    // The id of the speech recognition model.
    Utils::StringSelectionAspect model{this};
    Utils::BoolAspect qtResampling{this};
    Utils::BoolAspect holdToRecord{this};
    Utils::BoolAspect saveRecordings{this};
};

VoiceSettings &settings();

} // namespace VoiceInput::Internal
