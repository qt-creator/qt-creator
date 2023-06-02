// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "indexitem.h"

using namespace Utils;

namespace CppEditor {

IndexItem::Ptr IndexItem::create(const QString &symbolName, const QString &symbolType,
                                 const QString &symbolScope, IndexItem::ItemType type,
                                 const QString &fileName, int line, int column, const QIcon &icon,
                                 bool isFunctionDefinition)
{
    Ptr ptr(new IndexItem);

    ptr->m_symbolName = symbolName;
    ptr->m_symbolType = symbolType;
    ptr->m_symbolScope = symbolScope;
    ptr->m_type = type;
    ptr->m_filePath = FilePath::fromString(fileName);
    ptr->m_line = line;
    ptr->m_column = column;
    ptr->m_icon = icon;
    ptr->m_isFuncDef = isFunctionDefinition;

    return ptr;
}

IndexItem::Ptr IndexItem::create(const QString &fileName, int sizeHint)
{
    Ptr ptr(new IndexItem);

    ptr->m_filePath = FilePath::fromString(fileName);
    ptr->m_type = Declaration;
    ptr->m_line = 0;
    ptr->m_column = 0;
    ptr->m_children.reserve(sizeHint);

    return ptr;
}

bool IndexItem::unqualifiedNameAndScope(const QString &defaultName, QString *name,
                                        QString *scope) const
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

QString IndexItem::representDeclaration() const
{
    if (m_symbolType.isEmpty())
        return QString();

    const QString padding = m_symbolType.endsWith(QLatin1Char('*'))
        ? QString()
        : QString(QLatin1Char(' '));
    return m_symbolType + padding + m_symbolName;
}

QString IndexItem::shortNativeFilePath() const
{
    return m_filePath.shortNativePath();
}

void IndexItem::squeeze()
{
    m_children.squeeze();
    for (int i = 0, ei = m_children.size(); i != ei; ++i)
        m_children[i]->squeeze();
}

} // CppEditor
