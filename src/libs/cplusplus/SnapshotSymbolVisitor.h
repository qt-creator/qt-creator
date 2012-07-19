/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
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
**
**************************************************************************/

#ifndef CPLUSPLUS_SNAPSHOTSYMBOLVISITOR_H
#define CPLUSPLUS_SNAPSHOTSYMBOLVISITOR_H

#include "CppDocument.h"
#include <SymbolVisitor.h>
#include <QSet>

namespace CPlusPlus {

class CPLUSPLUS_EXPORT SnapshotSymbolVisitor : public CPlusPlus::SymbolVisitor
{
public:
    SnapshotSymbolVisitor(const Snapshot &snapshot);

    void accept(Document::Ptr doc);
    using SymbolVisitor::accept;

protected:
    void accept(Document::Ptr doc, QSet<QString> *processed);

private:
    Snapshot _snapshot;
    Document::Ptr _document;
};

} // namespace CPlusPlus

#endif // CPLUSPLUS_SNAPSHOTSYMBOLVISITOR_H
