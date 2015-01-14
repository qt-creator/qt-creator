/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "basetexteditmodifier.h"

#include <qmljs/qmljsmodelmanagerinterface.h>
#include <qmljstools/qmljsindenter.h>
#include <qmljseditor/qmljseditordocument.h>
#include <texteditor/tabsettings.h>
#include <utils/changeset.h>

using namespace QmlDesigner;

BaseTextEditModifier::BaseTextEditModifier(TextEditor::TextEditorWidget *textEdit):
        PlainTextEditModifier(textEdit)
{
}

void BaseTextEditModifier::indent(int offset, int length)
{
    if (length == 0 || offset < 0 || offset + length >= text().length())
        return;

    if (TextEditor::TextEditorWidget *baseTextEditorWidget = qobject_cast<TextEditor::TextEditorWidget*>(plainTextEdit())) {

        TextEditor::TextDocument *baseTextEditorDocument = baseTextEditorWidget->textDocument();
        QTextDocument *textDocument = baseTextEditorWidget->document();

        int startLine = -1;
        int endLine = -1;
        int column;

        baseTextEditorWidget->convertPosition(offset, &startLine, &column); //get line
        baseTextEditorWidget->convertPosition(offset + length, &endLine, &column); //get line

        QTextDocument *doc = baseTextEditorDocument->document();
        QTextCursor tc(doc);
        tc.beginEditBlock();

        if (startLine > 0) {
            TextEditor::TabSettings tabSettings = baseTextEditorDocument->tabSettings();
            for (int i = startLine; i <= endLine; i++) {
                QTextBlock start = textDocument->findBlockByNumber(i);

                if (start.isValid()) {
                    QmlJSEditor::Internal::Indenter indenter;
                    indenter.indentBlock(textDocument, start, QChar::Null, tabSettings);
                }
            }
        }

        tc.endEditBlock();
    }
}

int BaseTextEditModifier::indentDepth() const
{
    if (TextEditor::TextEditorWidget *bte = qobject_cast<TextEditor::TextEditorWidget*>(plainTextEdit()))
        return bte->textDocument()->tabSettings().m_indentSize;
    else
        return 0;
}

bool BaseTextEditModifier::renameId(const QString &oldId, const QString &newId)
{
    if (TextEditor::TextEditorWidget *bte = qobject_cast<TextEditor::TextEditorWidget*>(plainTextEdit())) {
        if (QmlJSEditor::QmlJSEditorDocument *document
                = qobject_cast<QmlJSEditor::QmlJSEditorDocument *>(bte->textDocument())) {
            Utils::ChangeSet changeSet;
            foreach (const QmlJS::AST::SourceLocation &loc,
                    document->semanticInfo().idLocations.value(oldId)) {
                changeSet.replace(loc.begin(), loc.end(), newId);
            }
            QTextCursor tc = bte->textCursor();
            changeSet.apply(&tc);
            return true;
        }
    }
    return false;
}
