// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmljsqtstylecodeformatter.h"

#include <texteditor/tabsettings.h>

using namespace QmlJS;
using namespace QmlJSTools;
using namespace TextEditor;

CreatorCodeFormatter::CreatorCodeFormatter() = default;

CreatorCodeFormatter::CreatorCodeFormatter(const TabSettings &tabSettings)
{
    setTabSize(tabSettings.m_tabSize);
    setIndentSize(tabSettings.m_indentSize);
}

void CreatorCodeFormatter::saveBlockData(QTextBlock *block, const BlockData &data) const
{
    auto cppData = static_cast<QmlJSCodeFormatterData *>(TextBlockUserData::codeFormatterData(*block));
    if (!cppData) {
        cppData = new QmlJSCodeFormatterData;
        TextBlockUserData::setCodeFormatterData(*block, cppData);
    }
    cppData->m_data = data;
}

bool CreatorCodeFormatter::loadBlockData(const QTextBlock &block, BlockData *data) const
{
    auto cppData = static_cast<const QmlJSCodeFormatterData *>(
        TextBlockUserData::codeFormatterData(block));
    if (!cppData)
        return false;

    *data = cppData->m_data;
    return true;
}

void CreatorCodeFormatter::saveLexerState(QTextBlock *block, int state) const
{
    TextBlockUserData::setLexerState(*block, state);
}

int CreatorCodeFormatter::loadLexerState(const QTextBlock &block) const
{
    return TextBlockUserData::lexerState(block);
}
