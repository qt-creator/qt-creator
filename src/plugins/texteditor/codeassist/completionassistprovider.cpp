// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "completionassistprovider.h"

#include <QChar>

using namespace TextEditor;

CompletionAssistProvider::CompletionAssistProvider(QObject *parent)
    : IAssistProvider(parent)
{}

CompletionAssistProvider::~CompletionAssistProvider() = default;

int CompletionAssistProvider::activationCharSequenceLength() const
{
    return 0;
}

bool CompletionAssistProvider::isActivationCharSequence(const QString &sequence) const
{
    Q_UNUSED(sequence)
    return false;
}

bool CompletionAssistProvider::isContinuationChar(const QChar &c) const
{
    return c.isLetterOrNumber() || c == QLatin1Char('_');
}
