// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cmakeutils.h"

#include "cmakeprojectconstants.h"

#include <utils/algorithm.h>
#include <utils/qtcassert.h>
#include <utils/stringutils.h>
#include <utils/theme/theme.h>

#include <QColor>
#include <QRegularExpression>
#include <QRegularExpressionMatchIterator>

using namespace Utils;

namespace CMakeProjectManager::Internal {

static QColor ansiColor(uint code)
{
    QTC_ASSERT(code < 8, return QColor());
    const int red   = code & 1 ? 170 : 0;
    const int green = code & 2 ? 170 : 0;
    const int blue  = code & 4 ? 170 : 0;
    return QColor(red, green, blue);
}

static QColor extractAnsiForeground(const QString &str)
{
    static QRegularExpression ansiRe(R"(\x1b\[([0-9;]*)m)");
    QRegularExpressionMatchIterator it = ansiRe.globalMatch(str);
    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        QStringList parts = match.captured(1).split(';');
        for (int i = 0; i < parts.size(); ++i) {
            bool ok;
            uint code = parts.at(i).toUInt(&ok);
            if (!ok)
                continue;
            if (code >= 30 && code <= 37)
                return ansiColor(code - 30);
            if (code == 38 && i + 1 < parts.size()) {
                bool subOk;
                uint subCode = parts.at(i + 1).toUInt(&subOk);
                if (subOk && subCode == 2 && i + 4 <= parts.size()) {
                    return QColor(
                        parts.at(i + 2).toUInt(),
                        parts.at(i + 3).toUInt(),
                        parts.at(i + 4).toUInt());
                }
                if (subOk && subCode == 5 && i + 2 < parts.size()) {
                    uint index = parts.at(i + 2).toUInt();
                    if (index < 8)
                        return ansiColor(index);
                    if (index < 16)
                        return ansiColor(index - 8).lighter(150);
                    if (index < 232) {
                        uint o = index - 16;
                        return QColor((o / 36) * 51, ((o / 6) % 6) * 51, (o % 6) * 51);
                    }
                    return QColor((index - 232) * 11, (index - 232) * 11, (index - 232) * 11);
                }
            }
        }
    }
    return {};
}

static bool hasAnsiColor(const QString &str)
{
    static QRegularExpression colorRe(R"(\x1b\[(?:[32][0-9;]*|38;[25];)m)");
    return colorRe.match(str).hasMatch();
}

static bool hasAnsiReset(const QString &str)
{
    static QRegularExpression resetRe(R"(\x1b\[0m)");
    return resetRe.match(str).hasMatch();
}

QString addCMakePrefix(const QString &str)
{
    static const QString prefix
        = Utils::ansiColoredText(Constants::OUTPUT_PREFIX, creatorColor(Theme::Token_Text_Muted));
    static QColor cachedColor = QColor::Invalid;

    if (!str.contains(QLatin1Char('\x1b')))
        return prefix
               + (cachedColor.isValid() ? Utils::ansiColoredText(str, cachedColor) : str);

    const bool hasColor = hasAnsiColor(str);
    const bool hasReset = hasAnsiReset(str);
    if (hasColor && !hasReset)
        cachedColor = extractAnsiForeground(str);
    else if (hasReset)
        cachedColor = QColor::Invalid;

    return prefix + (cachedColor.isValid() ? Utils::ansiColoredText(str, cachedColor) : str);
}

QStringList addCMakePrefix(const QStringList &list)
{
    return Utils::transform(list, [](const QString &str) { return addCMakePrefix(str); });
}

} // CMakeProjectManager::Internal
