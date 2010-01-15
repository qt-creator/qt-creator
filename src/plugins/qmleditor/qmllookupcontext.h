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

#include <qmljs/qmltypesystem.h>
#include <qmljs/parser/qmljsastvisitor_p.h>
#include <qmljs/qmldocument.h>
#include <qmljs/qmlsymbol.h>

#include <QStack>

namespace QmlEditor {
namespace Internal {

class QmlLookupContext
{
public:
    QmlLookupContext(const QStack<Qml::QmlSymbol *> &scopes,
                     const Qml::QmlDocument::Ptr &doc,
                     const Qml::Snapshot &snapshot,
                     Qml::QmlTypeSystem *typeSystem);

    Qml::QmlSymbol *resolve(const QString &name);
    Qml::QmlSymbol *resolveType(const QString &name)
    { return resolveType(name, _doc->fileName()); }
    Qml::QmlSymbol *resolveType(QmlJS::AST::UiQualifiedId *name)
    { return resolveType(toString(name), _doc->fileName()); }

    Qml::QmlDocument::Ptr document() const
    { return _doc; }

    QList<Qml::QmlSymbol*> visibleSymbolsInScope();
    QList<Qml::QmlSymbol*> visibleTypes();

    QList<Qml::QmlSymbol*> expandType(Qml::QmlSymbol *symbol);

private:
    Qml::QmlSymbol *resolveType(const QString &name, const QString &fileName);
    Qml::QmlSymbol *resolveProperty(const QString &name, Qml::QmlSymbol *scope, const QString &fileName);
    Qml::QmlSymbol *resolveBuildinType(const QString &name);

    static QString toString(QmlJS::AST::UiQualifiedId *id);

private:
    QStack<Qml::QmlSymbol *> _scopes;
    Qml::QmlDocument::Ptr _doc;
    Qml::Snapshot _snapshot;
    Qml::QmlTypeSystem *m_typeSystem;
};

} // namespace Internal
} // namespace QmlEditor

#endif // QMLLOOKUPCONTEXT_H
