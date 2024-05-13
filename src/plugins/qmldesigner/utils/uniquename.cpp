// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "uniquename.h"

#include <QRegularExpression>

namespace QmlDesigner {

/**
 * @brief Gets a unique name from a give name
 * @param oldName input name
 * @param predicate function to check if the name is unique. Retuns true if name is unique
 * @return a unique name
 */
// static
QString UniqueName::get(const QString &oldName, std::function<bool(const QString &)> predicate)
{
    QString newName = oldName;
    while (!predicate(newName))
        newName = nextName(newName);

    return newName;
}

// static
QString UniqueName::nextName(const QString &oldName)
{
    static QRegularExpression rgx("\\d+$"); // matches a number at the end of a string

    QString uniqueName = oldName;
    // if the name ends with a number, increment it
    QRegularExpressionMatch match = rgx.match(uniqueName);
    if (match.hasMatch()) { // ends with a number
        QString numStr = match.captured(0);
        int num = match.captured(0).toInt();

        // get number of padding zeros, ex: for "005" = 2
        int nPaddingZeros = 0;
        for (; nPaddingZeros < numStr.size() && numStr[nPaddingZeros] == '0'; ++nPaddingZeros);

        ++num;

        // if the incremented number's digits increased, decrease the padding zeros
        if (std::fmod(std::log10(num), 1.0) == 0)
            --nPaddingZeros;

        uniqueName = oldName.mid(0, match.capturedStart())
                     + QString('0').repeated(nPaddingZeros)
                     + QString::number(num);
    } else {
        uniqueName = oldName + '1';
    }

    return uniqueName;
}

} // namespace QmlDesigner
