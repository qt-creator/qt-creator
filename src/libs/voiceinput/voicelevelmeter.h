// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "voiceinput_global.h"

#include <QWidget>

namespace VoiceInput {

class Transcriber;
class VoiceLevelMeterPrivate;

class VOICEINPUT_EXPORT VoiceLevelMeter : public QWidget
{
    Q_OBJECT

public:
    explicit VoiceLevelMeter(Transcriber *transcriber, QWidget *parent = nullptr);
    ~VoiceLevelMeter() override;

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    VoiceLevelMeterPrivate *d;
};

} // namespace VoiceInput
