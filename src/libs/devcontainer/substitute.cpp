// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "substitute.h"

#include "devcontainertr.h"

#include <QRegularExpression>

namespace DevContainer::Internal {

void substituteVariables(QString &str, const Replacers &replacers)
{
    QRegularExpression re("\\$\\{([^}]+)\\}");
    QRegularExpressionMatchIterator it = re.globalMatch(str);

    struct Replace
    {
        qsizetype start;
        qsizetype length;
        QString replacement;
    };
    QList<Replace> replacements;

    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        QString varName = match.captured(1);
        QStringList parts = varName.split(':');
        QString variableName = parts.takeFirst();

        auto itReplacer = replacers.find(variableName);
        if (itReplacer != replacers.end()) {
            QString replacement = itReplacer.value()(parts);
            replacements.append({match.capturedStart(), match.capturedLength(), replacement});
        }
    }

    // Apply replacements in reverse order to avoid messing up indices
    for (auto it = replacements.crbegin(); it != replacements.crend(); ++it)
        str.replace(it->start, it->length, std::move(it->replacement));
}

} // namespace DevContainer::Internal
