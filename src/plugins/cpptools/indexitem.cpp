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

#include "indexitem.h"

#include <utils/fileutils.h>

using namespace CppTools;

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

void IndexItem::visitAllChildren(std::function<void (const IndexItem::Ptr &)> f) const
{
    foreach (const IndexItem::Ptr &child, m_children) {
        f(child);
        if (!child->m_children.isEmpty())
            child->visitAllChildren(f);
    }
}
