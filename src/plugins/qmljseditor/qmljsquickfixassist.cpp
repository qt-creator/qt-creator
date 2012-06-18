/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "qmljsquickfixassist.h"
#include "qmljseditorconstants.h"

//temp
#include "qmljsquickfix.h"

#include <extensionsystem/pluginmanager.h>

using namespace QmlJSEditor;
using namespace Internal;
using namespace QmlJSTools;
using namespace TextEditor;

// -----------------------
// QuickFixAssistInterface
// -----------------------
QmlJSQuickFixAssistInterface::QmlJSQuickFixAssistInterface(QmlJSTextEditorWidget *editor,
                                                           TextEditor::AssistReason reason)
    : DefaultAssistInterface(editor->document(), editor->position(), editor->editorDocument(), reason)
    , m_editor(editor)
    , m_semanticInfo(editor->semanticInfo())
    , m_currentFile(QmlJSRefactoringChanges::file(m_editor, m_semanticInfo.document))
{}

QmlJSQuickFixAssistInterface::~QmlJSQuickFixAssistInterface()
{}

const SemanticInfo &QmlJSQuickFixAssistInterface::semanticInfo() const
{
    return m_semanticInfo;
}

QmlJSRefactoringFilePtr QmlJSQuickFixAssistInterface::currentFile() const
{
    return m_currentFile;
}

QmlJSTextEditorWidget *QmlJSQuickFixAssistInterface::editor() const
{
    return m_editor;
}

// ----------------------
// QmlJSQuickFixProcessor
// ----------------------
QmlJSQuickFixProcessor::QmlJSQuickFixProcessor(const TextEditor::IAssistProvider *provider)
    : m_provider(provider)
{}

QmlJSQuickFixProcessor::~QmlJSQuickFixProcessor()
{}

const IAssistProvider *QmlJSQuickFixProcessor::provider() const
{
    return m_provider;
}

// ---------------------------
// QmlJSQuickFixAssistProvider
// ---------------------------
QmlJSQuickFixAssistProvider::QmlJSQuickFixAssistProvider()
{}

QmlJSQuickFixAssistProvider::~QmlJSQuickFixAssistProvider()
{}

bool QmlJSQuickFixAssistProvider::supportsEditor(const Core::Id &editorId) const
{
    return editorId == Core::Id(Constants::C_QMLJSEDITOR_ID);
}

IAssistProcessor *QmlJSQuickFixAssistProvider::createProcessor() const
{
    return new QmlJSQuickFixProcessor(this);
}

QList<QuickFixFactory *> QmlJSQuickFixAssistProvider::quickFixFactories() const
{
    QList<TextEditor::QuickFixFactory *> results;
    foreach (QmlJSQuickFixFactory *f, ExtensionSystem::PluginManager::getObjects<QmlJSEditor::QmlJSQuickFixFactory>())
        results.append(f);
    return results;
}
