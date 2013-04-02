/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#ifndef CPPTOOLS_TYPEHIERARCHYBUILDER_H
#define CPPTOOLS_TYPEHIERARCHYBUILDER_H

#include "cpptools_global.h"
#include "cppmodelmanagerinterface.h"

#include <cplusplus/Overview.h>

#include <QList>
#include <QStringList>
#include <QSet>

namespace CppTools {

class CPPTOOLS_EXPORT TypeHierarchy
{
    friend class TypeHierarchyBuilder;

public:
    TypeHierarchy();
    TypeHierarchy(CPlusPlus::Symbol *symbol);

    CPlusPlus::Symbol *symbol() const;
    const QList<TypeHierarchy> &hierarchy() const;

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
    void buildDerived(TypeHierarchy *typeHierarchy);

    CPlusPlus::Symbol *_symbol;
    CPlusPlus::Snapshot _snapshot;
    QStringList _dependencies;
    QSet<CPlusPlus::Symbol *> _visited;
    QHash<QString, QSet<QString> > _candidates;
    CPlusPlus::Overview _overview;
};

} // CppTools

#endif // CPPTOOLS_TYPEHIERARCHYBUILDER_H
