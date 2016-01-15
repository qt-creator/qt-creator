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

#include "indexitem.h"

#include <utils/fileutils.h>

using namespace CppTools;

IndexItem::Ptr IndexItem::create(const QString &symbolName, const QString &symbolType,
                                 const QString &symbolScope, IndexItem::ItemType type,
                                 const QString &fileName, int line, int column, const QIcon &icon)
{
    Ptr ptr(new IndexItem);

    ptr->m_symbolName = symbolName;
    ptr->m_symbolType = symbolType;
    ptr->m_symbolScope = symbolScope;
    ptr->m_type = type;
    ptr->m_fileName = fileName;
    ptr->m_line = line;
    ptr->m_column = column;
    ptr->m_icon = icon;

    return ptr;
}

IndexItem::Ptr IndexItem::create(const QString &fileName, int sizeHint)
{
    Ptr ptr(new IndexItem);

    ptr->m_fileName = fileName;
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
    return Utils::FileUtils::shortNativePath(Utils::FileName::fromString(m_fileName));
}

void IndexItem::squeeze()
{
    m_children.squeeze();
    for (int i = 0, ei = m_children.size(); i != ei; ++i)
        m_children[i]->squeeze();
}
