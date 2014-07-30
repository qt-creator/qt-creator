/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef CPPWORKINGCOPY_H
#define CPPWORKINGCOPY_H

#include "cpptools_global.h"

#include <QHash>
#include <QString>
#include <QPair>

namespace CppTools {

class CPPTOOLS_EXPORT WorkingCopy
{
public:
    WorkingCopy();

    void insert(const QString &fileName, const QByteArray &source, unsigned revision = 0)
    { _elements.insert(fileName, qMakePair(source, revision)); }

    bool contains(const QString &fileName) const
    { return _elements.contains(fileName); }

    QByteArray source(const QString &fileName) const
    { return _elements.value(fileName).first; }

    QPair<QByteArray, unsigned> get(const QString &fileName) const
    { return _elements.value(fileName); }

    QHashIterator<QString, QPair<QByteArray, unsigned> > iterator() const
    { return QHashIterator<QString, QPair<QByteArray, unsigned> >(_elements); }

    int size() const
    { return _elements.size(); }

private:
    typedef QHash<QString, QPair<QByteArray, unsigned> > Table;
    Table _elements;
};

} // namespace CppTools

#endif // CPPWORKINGCOPY_H
