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

#ifndef QMLJSEDITORCODEFORMATTER_H
#define QMLJSEDITORCODEFORMATTER_H

#include "qmljseditor_global.h"

#include <texteditor/basetextdocumentlayout.h>
#include <qmljs/qmljscodeformatter.h>

namespace TextEditor {
    class TabSettings;
}

namespace QmlJSEditor {

class QMLJSEDITOR_EXPORT QtStyleCodeFormatter : public QmlJS::CodeFormatter
{
public:
    QtStyleCodeFormatter();
    explicit QtStyleCodeFormatter(const TextEditor::TabSettings &tabSettings);

    void setIndentSize(int size);

protected:
    virtual void onEnter(int newState, int *indentDepth, int *savedIndentDepth) const;
    virtual void adjustIndent(const QList<QmlJS::Token> &tokens, int lexerState, int *indentDepth) const;

    virtual void saveBlockData(QTextBlock *block, const BlockData &data) const;
    virtual bool loadBlockData(const QTextBlock &block, BlockData *data) const;

    virtual void saveLexerState(QTextBlock *block, int state) const;
    virtual int loadLexerState(const QTextBlock &block) const;

private:
    int m_indentSize;

    class QmlJSCodeFormatterData: public TextEditor::CodeFormatterData
    {
    public:
        QmlJS::CodeFormatter::BlockData m_data;
    };
};

} // namespace QmlJSEditor

#endif // QMLJSEDITORCODEFORMATTER_H
