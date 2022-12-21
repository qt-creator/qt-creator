// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmljstools_global.h"

#include <texteditor/textdocumentlayout.h>
#include <qmljs/qmljscodeformatter.h>

namespace TextEditor { class TabSettings; }

namespace QmlJSTools {

class QMLJSTOOLS_EXPORT CreatorCodeFormatter : public QmlJS::QtStyleCodeFormatter
{
public:
    CreatorCodeFormatter();
    explicit CreatorCodeFormatter(const TextEditor::TabSettings &tabSettings);

protected:
    void saveBlockData(QTextBlock *block, const BlockData &data) const override;
    bool loadBlockData(const QTextBlock &block, BlockData *data) const override;

    void saveLexerState(QTextBlock *block, int state) const override;
    int loadLexerState(const QTextBlock &block) const override;

private:
    class QmlJSCodeFormatterData: public TextEditor::CodeFormatterData
    {
    public:
        QmlJS::CodeFormatter::BlockData m_data;
    };
};

} // namespace QmlJSTools
