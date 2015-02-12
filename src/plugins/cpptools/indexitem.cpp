/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
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
