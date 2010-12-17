/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#include "fakevimsyntax.h"

#include <utils/qtcassert.h>

typedef QLatin1String _;

namespace FakeVim {
namespace Internal {

///////////////////////////////////////////////////////////////////////
//
// HighlighterRules
//
///////////////////////////////////////////////////////////////////////

HighlighterRules::HighlighterRules()
{}

void HighlighterRules::addKeyWord(const QString &keyword, const QByteArray &group)
{
    m_keywordGroups.insert(keyword, group);
}

///////////////////////////////////////////////////////////////////////
//
// PythonHighlighter
//
///////////////////////////////////////////////////////////////////////

PythonHighlighterRules::PythonHighlighterRules()
{
    const QByteArray stmt = "statement";
    addKeyWord(_("assert"), stmt);
    addKeyWord(_("break"), stmt);
    addKeyWord(_("continue"), stmt);
    addKeyWord(_("del"), stmt);
    addKeyWord(_("except"), stmt);
    addKeyWord(_("exec"), stmt);
    addKeyWord(_("finally"), stmt);
    addKeyWord(_("global"), stmt);
    addKeyWord(_("lambda"), stmt);
    addKeyWord(_("pass"), stmt);
    addKeyWord(_("print"), stmt);
    addKeyWord(_("raise"), stmt);
    addKeyWord(_("return"), stmt);
    addKeyWord(_("try"), stmt);
    addKeyWord(_("with"), stmt);
    addKeyWord(_("yield"), stmt);

    const QByteArray func = "function";
    const QByteArray options = "nextgroup=pythonFunction skipwhite";
    addKeyWord(_("def"), stmt);
    addKeyWord(_("class"), stmt);
}


///////////////////////////////////////////////////////////////////////
//
// Highlighter
//
///////////////////////////////////////////////////////////////////////

Highlighter::Highlighter(QTextDocument *doc, HighlighterRules *rules)
  : QSyntaxHighlighter(doc), m_rules(rules)
{}

void Highlighter::highlightBlock(const QString &text)
{
    QTextCharFormat myClassFormat;
    myClassFormat.setFontWeight(QFont::Bold);
    myClassFormat.setForeground(Qt::darkMagenta);
    
    KeywordGroupMap::ConstIterator it, et = m_rules->m_keywordGroups.end();
    for (it = m_rules->m_keywordGroups.begin(); it != et; ++it) {
        const QString &keyword = it.key();
        //qDebug() << "EXAMINING " << keyword;
        int index = text.indexOf(keyword);
        while (index >= 0) {
            setFormat(index, keyword.size(), myClassFormat);
            index = text.indexOf(keyword, index + keyword.size());
         }
    }
}

} // namespace Internal
} // namespace FakeVim

