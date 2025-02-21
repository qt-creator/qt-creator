// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include "filepath.h"

namespace Utils {

class QTCREATOR_UTILS_EXPORT Link
{
public:
    Link() = default;
    Link(const FilePath &filePath, int line = 0, int column = 0);

    static Link fromString(const QString &filePathWithNumbers, bool canContainLineNumber = false);

    bool hasValidTarget() const;
    bool hasValidLinkText() const { return linkTextStart != linkTextEnd; }

    bool hasSameLocation(const Link &other) const;

    int linkTextStart = -1;
    int linkTextEnd = -1;

    FilePath targetFilePath;
    int targetLine = 0;
    int targetColumn = 0;

private:
    QTCREATOR_UTILS_EXPORT friend bool operator<(const Link &first, const Link &second);
    QTCREATOR_UTILS_EXPORT friend bool operator==(const Link &lhs, const Link &rhs);
    friend bool operator!=(const Link &lhs, const Link &rhs) { return !(lhs == rhs); }

    QTCREATOR_UTILS_EXPORT friend QDebug operator<<(QDebug dbg, const Link &link);

    friend size_t qHash(const Link &l, uint seed = 0)
    { return qHashMulti(seed, l.targetFilePath, l.targetLine, l.targetColumn); }
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
