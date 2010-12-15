/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "SnapshotSymbolVisitor.h"
#include <Symbols.h>

using namespace CPlusPlus;

SnapshotSymbolVisitor::SnapshotSymbolVisitor(const Snapshot &snapshot)
    : _snapshot(snapshot)
{
}

void SnapshotSymbolVisitor::accept(Document::Ptr doc)
{
    QSet<QString> processed;
    accept(doc, &processed);
}

void SnapshotSymbolVisitor::accept(Document::Ptr doc, QSet<QString> *processed)
{
    if (doc && doc->globalNamespace() && ! processed->contains(doc->fileName())) {
        processed->insert(doc->fileName());

        foreach (const Document::Include &i, doc->includes()) {
            if (Document::Ptr incl = _snapshot.document(i.fileName()))
                accept(incl, processed);
        }

        std::swap(_document, doc);
        accept(_document->globalNamespace());
        std::swap(_document, doc);
    }
}
