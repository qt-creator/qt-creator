/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#include "qmljsqtstylecodeformatter.h"

#include <texteditor/tabsettings.h>

#include <QDebug>

using namespace QmlJS;
using namespace QmlJSTools;
using namespace TextEditor;

CreatorCodeFormatter::CreatorCodeFormatter()
{
}

CreatorCodeFormatter::CreatorCodeFormatter(const TextEditor::TabSettings &tabSettings)
{
    setTabSize(tabSettings.m_tabSize);
    setIndentSize(tabSettings.m_indentSize);
}

void CreatorCodeFormatter::saveBlockData(QTextBlock *block, const BlockData &data) const
{
    TextBlockUserData *userData = BaseTextDocumentLayout::userData(*block);
    QmlJSCodeFormatterData *cppData = static_cast<QmlJSCodeFormatterData *>(userData->codeFormatterData());
    if (!cppData) {
        cppData = new QmlJSCodeFormatterData;
        userData->setCodeFormatterData(cppData);
    }
    cppData->m_data = data;
}

bool CreatorCodeFormatter::loadBlockData(const QTextBlock &block, BlockData *data) const
{
    TextBlockUserData *userData = BaseTextDocumentLayout::testUserData(block);
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
    BaseTextDocumentLayout::setLexerState(*block, state);
}

int CreatorCodeFormatter::loadLexerState(const QTextBlock &block) const
{
    return BaseTextDocumentLayout::lexerState(block);
}
