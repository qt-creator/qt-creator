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

#ifndef QMLJSQTSTYLECODEFORMATTER_H
#define QMLJSQTSTYLECODEFORMATTER_H

#include "qmljstools_global.h"

#include <texteditor/basetextdocumentlayout.h>
#include <qmljs/qmljscodeformatter.h>

namespace TextEditor {
    class TabSettings;
}

namespace QmlJSTools {

class QMLJSTOOLS_EXPORT QtStyleCodeFormatter : public QmlJS::CodeFormatter
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

} // namespace QmlJSTools

#endif // QMLJSQTSTYLECODEFORMATTER_H
