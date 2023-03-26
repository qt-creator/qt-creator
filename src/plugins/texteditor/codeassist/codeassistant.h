// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "assistenums.h"

#include <texteditor/texteditor_global.h>

#include <QObject>

namespace TextEditor {

class CodeAssistantPrivate;
class IAssistProvider;
class TextEditorWidget;

class CodeAssistant : public QObject
{
    Q_OBJECT

public:
    CodeAssistant(TextEditorWidget *editorWidget);
    ~CodeAssistant() override;

    void process();
    void notifyChange();
    bool hasContext() const;
    void destroyContext();

    QVariant userData() const;
    void setUserData(const QVariant &data);

    void invoke(AssistKind assistKind, IAssistProvider *provider = nullptr);

signals:
    void finished();

private:
    CodeAssistantPrivate *d;
};

} //TextEditor
