/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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
#include "stringtable.h"

#include <cplusplus/CppDocument.h>
#include <cplusplus/Icons.h>
#include <cplusplus/Overview.h>

#include <utils/fileutils.h>
#include <utils/function.h>

#include <QIcon>
#include <QString>
#include <QSet>
#include <QSharedPointer>
#include <QHash>

namespace CppTools {

class CPPTOOLS_EXPORT ModelItemInfo
{
    Q_DISABLE_COPY(ModelItemInfo)

public:
    enum ItemType { Enum, Class, Function, Declaration };

private:
    ModelItemInfo(const QString &symbolName,
                  const QString &symbolType,
                  const QString &symbolScope,
                  ItemType type,
                  const QString &fileName,
                  int line,
                  int column,
                  const QIcon &icon)
        : m_symbolName(symbolName),
          m_symbolType(symbolType),
          m_symbolScope(symbolScope),
          m_fileName(fileName),
          m_icon(icon),
          m_type(type),
          m_line(line),
          m_column(column)
    {}

    ModelItemInfo(const QString &fileName, int sizeHint)
        : m_fileName(fileName)
        , m_type(Declaration)
        , m_line(0)
        , m_column(0)
    { m_children.reserve(sizeHint); }

public:
    typedef QSharedPointer<ModelItemInfo> Ptr;
    static Ptr create(const QString &symbolName,
                      const QString &symbolType,
                      const QString &symbolScope,
                      ItemType type,
                      const QString &fileName,
                      int line,
                      int column,
                      const QIcon &icon)
    {
        return Ptr(new ModelItemInfo(
                       symbolName, symbolType, symbolScope, type, fileName, line, column, icon));
    }

    static Ptr create(const QString &fileName, int sizeHint)
    {
        return Ptr(new ModelItemInfo(fileName, sizeHint));
    }

    QString scopedSymbolName() const
    {
        return m_symbolScope.isEmpty()
                ? m_symbolName
                : m_symbolScope +  QLatin1String("::") + m_symbolName;
    }

    bool unqualifiedNameAndScope(const QString &defaultName, QString *name, QString *scope) const
    {
        *name = defaultName;
        *scope = m_symbolScope;
        const QString qualifiedName = scopedSymbolName();
        const int colonColonPosition = qualifiedName.lastIndexOf(QLatin1String("::"));
        if (colonColonPosition != -1) {
            *name = qualifiedName.mid(colonColonPosition + 2);
            *scope = qualifiedName.left(colonColonPosition);
            return true;
        }
        return false;
    }

    static QString representDeclaration(const QString &name, const QString &type)
    {
        if (type.isEmpty())
            return QString();

        const QString padding = type.endsWith(QLatin1Char('*'))
            ? QString()
            : QString(QLatin1Char(' '));
        return type + padding + name;
    }

    QString shortNativeFilePath() const
    { return Utils::FileUtils::shortNativePath(Utils::FileName::fromString(m_fileName)); }

    QString symbolName() const { return m_symbolName; }
    QString symbolType() const { return m_symbolType; }
    QString symbolScope() const { return m_symbolScope; }
    QString fileName() const { return m_fileName; }
    QIcon icon() const { return m_icon; }
    ItemType type() const { return m_type; }
    int line() const { return m_line; }
    int column() const { return m_column; }

    void addChild(ModelItemInfo::Ptr childItem) { m_children.append(childItem); }
    void squeeze();

    void visitAllChildren(std::function<void (const ModelItemInfo::Ptr &)> f) const;

private:
    QString m_symbolName; // as found in the code, therefore might be qualified
    QString m_symbolType;
    QString m_symbolScope;
    QString m_fileName;
    QIcon m_icon;
    ItemType m_type;
    int m_line;
    int m_column;
    QVector<ModelItemInfo::Ptr> m_children;
};

class SearchSymbols: protected CPlusPlus::SymbolVisitor
{
public:
    typedef SymbolSearcher::SymbolTypes SymbolTypes;

    static SymbolTypes AllTypes;

    SearchSymbols(Internal::StringTable &stringTable);

    void setSymbolsToSearchFor(const SymbolTypes &types);

    ModelItemInfo::Ptr operator()(CPlusPlus::Document::Ptr doc, int sizeHint = 500)
    { return operator()(doc, sizeHint, QString()); }

    ModelItemInfo::Ptr operator()(CPlusPlus::Document::Ptr doc, int sizeHint, const QString &scope);

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
    virtual bool visit(CPlusPlus::ObjCClass *);
    virtual bool visit(CPlusPlus::ObjCForwardClassDeclaration *);
    virtual bool visit(CPlusPlus::ObjCProtocol *);
    virtual bool visit(CPlusPlus::ObjCForwardProtocolDeclaration *);
    virtual bool visit(CPlusPlus::ObjCMethod *);
    virtual bool visit(CPlusPlus::ObjCPropertyDeclaration *);

    QString scopedSymbolName(const QString &symbolName, const CPlusPlus::Symbol *symbol) const;
    QString scopedSymbolName(const CPlusPlus::Symbol *symbol) const;
    QString scopeName(const QString &name, const CPlusPlus::Symbol *symbol) const;
    ModelItemInfo::Ptr addChildItem(const QString &symbolName,
                                    const QString &symbolType,
                                    const QString &symbolScope,
                                    ModelItemInfo::ItemType type,
                                    CPlusPlus::Symbol *symbol);

private:
    QString findOrInsert(const QString &s)
    { return strings.insert(s); }

    Internal::StringTable &strings;            // Used to avoid QString duplication

    ModelItemInfo::Ptr _parent;
    QString _scope;
    CPlusPlus::Overview overview;
    CPlusPlus::Icons icons;
    SymbolTypes symbolsToSearchFor;
    QHash<const CPlusPlus::StringLiteral *, QString> m_paths;
};

} // namespace CppTools

Q_DECLARE_OPERATORS_FOR_FLAGS(CppTools::SearchSymbols::SymbolTypes)
Q_DECLARE_METATYPE(CppTools::ModelItemInfo::Ptr)

#endif // SEARCHSYMBOLS_H
