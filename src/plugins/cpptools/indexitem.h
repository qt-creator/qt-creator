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

#include <QIcon>
#include <QSharedPointer>
#include <QMetaType>

#include <functional>

namespace CppTools {

class CPPTOOLS_EXPORT IndexItem
{
    Q_DISABLE_COPY(IndexItem)

    IndexItem() {}

public:
    enum ItemType {
        Enum        = 1 << 0,
        Class       = 1 << 1,
        Function    = 1 << 2,
        Declaration = 1 << 3,

        All = Enum | Class | Function | Declaration
    };

public:
    typedef QSharedPointer<IndexItem> Ptr;
    static Ptr create(const QString &symbolName,
                      const QString &symbolType,
                      const QString &symbolScope,
                      ItemType type,
                      const QString &fileName,
                      int line,
                      int column,
                      const QIcon &icon);
    static Ptr create(const QString &fileName, int sizeHint);

    QString scopedSymbolName() const
    {
        return m_symbolScope.isEmpty()
                ? m_symbolName
                : m_symbolScope +  QLatin1String("::") + m_symbolName;
    }

    bool unqualifiedNameAndScope(const QString &defaultName, QString *name, QString *scope) const;

    QString representDeclaration() const;

    QString shortNativeFilePath() const;

    QString symbolName() const { return m_symbolName; }
    QString symbolType() const { return m_symbolType; }
    QString symbolScope() const { return m_symbolScope; }
    QString fileName() const { return m_fileName; }
    QIcon icon() const { return m_icon; }
    ItemType type() const { return m_type; }
    int line() const { return m_line; }
    int column() const { return m_column; }

    void addChild(IndexItem::Ptr childItem) { m_children.append(childItem); }
    void squeeze();

    enum VisitorResult {
        Break, /// terminates traversal
        Continue, /// continues traversal with the next sibling
        Recurse, /// continues traversal with the children
    };

    typedef std::function<VisitorResult (const IndexItem::Ptr &)> Visitor;

    VisitorResult visitAllChildren(Visitor callback) const
    {
        VisitorResult result = Recurse;
        foreach (const IndexItem::Ptr &child, m_children) {
            result = callback(child);
            switch (result) {
            case Break:
                return Break;
            case Continue:
                continue;
            case Recurse:
                if (!child->m_children.isEmpty()) {
                    result = child->visitAllChildren(callback);
                    if (result == Break)
                        return Break;
                }
            }
        }
        return result;
    }

private:
    QString m_symbolName; // as found in the code, therefore might be qualified
    QString m_symbolType;
    QString m_symbolScope;
    QString m_fileName;
    QIcon m_icon;
    ItemType m_type = All;
    int m_line = 0;
    int m_column = 0;
    QVector<IndexItem::Ptr> m_children;
};

} // CppTools namespace

Q_DECLARE_METATYPE(CppTools::IndexItem::Ptr)
