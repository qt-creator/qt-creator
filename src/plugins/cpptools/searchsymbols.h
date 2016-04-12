/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include "cpptools_global.h"
#include "cppindexingsupport.h"
#include "indexitem.h"
#include "stringtable.h"

#include <cplusplus/CppDocument.h>
#include <cplusplus/Overview.h>

#include <QString>
#include <QSet>
#include <QHash>

namespace CppTools {

class SearchSymbols: protected CPlusPlus::SymbolVisitor
{
public:
    typedef SymbolSearcher::SymbolTypes SymbolTypes;

    static SymbolTypes AllTypes;

    SearchSymbols(Internal::StringTable &stringTable);

    void setSymbolsToSearchFor(const SymbolTypes &types);

    IndexItem::Ptr operator()(CPlusPlus::Document::Ptr doc)
    { return operator()(doc, QString()); }

    IndexItem::Ptr operator()(CPlusPlus::Document::Ptr doc, const QString &scope);

protected:
    using SymbolVisitor::visit;

    void accept(CPlusPlus::Symbol *symbol)
    { CPlusPlus::Symbol::visitSymbol(symbol, this); }

    virtual bool visit(CPlusPlus::UsingNamespaceDirective *);
    virtual bool visit(CPlusPlus::UsingDeclaration *);
    virtual bool visit(CPlusPlus::NamespaceAlias *);
    virtual bool visit(CPlusPlus::Declaration *);
    virtual bool visit(CPlusPlus::Argument *);
    virtual bool visit(CPlusPlus::TypenameArgument *);
    virtual bool visit(CPlusPlus::BaseClass *);
    virtual bool visit(CPlusPlus::Enum *);
    virtual bool visit(CPlusPlus::Function *);
    virtual bool visit(CPlusPlus::Namespace *);
    virtual bool visit(CPlusPlus::Template *);
    virtual bool visit(CPlusPlus::Class *);
    virtual bool visit(CPlusPlus::Block *);
    virtual bool visit(CPlusPlus::ForwardClassDeclaration *);

    // Objective-C
    virtual bool visit(CPlusPlus::ObjCBaseClass *);
    virtual bool visit(CPlusPlus::ObjCBaseProtocol *);
    virtual bool visit(CPlusPlus::ObjCClass *symbol);
    virtual bool visit(CPlusPlus::ObjCForwardClassDeclaration *);
    virtual bool visit(CPlusPlus::ObjCProtocol *symbol);
    virtual bool visit(CPlusPlus::ObjCForwardProtocolDeclaration *);
    virtual bool visit(CPlusPlus::ObjCMethod *symbol);
    virtual bool visit(CPlusPlus::ObjCPropertyDeclaration *symbol);

    QString scopedSymbolName(const QString &symbolName, const CPlusPlus::Symbol *symbol) const;
    QString scopedSymbolName(const CPlusPlus::Symbol *symbol) const;
    QString scopeName(const QString &name, const CPlusPlus::Symbol *symbol) const;
    IndexItem::Ptr addChildItem(const QString &symbolName, const QString &symbolType,
                                const QString &symbolScope, IndexItem::ItemType type,
                                CPlusPlus::Symbol *symbol);

private:
    template<class T> void processClass(T *clazz);
    template<class T> void processFunction(T *func);

private:
    QString findOrInsert(const QString &s)
    { return strings.insert(s); }

    Internal::StringTable &strings;            // Used to avoid QString duplication

    IndexItem::Ptr _parent;
    QString _scope;
    CPlusPlus::Overview overview;
    SymbolTypes symbolsToSearchFor;
    QHash<const CPlusPlus::StringLiteral *, QString> m_paths;
};

} // namespace CppTools

Q_DECLARE_OPERATORS_FOR_FLAGS(CppTools::SearchSymbols::SymbolTypes)
