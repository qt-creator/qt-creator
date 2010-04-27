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

