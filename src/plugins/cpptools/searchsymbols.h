/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#ifndef SEARCHSYMBOLS_H
#define SEARCHSYMBOLS_H

#include <cplusplus/CppDocument.h>
#include <cplusplus/Icons.h>
#include <cplusplus/Overview.h>
#include <Symbols.h>
#include <SymbolVisitor.h>

#include <QIcon>
#include <QMetaType>
#include <QString>
#include <QSet>

#include <functional>

namespace CppTools {
namespace Internal {

struct ModelItemInfo
{
    enum ItemType { Enum, Class, Method };

    ModelItemInfo()
    { }

    ModelItemInfo(const QString &symbolName,
                  const QString &symbolType,
                  ItemType type,
                  const QString &fileName,
                  int line,
                  const QIcon &icon)
        : symbolName(symbolName),
          symbolType(symbolType),
          type(type),
          fileName(fileName),
          line(line),
          icon(icon)
    { }

    QString symbolName;
    QString symbolType;
    ItemType type;
    QString fileName;
    int line;
    QIcon icon;
};

class SearchSymbols: public std::unary_function<CPlusPlus::Document::Ptr, QList<ModelItemInfo> >,
                     protected CPlusPlus::SymbolVisitor
{
public:
    enum SymbolType {
        Classes   = 0x1,
        Functions = 0x2,
        Enums     = 0x4
    };
    Q_DECLARE_FLAGS(SymbolTypes, SymbolType)

    SearchSymbols();

    void setSymbolsToSearchFor(SymbolTypes types);
    void setSeparateScope(bool separateScope);

    QList<ModelItemInfo> operator()(CPlusPlus::Document::Ptr doc)
    { return operator()(doc, QString()); }

    QList<ModelItemInfo> operator()(CPlusPlus::Document::Ptr doc, const QString &scope);

protected:
    using SymbolVisitor::visit;

    void accept(CPlusPlus::Symbol *symbol)
    { CPlusPlus::Symbol::visitSymbol(symbol, this); }

    QString switchScope(const QString &scope);
    virtual bool visit(CPlusPlus::Enum *symbol);
    virtual bool visit(CPlusPlus::Function *symbol);
    virtual bool visit(CPlusPlus::Namespace *symbol);
#if 0
    // This visit method would make function declaration be included in QuickOpen
    virtual bool visit(CPlusPlus::Declaration *symbol);
#endif
    virtual bool visit(CPlusPlus::Class *symbol);

    QString scopedSymbolName(const QString &symbolName) const;
    QString scopedSymbolName(const CPlusPlus::Symbol *symbol) const;
    QString symbolName(const CPlusPlus::Symbol *symbol) const;
    void appendItem(const QString &name,
                    const QString &info,
                    ModelItemInfo::ItemType type,
                    const CPlusPlus::Symbol *symbol);

private:
    QString findOrInsert(const QString &s)
    { return *strings.insert(s); }

    QSet<QString> strings;            // Used to avoid QString duplication

    QString _scope;
    CPlusPlus::Overview overview;
    CPlusPlus::Icons icons;
    QList<ModelItemInfo> items;
    SymbolTypes symbolsToSearchFor;
    bool separateScope;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(SearchSymbols::SymbolTypes)

} // namespace Internal
} // namespace CppTools

Q_DECLARE_METATYPE(CppTools::Internal::ModelItemInfo)

#endif // SEARCHSYMBOLS_H
