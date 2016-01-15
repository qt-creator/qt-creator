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

#include "keywordlist.h"

using namespace TextEditor;
using namespace Internal;

void KeywordList::addKeyword(const QString &keyword)
{
    if (keyword.isEmpty())
        return;

    m_keywords.insert(keyword);
}

bool KeywordList::isKeyword(const QString &keyword, Qt::CaseSensitivity sensitivity) const
{
    if (keyword.isEmpty())
        return false;

    // Case sensitivity could be implemented, for example, by converting all keywords to lower
    // if the global sensitivity attribute is insensitive, then always checking for containment
    // (with a conversion to lower in the necessary cases). But the code below is one alternative
    // to support the existence of local sensitivity attributes (which override the global one -
    // currently not documented).
    if (sensitivity == Qt::CaseSensitive) {
        return m_keywords.contains(keyword);
    } else {
        foreach (const QString &s, m_keywords)
            if (keyword.compare(s, Qt::CaseInsensitive) == 0)
                return true;
        return false;
    }
}
