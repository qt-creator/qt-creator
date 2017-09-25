/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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

#include "mocksqlitereadstatement.h"

template <>
std::vector<FilePathIndex> MockSqliteReadStatement::values<FilePathIndex>(std::size_t reserveSize)
{
    return valuesReturnStdVectorInt(reserveSize);
}

template <>
std::vector<Location>
MockSqliteReadStatement::values<Location, 3>(std::size_t reserveSize,
                                             const Utils::PathString &sourcePath,
                                             const uint &line,
                                             const uint &column)
{
    return valuesReturnStdVectorLocation(reserveSize, sourcePath, line, column);
}

template <>
std::vector<Source>
MockSqliteReadStatement::values<Source, 2>(std::size_t reserveSize, const std::vector<qint64> &sourceIds)
{
    return valuesReturnStdVectorSource(reserveSize, sourceIds);
}

template <>
Utils::optional<uint32_t>
MockSqliteReadStatement::value<uint32_t>(const Utils::SmallStringView &text)
{
    return valueReturnUInt32(text);
}
