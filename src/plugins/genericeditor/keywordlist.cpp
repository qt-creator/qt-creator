/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
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
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "keywordlist.h"

using namespace GenericEditor;
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
