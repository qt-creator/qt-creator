/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#ifndef CPLUSPLUS_DEPENDENCYTABLE_H
#define CPLUSPLUS_DEPENDENCYTABLE_H

#include <CPlusPlusForwardDeclarations.h>

#include <QtCore/QBitArray>
#include <QtCore/QHash>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QVector>

namespace CPlusPlus {

class Snapshot;

class CPLUSPLUS_EXPORT DependencyTable
{
public:
    bool isValidFor(const Snapshot &snapshot) const;

    QStringList filesDependingOn(const QString &fileName) const;
    QHash<QString, QStringList> dependencyTable() const;

    void build(const Snapshot &snapshot);

private:
    QHash<QString, QStringList> includesPerFile;
    QVector<QString> files;
    QHash<QString, int> fileIndex;
    QHash<int, QList<int> > includes;
    QVector<QBitArray> includeMap;
};

} // namespace CPlusPlus

#endif // CPLUSPLUS_DEPENDENCYTABLE_H
