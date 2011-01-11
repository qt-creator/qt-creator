/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef SEARCHSYMBOLS_H
#define SEARCHSYMBOLS_H

#include <cplusplus/CppDocument.h>
#include <cplusplus/Icons.h>
#include <cplusplus/Overview.h>
#include <Symbols.h>
#include <SymbolVisitor.h>

#include <QtGui/QIcon>
#include <QtCore/QMetaType>
#include <QtCore/QString>
#include <QtCore/QSet>
#include <QtCore/QHash>

#include <functional>

namespace CppTools {
namespace Internal {

struct ModelItemInfo
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

class SearchSymbols: public std::unary_function<CPlusPlus::Document::Ptr, QList<ModelItemInfo> >,
                     protected CPlusPlus::SymbolVisitor
{
public:
    enum SymbolType {
        Classes      = 0x1,
        Functions    = 0x2,
        Enums        = 0x4,
        Declarations = 0x8
    };

    Q_DECLARE_FLAGS(SymbolTypes, SymbolType)

    static SymbolTypes AllTypes;

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

} // namespace Internal
} // namespace CppTools

Q_DECLARE_OPERATORS_FOR_FLAGS(CppTools::Internal::SearchSymbols::SymbolTypes)
Q_DECLARE_METATYPE(CppTools::Internal::ModelItemInfo)

#endif // SEARCHSYMBOLS_H
