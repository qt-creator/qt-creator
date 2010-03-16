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

#include "basetexteditmodifier.h"

#include <extensionsystem/pluginmanager.h>
#include <qmljs/qmljsdocument.h>
#include <qmljseditor/qmljsmodelmanagerinterface.h>
#include <texteditor/tabsettings.h>

using namespace QmlDesigner;
using namespace QmlJSEditor;

BaseTextEditModifier::BaseTextEditModifier(TextEditor::BaseTextEditor *textEdit):
        PlainTextEditModifier(textEdit)
{
}

void BaseTextEditModifier::indent(int offset, int length)
{
    if (length == 0 || offset < 0 || offset + length >= text().length())
        return;

    if (TextEditor::BaseTextEditor *bte = dynamic_cast<TextEditor::BaseTextEditor*>(plainTextEdit())) {
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
    if (TextEditor::BaseTextEditor *bte = dynamic_cast<TextEditor::BaseTextEditor*>(plainTextEdit())) {
        return bte->tabSettings().m_indentSize;
    } else {
        return 0;
    }
}

QmlJS::Snapshot BaseTextEditModifier::getSnapshot()
{
    ExtensionSystem::PluginManager *pluginManager = ExtensionSystem::PluginManager::instance();
    QmlJSEditor::ModelManagerInterface *modelManager = pluginManager->getObject<QmlJSEditor::ModelManagerInterface>();
    if (modelManager)
        return modelManager->snapshot();
    else
        return QmlJS::Snapshot();
}
