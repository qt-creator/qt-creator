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
