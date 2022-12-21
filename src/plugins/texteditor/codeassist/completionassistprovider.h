// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "iassistprovider.h"


namespace TextEditor {

class TEXTEDITOR_EXPORT CompletionAssistProvider : public IAssistProvider
{
    Q_OBJECT

public:
    CompletionAssistProvider(QObject *parent = nullptr);
    ~CompletionAssistProvider() override;

    virtual int activationCharSequenceLength() const;
    virtual bool isActivationCharSequence(const QString &sequence) const;
    virtual bool isContinuationChar(const QChar &c) const;
};

} // TextEditor
