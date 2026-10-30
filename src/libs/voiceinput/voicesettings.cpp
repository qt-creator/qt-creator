// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "voicesettings.h"

#include "voiceinputtr.h"

#include <utils/shutdownguard.h>

#include <QStandardItem>

using namespace Utils;

namespace VoiceInput::Internal {

/*!
    \internal

    Returns the speech recognition models that voice input offers.
*/
const QList<ModelInfo> &models()
{
    const auto whisper = [](const QString &id, const QString &name, int sizeMb) {
        return ModelInfo{
            .id = id,
            .name = name,
            .sizeMb = sizeMb,
            .repository = "ggerganov/whisper.cpp",
            .license = {QUrl("https://github.com/openai/whisper/blob/main/LICENSE"),
                        "MIT",
                        "OpenAI"},
        };
    };
    static const QList<ModelInfo> theModels = {
        whisper("tiny.en", Tr::tr("Tiny, English only"), 75),
        whisper("tiny", Tr::tr("Tiny, multilingual"), 75),
        whisper("base.en", Tr::tr("Base, English only"), 142),
        whisper("base", Tr::tr("Base, multilingual"), 142),
        whisper("small.en", Tr::tr("Small, English only"), 466),
        whisper("small", Tr::tr("Small, multilingual"), 466),
        whisper("large-v3-turbo-q5_0", Tr::tr("Large v3 Turbo, quantized, multilingual"), 547),
        whisper("medium", Tr::tr("Medium, multilingual"), 1500),
        whisper("large-v3-turbo", Tr::tr("Large v3 Turbo, multilingual"), 1600),
    };
    return theModels;
}

const ModelInfo *modelInfo(const QString &id)
{
    for (const ModelInfo &info : models()) {
        if (info.id == id)
            return &info;
    }
    return nullptr;
}

// Lists the models for the model setting, with their ids as data.
static void fillModelItems(const StringSelectionAspect::ResultCallback &callback)
{
    QList<QStandardItem *> items;
    for (const ModelInfo &info : models()) {
        auto item = new QStandardItem(info.name);
        item->setData(info.id);
        items.append(item);
    }
    callback(items);
}

/*!
    \class VoiceInput::Internal::VoiceSettings
    \internal

    \brief The VoiceSettings class holds the voice input settings, which are
    saved as soon as they change.
*/

VoiceSettings &settings()
{
    static GuardedObject<VoiceSettings> theSettings;
    return theSettings;
}

VoiceSettings::VoiceSettings()
{
    setSettingsGroup("VoiceInput");

    model.setSettingsKey("Model");
    model.setLabelText(Tr::tr("Model:"));
    model.setComboBoxEditable(false);
    model.setFillCallback(&fillModelItems);

    qtResampling.setSettingsKey("QtResampling");

    holdToRecord.setSettingsKey("HoldToRecord");
    holdToRecord.setDefaultValue(true);
    holdToRecord.setLabelText(Tr::tr("Hold to Record"));
    holdToRecord.setToolTip(Tr::tr("Records while you hold the button down. Otherwise, a click "
                                   "starts recording and the next one stops it."));

    saveRecordings.setSettingsKey("SaveRecordings");
    saveRecordings.setLabelText(Tr::tr("Save Recordings"));

    readSettings();
    connect(this, &AspectContainer::changed, this, &AspectContainer::writeSettings);
}

} // namespace VoiceInput::Internal
