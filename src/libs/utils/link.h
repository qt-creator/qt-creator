/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include "filepath.h"
#include "porting.h"

#include <QString>
#include <qmetatype.h>

#include <functional>


namespace Utils {

class QTCREATOR_UTILS_EXPORT Link
{
public:
    Link(const Utils::FilePath &filePath = Utils::FilePath(), int line = 0, int column = 0)
        : targetFilePath(filePath)
        , targetLine(line)
        , targetColumn(column)
    {}

    static Link fromString(const QString &fileName,
                           bool canContainLineNumber = false,
                           QString *postfix = nullptr);
    static Link fromFilePath(const FilePath &filePath,
                             bool canContainLineNumber = false,
                             QString *postfix = nullptr);

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

    int linkTextStart = -1;
    int linkTextEnd = -1;

    Utils::FilePath targetFilePath;
    int targetLine;
    int targetColumn;
};

QTCREATOR_UTILS_EXPORT QHashValueType qHash(const Link &l);

using ProcessLinkCallback = std::function<void(const Link &)>;
using Links = QList<Link>;

} // namespace Utils

Q_DECLARE_METATYPE(Utils::Link)
