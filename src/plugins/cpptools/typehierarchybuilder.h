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

#ifndef CPPTOOLS_TYPEHIERARCHYBUILDER_H
#define CPPTOOLS_TYPEHIERARCHYBUILDER_H

#include "cpptools_global.h"

#include <cplusplus/CppDocument.h>
#include <cplusplus/Overview.h>

#include <QList>
#include <QSet>

QT_FORWARD_DECLARE_CLASS(QStringList)

namespace CppTools {

class CPPTOOLS_EXPORT TypeHierarchy
{
    friend class TypeHierarchyBuilder;

public:
    TypeHierarchy();
    TypeHierarchy(CPlusPlus::Symbol *symbol);

    CPlusPlus::Symbol *symbol() const;
    const QList<TypeHierarchy> &hierarchy() const;

    bool operator==(const TypeHierarchy &other) const
    { return _symbol == other._symbol; }

private:
    CPlusPlus::Symbol *_symbol;
    QList<TypeHierarchy> _hierarchy;
};

class CPPTOOLS_EXPORT TypeHierarchyBuilder
{
public:
    TypeHierarchyBuilder(CPlusPlus::Symbol *symbol, const CPlusPlus::Snapshot &snapshot);

    TypeHierarchy buildDerivedTypeHierarchy();

private:
    void reset();
    void buildDerived(TypeHierarchy *typeHierarchy, const QStringList &dependencies);
    QStringList filesDependingOn(CPlusPlus::Symbol *symbol) const;

    CPlusPlus::Symbol *_symbol;
    CPlusPlus::Snapshot _snapshot;
    QSet<CPlusPlus::Symbol *> _visited;
    QHash<QString, QSet<QString> > _candidates;
    CPlusPlus::Overview _overview;
};

} // CppTools

#endif // CPPTOOLS_TYPEHIERARCHYBUILDER_H
