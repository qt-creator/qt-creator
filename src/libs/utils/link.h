// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include "filepath.h"

#include <QMetaType>
#include <QString>

#include <functional>

namespace Utils {

class QTCREATOR_UTILS_EXPORT Link
{
public:
    Link(const FilePath &filePath = FilePath(), int line = 0, int column = 0)
        : targetFilePath(filePath)
        , targetLine(line)
        , targetColumn(column)
    {}

    static Link fromString(const QString &filePathWithNumbers, bool canContainLineNumber = false);

    bool hasValidTarget() const
    { return !targetFilePath.isEmpty(); }

    bool hasValidLinkText() const
    { return linkTextStart != linkTextEnd; }

    bool operator==(const Link &other) const
    {
        return targetFilePath == other.targetFilePath
                && targetLine == other.targetLine
                && targetColumn == other.targetColumn
                && linkTextStart == other.linkTextStart
                && linkTextEnd == other.linkTextEnd;
    }
    bool operator!=(const Link &other) const { return !(*this == other); }

    friend size_t qHash(const Link &l, uint seed = 0)
    { return qHashMulti(seed, l.targetFilePath, l.targetLine, l.targetColumn); }

    int linkTextStart = -1;
    int linkTextEnd = -1;

    FilePath targetFilePath;
    int targetLine;
    int targetColumn;
};

using LinkHandler = std::function<void(const Link &)>;
using Links = QList<Link>;

} // namespace Utils

Q_DECLARE_METATYPE(Utils::Link)
