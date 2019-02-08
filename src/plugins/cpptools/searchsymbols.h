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

#include <cplusplus/CppDocument.h>
#include <cplusplus/Overview.h>

#include <QString>
#include <QSet>
#include <QHash>

namespace CppTools {

class SearchSymbols: protected CPlusPlus::SymbolVisitor
{
public:
    using SymbolTypes = SymbolSearcher::SymbolTypes;

    static SymbolTypes AllTypes;

    SearchSymbols();

    void setSymbolsToSearchFor(const SymbolTypes &types);

    IndexItem::Ptr operator()(CPlusPlus::Document::Ptr doc)
    { return operator()(doc, QString()); }

    IndexItem::Ptr operator()(CPlusPlus::Document::Ptr doc, const QString &scope);

protected:
    using SymbolVisitor::visit;

    void accept(CPlusPlus::Symbol *symbol)
    { CPlusPlus::Symbol::visitSymbol(symbol, this); }

    bool visit(CPlusPlus::UsingNamespaceDirective *) override;
    bool visit(CPlusPlus::UsingDeclaration *) override;
    bool visit(CPlusPlus::NamespaceAlias *) override;
    bool visit(CPlusPlus::Declaration *) override;
    bool visit(CPlusPlus::Argument *) override;
    bool visit(CPlusPlus::TypenameArgument *) override;
    bool visit(CPlusPlus::BaseClass *) override;
    bool visit(CPlusPlus::Enum *) override;
    bool visit(CPlusPlus::Function *) override;
    bool visit(CPlusPlus::Namespace *) override;
    bool visit(CPlusPlus::Template *) override;
    bool visit(CPlusPlus::Class *) override;
    bool visit(CPlusPlus::Block *) override;
    bool visit(CPlusPlus::ForwardClassDeclaration *) override;

    // Objective-C
    bool visit(CPlusPlus::ObjCBaseClass *) override;
    bool visit(CPlusPlus::ObjCBaseProtocol *) override;
    bool visit(CPlusPlus::ObjCClass *symbol) override;
    bool visit(CPlusPlus::ObjCForwardClassDeclaration *) override;
    bool visit(CPlusPlus::ObjCProtocol *symbol) override;
    bool visit(CPlusPlus::ObjCForwardProtocolDeclaration *) override;
    bool visit(CPlusPlus::ObjCMethod *symbol) override;
    bool visit(CPlusPlus::ObjCPropertyDeclaration *symbol) override;

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
    IndexItem::Ptr _parent;
    QString _scope;
    CPlusPlus::Overview overview;
    SymbolTypes symbolsToSearchFor;
    QHash<const CPlusPlus::StringLiteral *, QString> m_paths;
};

} // namespace CppTools

Q_DECLARE_OPERATORS_FOR_FLAGS(CppTools::SearchSymbols::SymbolTypes)
