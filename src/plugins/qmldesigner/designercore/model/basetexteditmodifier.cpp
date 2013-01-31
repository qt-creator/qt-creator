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

#include "basetexteditmodifier.h"

#include <extensionsystem/pluginmanager.h>
#include <qmljs/qmljsdocument.h>
#include <qmljs/qmljsmodelmanagerinterface.h>
#include <qmljseditor/qmljseditor.h>
#include <texteditor/tabsettings.h>
#include <utils/changeset.h>

using namespace QmlDesigner;

BaseTextEditModifier::BaseTextEditModifier(TextEditor::BaseTextEditorWidget *textEdit):
        PlainTextEditModifier(textEdit)
{
}

void BaseTextEditModifier::indent(int offset, int length)
{
    if (length == 0 || offset < 0 || offset + length >= text().length())
        return;

    if (TextEditor::BaseTextEditorWidget *bte = qobject_cast<TextEditor::BaseTextEditorWidget*>(plainTextEdit())) {
        // find the applicable block:
        QTextDocument *doc = bte->document();
        QTextCursor tc(doc);
        tc.beginEditBlock();
        tc.setPosition(offset);
        tc.setPosition(offset + length, QTextCursor::KeepAnchor);
        bte->indentInsertedText(tc);
        tc.endEditBlock();
    }
}

int BaseTextEditModifier::indentDepth() const
{
    if (TextEditor::BaseTextEditorWidget *bte = qobject_cast<TextEditor::BaseTextEditorWidget*>(plainTextEdit()))
        return bte->tabSettings().m_indentSize;
    else
        return 0;
}

bool BaseTextEditModifier::renameId(const QString &oldId, const QString &newId)
{
    if (QmlJSEditor::QmlJSTextEditorWidget *qmljse = qobject_cast<QmlJSEditor::QmlJSTextEditorWidget*>(plainTextEdit())) {
        Utils::ChangeSet changeSet;
        foreach (const QmlJS::AST::SourceLocation &loc, qmljse->semanticInfo().idLocations.value(oldId)) {
            changeSet.replace(loc.begin(), loc.end(), newId);
        }
        QTextCursor tc = qmljse->textCursor();
        changeSet.apply(&tc);
        return true;
    } else {
        return false;
    }
}

QmlJS::Snapshot BaseTextEditModifier::getSnapshot() const
{
    QmlJS::ModelManagerInterface *modelManager = QmlJS::ModelManagerInterface::instance();
    if (modelManager)
        return modelManager->snapshot();
    else
        return QmlJS::Snapshot();
}

QStringList BaseTextEditModifier::importPaths() const
{
    QmlJS::ModelManagerInterface *modelManager = QmlJS::ModelManagerInterface::instance();
    if (modelManager)
        return modelManager->importPaths();
    else
        return QStringList();
}
