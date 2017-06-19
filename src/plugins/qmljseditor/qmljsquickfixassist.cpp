/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "qmljsquickfixassist.h"
#include "qmljseditorconstants.h"
#include "qmljseditordocument.h"

//temp
#include "qmljsquickfix.h"

#include <extensionsystem/pluginmanager.h>

using namespace QmlJSTools;
using namespace TextEditor;

namespace QmlJSEditor {

using namespace Internal;

// -----------------------
// QuickFixAssistInterface
// -----------------------
QmlJSQuickFixAssistInterface::QmlJSQuickFixAssistInterface(QmlJSEditorWidget *editor,
                                                           AssistReason reason)
    : AssistInterface(editor->document(), editor->position(),
                      editor->textDocument()->filePath().toString(), reason)
    , m_semanticInfo(editor->qmlJsEditorDocument()->semanticInfo())
    , m_currentFile(QmlJSRefactoringChanges::file(editor, m_semanticInfo.document))
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

// ---------------------------
// QmlJSQuickFixAssistProvider
// ---------------------------
QmlJSQuickFixAssistProvider::QmlJSQuickFixAssistProvider(QObject *parent)
    : TextEditor::QuickFixAssistProvider(parent)
{}

QmlJSQuickFixAssistProvider::~QmlJSQuickFixAssistProvider()
{}

IAssistProvider::RunType QmlJSQuickFixAssistProvider::runType() const
{
    return Synchronous;
}

IAssistProcessor *QmlJSQuickFixAssistProvider::createProcessor() const
{
    return new QuickFixAssistProcessor(this);
}

QList<QuickFixFactory *> QmlJSQuickFixAssistProvider::quickFixFactories() const
{
    QList<QuickFixFactory *> results;
    foreach (QmlJSQuickFixFactory *f, ExtensionSystem::PluginManager::getObjects<QmlJSQuickFixFactory>())
        results.append(f);
    return results;
}

} // namespace QmlJSEditor
