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

#include "qmljsqtstylecodeformatter.h"

#include <texteditor/tabsettings.h>

using namespace QmlJS;
using namespace QmlJSTools;
using namespace TextEditor;

CreatorCodeFormatter::CreatorCodeFormatter()
{
}

CreatorCodeFormatter::CreatorCodeFormatter(const TabSettings &tabSettings)
{
    setTabSize(tabSettings.m_tabSize);
    setIndentSize(tabSettings.m_indentSize);
}

void CreatorCodeFormatter::saveBlockData(QTextBlock *block, const BlockData &data) const
{
    TextBlockUserData *userData = TextDocumentLayout::userData(*block);
    QmlJSCodeFormatterData *cppData = static_cast<QmlJSCodeFormatterData *>(userData->codeFormatterData());
    if (!cppData) {
        cppData = new QmlJSCodeFormatterData;
        userData->setCodeFormatterData(cppData);
    }
    cppData->m_data = data;
}

bool CreatorCodeFormatter::loadBlockData(const QTextBlock &block, BlockData *data) const
{
    TextBlockUserData *userData = TextDocumentLayout::testUserData(block);
    if (!userData)
        return false;
    QmlJSCodeFormatterData *cppData = static_cast<QmlJSCodeFormatterData *>(userData->codeFormatterData());
    if (!cppData)
        return false;

    *data = cppData->m_data;
    return true;
}

void CreatorCodeFormatter::saveLexerState(QTextBlock *block, int state) const
{
    TextDocumentLayout::setLexerState(*block, state);
}

int CreatorCodeFormatter::loadLexerState(const QTextBlock &block) const
{
    return TextDocumentLayout::lexerState(block);
}
