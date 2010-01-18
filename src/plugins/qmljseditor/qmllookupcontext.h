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

#include <qmljs/qmljstypesystem.h>
#include <qmljs/parser/qmljsastvisitor_p.h>
#include <qmljs/qmljsdocument.h>
#include <qmljs/qmljssymbol.h>

#include <QStack>

namespace QmlJSEditor {
namespace Internal {

class QmlLookupContext
{
public:
    QmlLookupContext(const QStack<QmlJS::Symbol *> &scopes,
                     const QmlJS::Document::Ptr &doc,
                     const QmlJS::Snapshot &snapshot,
                     QmlJS::TypeSystem *typeSystem);

    QmlJS::Symbol *resolve(const QString &name);
    QmlJS::Symbol *resolveType(const QString &name)
    { return resolveType(name, _doc->fileName()); }
    QmlJS::Symbol *resolveType(QmlJS::AST::UiQualifiedId *name)
    { return resolveType(toString(name), _doc->fileName()); }

    QmlJS::Document::Ptr document() const
    { return _doc; }

    QList<QmlJS::Symbol*> visibleSymbolsInScope();
    QList<QmlJS::Symbol*> visibleTypes();

    QList<QmlJS::Symbol*> expandType(QmlJS::Symbol *symbol);

private:
    QmlJS::Symbol *resolveType(const QString &name, const QString &fileName);
    QmlJS::Symbol *resolveProperty(const QString &name, QmlJS::Symbol *scope, const QString &fileName);
    QmlJS::Symbol *resolveBuildinType(const QString &name);

    static QString toString(QmlJS::AST::UiQualifiedId *id);

private:
    QStack<QmlJS::Symbol *> _scopes;
    QmlJS::Document::Ptr _doc;
    QmlJS::Snapshot _snapshot;
    QmlJS::TypeSystem *m_typeSystem;
};

} // namespace Internal
} // namespace QmlJSEditor

#endif // QMLLOOKUPCONTEXT_H
