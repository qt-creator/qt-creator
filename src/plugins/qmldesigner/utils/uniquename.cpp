// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "uniquename.h"

#include <QFileInfo>
#include <QRegularExpression>

namespace QmlDesigner {

/**
 * @brief Generates a unique name based on the provided name.
 *
 * This method iteratively generates a name by appending suffixes until a unique name is found.
 * The uniqueness of the generated name is determined by the provided predicate function.
 *
 * @param oldName The original name to be made unique.
 * @param predicate A function that checks if a name is unique. Returns true if the name is unique,
 *        false otherwise.
 * @return A unique name derived from the provided name.
 */
QString UniqueName::get(const QString &oldName, std::function<bool(const QString &)> predicate)
{
    QString newName = oldName;
    while (!predicate(newName))
        newName = nextName(newName);

    return newName;
}

/**
 * @brief Generates a unique path based on the provided path. If the path belongs to a file, the
 * filename or if it's a directory, the directory name will be adjusted to ensure uniqueness.
 *
 * This method appends a numerical suffix (or increment it if it exists) to the filename or
 * directory name if necessary to make it unique.
 *
 * @param path The original path to be made unique.
 * @return A unique path derived from the provided path.
 */
QString UniqueName::getPath(const QString &path)
{
    // Remove the trailing slash if it exists (otherwise QFileInfo::path() returns empty)
    QString adjustedPath = path;
    if (adjustedPath.endsWith('/'))
        adjustedPath.chop(1);

    QFileInfo fileInfo = QFileInfo(adjustedPath);
    QString baseName = fileInfo.baseName();
    QString suffix = fileInfo.completeSuffix();
    if (!suffix.isEmpty())
        suffix.prepend('.');

    QString parentDir = fileInfo.path();
    QString pathTemplate = parentDir + "/%1" + suffix;

    QString uniqueBaseName = UniqueName::get(baseName, [&] (const QString &currName) {
        return !QFileInfo::exists(pathTemplate.arg(currName));
    });

    return pathTemplate.arg(uniqueBaseName);
}

QString UniqueName::nextName(const QString &oldName)
{
    static QRegularExpression rgx("\\d+$"); // match a number at the end of a string

    QString uniqueName = oldName;
    // if the name ends with a number, increment it
    QRegularExpressionMatch match = rgx.match(uniqueName);
    if (match.hasMatch()) { // ends with a number
        QString numStr = match.captured(0);
        int num = numStr.toInt() + 1;

        // get number of padding zeros, ex: for "005" = 2
        int nPaddingZeros = 0;
        for (; nPaddingZeros < numStr.size() && numStr[nPaddingZeros] == '0'; ++nPaddingZeros);

        // if the incremented number's digits increased, decrease the padding zeros
        if (std::fmod(std::log10(num), 1.0) == 0)
            --nPaddingZeros;

        uniqueName = oldName.left(match.capturedStart())
                     + QString('0').repeated(nPaddingZeros)
                     + QString::number(num);
    } else {
        uniqueName = oldName + '1';
    }

    return uniqueName;
}

} // namespace QmlDesigner
