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
    Link() = default;
    Link(const FilePath &filePath, int line = 0, int column = 0)
        : targetFilePath(filePath)
        , targetLine(line)
        , targetColumn(column)
    {}

    static Link fromString(const QString &filePathWithNumbers, bool canContainLineNumber = false);

    bool hasValidTarget() const
    {
        if (!targetFilePath.isEmpty())
            return true;
        return !targetFilePath.scheme().isEmpty() || !targetFilePath.host().isEmpty();
    }

    bool hasValidLinkText() const
    { return linkTextStart != linkTextEnd; }

    bool operator==(const Link &other) const
    {
        return hasSameLocation(other)
               && linkTextStart == other.linkTextStart
               && linkTextEnd == other.linkTextEnd;
    }
    bool operator!=(const Link &other) const { return !(*this == other); }

    bool hasSameLocation(const Link &other) const
    {
        return targetFilePath == other.targetFilePath
               && targetLine == other.targetLine
               && targetColumn == other.targetColumn;
    }

    friend size_t qHash(const Link &l, uint seed = 0)
    { return qHashMulti(seed, l.targetFilePath, l.targetLine, l.targetColumn); }

    int linkTextStart = -1;
    int linkTextEnd = -1;

    FilePath targetFilePath;
    int targetLine = 0;
    int targetColumn = 0;
};

using LinkHandler = std::function<void(const Link &)>;
using Links = QList<Link>;

} // namespace Utils

Q_DECLARE_METATYPE(Utils::Link)

namespace std {

template<> struct hash<Utils::Link>
{
    size_t operator()(const Utils::Link &fn) const { return qHash(fn); }
};

} // std
