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

#ifndef SEARCHSYMBOLS_H
#define SEARCHSYMBOLS_H

#include "cpptools_global.h"
#include "cppindexingsupport.h"

#include <cplusplus/CppDocument.h>
#include <cplusplus/Icons.h>
#include <cplusplus/Overview.h>
#include <cplusplus/SymbolVisitor.h>
#include <cplusplus/Symbols.h>

#include <QIcon>
#include <QMetaType>
#include <QString>
#include <QSet>
#include <QHash>

#include <functional>

namespace CppTools {

struct CPPTOOLS_EXPORT ModelItemInfo
{
    enum ItemType { Enum, Class, Method, Declaration };

    ModelItemInfo()
        : type(Declaration),
          line(0),
          column(0)
    { }

    ModelItemInfo(const QString &symbolName,
                  const QString &symbolType,
                  ItemType type,
                  QStringList fullyQualifiedName,
                  const QString &fileName,
                  int line,
                  int column,
                  const QIcon &icon)
        : symbolName(symbolName),
          symbolType(symbolType),
          fullyQualifiedName(fullyQualifiedName),
          fileName(fileName),
          icon(icon),
          type(type),
          line(line),
          column(column)
    { }

    ModelItemInfo(const ModelItemInfo &otherInfo)
        : symbolName(otherInfo.symbolName),
          symbolType(otherInfo.symbolType),
          fullyQualifiedName(otherInfo.fullyQualifiedName),
          fileName(otherInfo.fileName),
          icon(otherInfo.icon),
          type(otherInfo.type),
          line(otherInfo.line),
          column(otherInfo.column)
    {  }

    QString symbolName;
    QString symbolType;
    QStringList fullyQualifiedName;
    QString fileName;
    QIcon icon;
    ItemType type;
    int line;
    int column;
};

class SearchSymbols: public std::binary_function<CPlusPlus::Document::Ptr, int, QList<ModelItemInfo> >,
                     protected CPlusPlus::SymbolVisitor
{
public:
    typedef SymbolSearcher::SymbolTypes SymbolTypes;

    static SymbolTypes AllTypes;

    SearchSymbols();

    void setSymbolsToSearchFor(SymbolTypes types);
    void setSeparateScope(bool separateScope);

    QList<ModelItemInfo> operator()(CPlusPlus::Document::Ptr doc, int sizeHint = 500)
    { return operator()(doc, sizeHint, QString()); }

    QList<ModelItemInfo> operator()(CPlusPlus::Document::Ptr doc, int sizeHint, const QString &scope);

protected:
    using SymbolVisitor::visit;

    void accept(CPlusPlus::Symbol *symbol)
    { CPlusPlus::Symbol::visitSymbol(symbol, this); }

    QString switchScope(const QString &scope);

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
    virtual bool visit(CPlusPlus::ObjCClass *);
    virtual bool visit(CPlusPlus::ObjCForwardClassDeclaration *);
    virtual bool visit(CPlusPlus::ObjCProtocol *);
    virtual bool visit(CPlusPlus::ObjCForwardProtocolDeclaration *);
    virtual bool visit(CPlusPlus::ObjCMethod *);
    virtual bool visit(CPlusPlus::ObjCPropertyDeclaration *);

    QString scopedSymbolName(const QString &symbolName) const;
    QString scopedSymbolName(const CPlusPlus::Symbol *symbol) const;
    QString symbolName(const CPlusPlus::Symbol *symbol) const;
    void appendItem(const QString &name,
                    const QString &info,
                    ModelItemInfo::ItemType type,
                    CPlusPlus::Symbol *symbol);

private:
    QString findOrInsert(const QString &s)
    { return *strings.insert(s); }

    QSet<QString> strings;            // Used to avoid QString duplication

    QString _scope;
    CPlusPlus::Overview overview;
    CPlusPlus::Icons icons;
    QList<ModelItemInfo> items;
    SymbolTypes symbolsToSearchFor;
    QHash<const CPlusPlus::StringLiteral *, QString> m_paths;
    bool separateScope;
};

} // namespace CppTools

Q_DECLARE_OPERATORS_FOR_FLAGS(CppTools::SearchSymbols::SymbolTypes)
Q_DECLARE_METATYPE(CppTools::ModelItemInfo)

#endif // SEARCHSYMBOLS_H
