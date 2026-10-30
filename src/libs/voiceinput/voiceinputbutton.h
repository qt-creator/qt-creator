// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "voiceinput_global.h"

#include <utils/qtdesignwidgets.h>

namespace Utils { class PlainTextEdit; }

namespace VoiceInput {

class Transcriber;
class VoiceInputButtonPrivate;

class VOICEINPUT_EXPORT VoiceInputButton : public Utils::QtcIconButton
{
    Q_OBJECT

public:
    explicit VoiceInputButton(QWidget *parent = nullptr);
    ~VoiceInputButton() override;

    void setTextEdit(Utils::PlainTextEdit *textEdit);
    Transcriber *transcriber() const;

signals:
    void errorOccurred(const QString &message);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    friend class VoiceInputButtonPrivate;
    VoiceInputButtonPrivate *d;
};

} // namespace VoiceInput
