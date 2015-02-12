/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef CPPWORKINGCOPY_H
#define CPPWORKINGCOPY_H

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
    { insert(Utils::FileName::fromString(fileName), source, revision); }

    void insert(const Utils::FileName &fileName, const QByteArray &source, unsigned revision = 0)
    { _elements.insert(fileName, qMakePair(source, revision)); }

    bool contains(const QString &fileName) const
    { return contains(Utils::FileName::fromString(fileName)); }

    bool contains(const Utils::FileName &fileName) const
    { return _elements.contains(fileName); }

    QByteArray source(const QString &fileName) const
    { return source(Utils::FileName::fromString(fileName)); }

    QByteArray source(const Utils::FileName &fileName) const
    { return _elements.value(fileName).first; }

    unsigned revision(const QString &fileName) const
    { return revision(Utils::FileName::fromString(fileName)); }

    unsigned revision(const Utils::FileName &fileName) const
    { return _elements.value(fileName).second; }

    QPair<QByteArray, unsigned> get(const QString &fileName) const
    { return get(Utils::FileName::fromString(fileName)); }

    QPair<QByteArray, unsigned> get(const Utils::FileName &fileName) const
    { return _elements.value(fileName); }

    QHashIterator<Utils::FileName, QPair<QByteArray, unsigned> > iterator() const
    { return QHashIterator<Utils::FileName, QPair<QByteArray, unsigned> >(_elements); }

    int size() const
    { return _elements.size(); }

private:
    typedef QHash<Utils::FileName, QPair<QByteArray, unsigned> > Table;
    Table _elements;
};

} // namespace CppTools

#endif // CPPWORKINGCOPY_H
