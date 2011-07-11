/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

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
    if (TextEditor::BaseTextEditorWidget *bte = qobject_cast<TextEditor::BaseTextEditorWidget*>(plainTextEdit())) {
        return bte->tabSettings().m_indentSize;
    } else {
        return 0;
    }
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

namespace {
static inline QmlJS::ModelManagerInterface *getModelManager()
{
    ExtensionSystem::PluginManager *pluginManager = ExtensionSystem::PluginManager::instance();
    return pluginManager->getObject<QmlJS::ModelManagerInterface>();
}
}

QmlJS::Snapshot BaseTextEditModifier::getSnapshot() const
{
    QmlJS::ModelManagerInterface *modelManager = getModelManager();
    if (modelManager)
        return modelManager->snapshot();
    else
        return QmlJS::Snapshot();
}

QStringList BaseTextEditModifier::importPaths() const
{
    QmlJS::ModelManagerInterface *modelManager = getModelManager();
    if (modelManager)
        return modelManager->importPaths();
    else
        return QStringList();
}
