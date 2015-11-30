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
QmlJSQuickFixAssistProvider::QmlJSQuickFixAssistProvider()
{}

QmlJSQuickFixAssistProvider::~QmlJSQuickFixAssistProvider()
{}

IAssistProvider::RunType QmlJSQuickFixAssistProvider::runType() const
{
    return Synchronous;
}

bool QmlJSQuickFixAssistProvider::supportsEditor(Core::Id editorId) const
{
    return editorId == Constants::C_QMLJSEDITOR_ID;
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
