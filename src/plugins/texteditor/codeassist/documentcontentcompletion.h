// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "completionassistprovider.h"

#include "texteditor/texteditorconstants.h"

namespace TextEditor {

class TEXTEDITOR_EXPORT DocumentContentCompletionProvider : public CompletionAssistProvider
{
    Q_OBJECT

public:
    DocumentContentCompletionProvider(
            const QString &snippetGroup = QString(Constants::TEXT_SNIPPET_GROUP_ID));

    IAssistProcessor *createProcessor(const AssistInterface *interface) const override;

private:
    QString m_snippetGroup;
};

} // TextEditor
