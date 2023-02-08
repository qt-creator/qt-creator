// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "cppeditor_global.h"

#include <utils/filepath.h>

#include <QIcon>
#include <QSharedPointer>
#include <QMetaType>

#include <functional>

namespace CppEditor {

class CPPEDITOR_EXPORT IndexItem
{
    Q_DISABLE_COPY(IndexItem)

    IndexItem() = default;

public:
    enum ItemType {
        Enum        = 1 << 0,
        Class       = 1 << 1,
        Function    = 1 << 2,
        Declaration = 1 << 3,

        All = Enum | Class | Function | Declaration
    };

public:
    using Ptr = QSharedPointer<IndexItem>;
    static Ptr create(const QString &symbolName,
                      const QString &symbolType,
                      const QString &symbolScope,
                      ItemType type,
                      const QString &fileName,
                      int line,
                      int column,
                      const QIcon &icon,
                      bool isFunctionDefinition);
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
    const Utils::FilePath &filePath() const { return m_filePath; }
    QIcon icon() const { return m_icon; }
    ItemType type() const { return m_type; }
    int line() const { return m_line; }
    int column() const { return m_column; }
    bool isFunctionDefinition() const { return m_isFuncDef; }

    void addChild(IndexItem::Ptr childItem) { m_children.append(childItem); }
    void squeeze();

    enum VisitorResult {
        Break, /// terminates traversal
        Continue, /// continues traversal with the next sibling
        Recurse, /// continues traversal with the children
    };

    using Visitor = std::function<VisitorResult (const IndexItem::Ptr &)>;

    VisitorResult visitAllChildren(Visitor callback) const
    {
        VisitorResult result = Recurse;
        for (const IndexItem::Ptr &child : std::as_const(m_children)) {
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
    Utils::FilePath m_filePath;
    QIcon m_icon;
    ItemType m_type = All;
    int m_line = 0;
    int m_column = 0;
    bool m_isFuncDef = false;
    QVector<IndexItem::Ptr> m_children;
};

} // CppEditor namespace

Q_DECLARE_METATYPE(CppEditor::IndexItem::Ptr)
