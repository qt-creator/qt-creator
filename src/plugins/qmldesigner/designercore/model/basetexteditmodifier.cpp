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

#include "basetexteditmodifier.h"

#include <extensionsystem/pluginmanager.h>
#include <qmljs/qmljsdocument.h>
#include <qmljs/qmljsmodelmanagerinterface.h>
#include <qmljseditor/qmljseditor.h>
#include <texteditor/tabsettings.h>

using namespace QmlDesigner;

BaseTextEditModifier::BaseTextEditModifier(TextEditor::BaseTextEditor *textEdit):
        PlainTextEditModifier(textEdit)
{
}

void BaseTextEditModifier::indent(int offset, int length)
{
    if (length == 0 || offset < 0 || offset + length >= text().length())
        return;

    if (TextEditor::BaseTextEditor *bte = qobject_cast<TextEditor::BaseTextEditor*>(plainTextEdit())) {
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
    if (TextEditor::BaseTextEditor *bte = qobject_cast<TextEditor::BaseTextEditor*>(plainTextEdit())) {
        return bte->tabSettings().m_indentSize;
    } else {
        return 0;
    }
}

bool BaseTextEditModifier::renameId(const QString &oldId, const QString &newId)
{
    if (QmlJSEditor::QmlJSTextEditor *qmljse = qobject_cast<QmlJSEditor::QmlJSTextEditor*>(plainTextEdit())) {
        qmljse->renameId(oldId, newId);
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
