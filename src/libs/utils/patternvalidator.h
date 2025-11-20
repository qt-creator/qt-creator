// Copyright (C) 2025 Andre Hartmann.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QValidator>

/**
 * Replaces \a forbiddenChars with \a replacementChar while typing to avoid invalid pattern.
 */
class PatternValidator : public QValidator {
public:
    PatternValidator(QChar replacementChar, const QList<QChar> &forbiddenChars, QObject *parent = nullptr)
        : QValidator(parent), m_replacementChar(replacementChar), m_forbiddenChars(forbiddenChars) {}

    State validate(QString &input, int &) const override
    {
        for (QChar c : m_forbiddenChars)
            input.replace(c, m_replacementChar);

        return QValidator::Acceptable;
    }

private:
    const QChar m_replacementChar;
    const QList<QChar> m_forbiddenChars;
};
