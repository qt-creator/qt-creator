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

#include "cpptools_global.h"

#include <utils/fileutils.h>

#include <QHash>
#include <QString>
#include <QPair>

namespace CppTools {

class CPPTOOLS_EXPORT WorkingCopy
{
public:
    WorkingCopy();

    void insert(const QString &fileName, const QByteArray &source, unsigned revision = 0)
    { insert(Utils::FilePath::fromString(fileName), source, revision); }

    void insert(const Utils::FilePath &fileName, const QByteArray &source, unsigned revision = 0)
    { _elements.insert(fileName, qMakePair(source, revision)); }

    bool contains(const QString &fileName) const
    { return contains(Utils::FilePath::fromString(fileName)); }

    bool contains(const Utils::FilePath &fileName) const
    { return _elements.contains(fileName); }

    QByteArray source(const QString &fileName) const
    { return source(Utils::FilePath::fromString(fileName)); }

    QByteArray source(const Utils::FilePath &fileName) const
    { return _elements.value(fileName).first; }

    unsigned revision(const QString &fileName) const
    { return revision(Utils::FilePath::fromString(fileName)); }

    unsigned revision(const Utils::FilePath &fileName) const
    { return _elements.value(fileName).second; }

    QPair<QByteArray, unsigned> get(const QString &fileName) const
    { return get(Utils::FilePath::fromString(fileName)); }

    QPair<QByteArray, unsigned> get(const Utils::FilePath &fileName) const
    { return _elements.value(fileName); }

    using Table = QHash<Utils::FilePath, QPair<QByteArray, unsigned> >;
    const Table &elements() const
    { return _elements; }

    int size() const
    { return _elements.size(); }

private:
    Table _elements;
};

} // namespace CppTools
