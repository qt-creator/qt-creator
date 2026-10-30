// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "voiceinput_global.h"

#include <QObject>

#include <functional>

QT_BEGIN_NAMESPACE
class QWidget;
QT_END_NAMESPACE

namespace QtTaskTree { class QTaskTree; }

namespace VoiceInput {

class TranscriberPrivate;

class VOICEINPUT_EXPORT Transcriber : public QObject
{
    Q_OBJECT

public:
    enum class State { Idle, Recording, Finishing };

    explicit Transcriber(QObject *parent = nullptr);
    ~Transcriber() override;

    State state() const;
    bool isLoadingModel() const;

    void start(QWidget *dialogParent);
    void stop();

    static bool hasModel();
    static bool holdsToRecord();
    static void setDownloadProgressHandler(
        const std::function<void(QtTaskTree::QTaskTree *taskTree, const QString &title)> &handler);
    static QString modelName();
    static void showSettingsMenu(QWidget *parent,
                                 const QPoint &globalPos,
                                 const std::function<void()> &onChanged = {});

signals:
    void stateChanged(VoiceInput::Transcriber::State state);
    void loadingModelChanged(bool loading);
    void transcriptChanged(const QString &text);
    void levelMeasured(qint64 firstSample, float level);
    void errorOccurred(const QString &message);

private:
    friend class TranscriberPrivate;
    TranscriberPrivate *d;
};

} // namespace VoiceInput
