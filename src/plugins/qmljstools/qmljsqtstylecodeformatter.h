/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
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

class QMLJSTOOLS_EXPORT CreatorCodeFormatter : public QmlJS::QtStyleCodeFormatter
{
public:
    CreatorCodeFormatter();
    explicit CreatorCodeFormatter(const TextEditor::TabSettings &tabSettings);

protected:
    virtual void saveBlockData(QTextBlock *block, const BlockData &data) const;
    virtual bool loadBlockData(const QTextBlock &block, BlockData *data) const;

    virtual void saveLexerState(QTextBlock *block, int state) const;
    virtual int loadLexerState(const QTextBlock &block) const;

private:
    class QmlJSCodeFormatterData: public TextEditor::CodeFormatterData
    {
    public:
        QmlJS::CodeFormatter::BlockData m_data;
    };
};

} // namespace QmlJSTools

#endif // QMLJSQTSTYLECODEFORMATTER_H
