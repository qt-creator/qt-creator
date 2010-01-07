/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef QMLLOOKUPCONTEXT_H
#define QMLLOOKUPCONTEXT_H

#include <QStack>

#include "qmldocument.h"
#include "qmljsastvisitor_p.h"
#include "qmlsymbol.h"

namespace QmlEditor {
namespace Internal {

class QmlLookupContext
{
public:
    QmlLookupContext(const QStack<QmlJS::AST::Node *> &scopes,
                     const QmlDocument::Ptr &doc,
                     const Snapshot &snapshot);
    ~QmlLookupContext();

    QmlSymbol *resolve(const QString &name);
    QmlSymbol *resolveType(const QString &name)
    { return resolveType(name, _doc->fileName()); }
    QmlSymbol *resolveType(QmlJS::AST::UiQualifiedId *name)
    { return resolveType(toString(name), _doc->fileName()); }

    QmlDocument::Ptr document() const
    { return _doc; }

    QList<QmlSymbol*> visibleSymbols(QmlJS::AST::Node *scope);
    QList<QmlSymbol*> visibleTypes();

private:
    QmlSymbol *createSymbol(const QString &fileName, QmlJS::AST::UiObjectMember *node);

    QmlSymbol *resolveType(const QString &name, const QString &fileName);
    QmlSymbol *resolveProperty(const QString &name, QmlJS::AST::Node *scope, const QString &fileName);
    QmlSymbol *resolveProperty(const QString &name, QmlJS::AST::UiObjectInitializer *initializer, const QString &fileName);

    static QString toString(QmlJS::AST::UiQualifiedId *id);

private:
    QStack<QmlJS::AST::Node *> _scopes;
    QmlDocument::Ptr _doc;
    Snapshot _snapshot;
    QList<QmlSymbol*> _temporarySymbols;
};

} // namespace Internal
} // namespace QmlEditor

#endif // QMLLOOKUPCONTEXT_H
