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

#include "clangmodelmanagersupport.h"

#include "constants.h"
#include "clangeditordocumentprocessor.h"
#include "clangutils.h"

#include <coreplugin/editormanager/editormanager.h>
#include <cpptools/baseeditordocumentparser.h>
#include <cpptools/editordocumenthandle.h>
#include <projectexplorer/project.h>

#include <clangbackendipc/cmbregisterprojectsforcodecompletioncommand.h>
#include <clangbackendipc/filecontainer.h>
#include <clangbackendipc/projectpartcontainer.h>
#include <utils/qtcassert.h>

#include <QCoreApplication>

using namespace ClangCodeModel;
using namespace ClangCodeModel::Internal;

static ModelManagerSupportClang *m_instance_forTestsOnly = 0;

static CppTools::CppModelManager *cppModelManager()
{
    return CppTools::CppModelManager::instance();
}

ModelManagerSupportClang::ModelManagerSupportClang()
    : m_completionAssistProvider(m_ipcCommunicator)
{
    QTC_CHECK(!m_instance_forTestsOnly);
    m_instance_forTestsOnly = this;

    Core::EditorManager *editorManager = Core::EditorManager::instance();
    connect(editorManager, &Core::EditorManager::currentEditorChanged,
            this, &ModelManagerSupportClang::onCurrentEditorChanged);
    connect(editorManager, &Core::EditorManager::editorOpened,
            this, &ModelManagerSupportClang::onEditorOpened);

    CppTools::CppModelManager *modelManager = cppModelManager();
    connect(modelManager, &CppTools::CppModelManager::abstractEditorSupportContentsUpdated,
            this, &ModelManagerSupportClang::onAbstractEditorSupportContentsUpdated);
    connect(modelManager, &CppTools::CppModelManager::abstractEditorSupportRemoved,
            this, &ModelManagerSupportClang::onAbstractEditorSupportRemoved);
    connect(modelManager, &CppTools::CppModelManager::projectPartsUpdated,
            this, &ModelManagerSupportClang::onProjectPartsUpdated);
    connect(modelManager, &CppTools::CppModelManager::projectPartsRemoved,
            this, &ModelManagerSupportClang::onProjectPartsRemoved);
}

ModelManagerSupportClang::~ModelManagerSupportClang()
{
    m_instance_forTestsOnly = 0;
}

CppTools::CppCompletionAssistProvider *ModelManagerSupportClang::completionAssistProvider()
{
    return &m_completionAssistProvider;
}

CppTools::BaseEditorDocumentProcessor *ModelManagerSupportClang::editorDocumentProcessor(
        TextEditor::TextDocument *baseTextDocument)
{
    return new ClangEditorDocumentProcessor(this, baseTextDocument);
}

void ModelManagerSupportClang::onCurrentEditorChanged(Core::IEditor *newCurrent)
{
    // If we switch away from a cpp editor, update the backend about
    // the document's unsaved content.
    if (m_previousCppEditor && m_previousCppEditor->document()->isModified()) {
        m_ipcCommunicator.updateUnsavedFileFromCppEditorDocument(
                                m_previousCppEditor->document()->filePath().toString());
    }

    // Remember previous editor
    if (newCurrent && cppModelManager()->isCppEditor(newCurrent))
        m_previousCppEditor = newCurrent;
    else
        m_previousCppEditor.clear();
}

void ModelManagerSupportClang::onEditorOpened(Core::IEditor *editor)
{
    QTC_ASSERT(editor, return);
    Core::IDocument *document = editor->document();
    QTC_ASSERT(document, return);
    TextEditor::TextDocument *textDocument = qobject_cast<TextEditor::TextDocument *>(document);

    if (textDocument && cppModelManager()->isCppEditor(editor)) {
        // Handle externally changed documents
        connect(textDocument, &Core::IDocument::reloadFinished,
                this, &ModelManagerSupportClang::onCppDocumentReloadFinished,
                Qt::UniqueConnection);

        // Handle changes from e.g. refactoring actions
        connect(textDocument, &TextEditor::TextDocument::contentsChanged,
                this, &ModelManagerSupportClang::onCppDocumentContentsChanged,
                Qt::UniqueConnection);

        // TODO: Ensure that not fully loaded documents are updated?
    }
}

void ModelManagerSupportClang::onCppDocumentReloadFinished(bool success)
{
    if (!success)
        return;

    Core::IDocument *document = qobject_cast<Core::IDocument *>(sender());
    m_ipcCommunicator.updateUnsavedFileIfNotCurrentDocument(document);
}

void ModelManagerSupportClang::onCppDocumentContentsChanged()
{
    Core::IDocument *document = qobject_cast<Core::IDocument *>(sender());
    m_ipcCommunicator.updateUnsavedFileIfNotCurrentDocument(document);
}

void ModelManagerSupportClang::onAbstractEditorSupportContentsUpdated(const QString &filePath,
                                                                      const QByteArray &content)
{
    QTC_ASSERT(!filePath.isEmpty(), return);
    m_ipcCommunicator.updateUnsavedFile(filePath, content);
}

void ModelManagerSupportClang::onAbstractEditorSupportRemoved(const QString &filePath)
{
    QTC_ASSERT(!filePath.isEmpty(), return);
    if (!cppModelManager()->cppEditorDocument(filePath)) {
        const QString projectPartId = Utils::projectPartIdForFile(filePath);
        m_ipcCommunicator.unregisterFilesForCodeCompletion(
            {ClangBackEnd::FileContainer(filePath, projectPartId)});
    }
}

void ModelManagerSupportClang::onProjectPartsUpdated(ProjectExplorer::Project *project)
{
    QTC_ASSERT(project, return);
    const CppTools::ProjectInfo projectInfo = cppModelManager()->projectInfo(project);
    QTC_ASSERT(projectInfo.isValid(), return);
    m_ipcCommunicator.registerProjectsParts(projectInfo.projectParts());
}

void ModelManagerSupportClang::onProjectPartsRemoved(const QStringList &projectPartIds)
{
    if (!projectPartIds.isEmpty())
        m_ipcCommunicator.unregisterProjectPartsForCodeCompletion(projectPartIds);
}

#ifdef QT_TESTLIB_LIB
ModelManagerSupportClang *ModelManagerSupportClang::instance_forTestsOnly()
{
    return m_instance_forTestsOnly;
}
#endif

IpcCommunicator &ModelManagerSupportClang::ipcCommunicator()
{
    return m_ipcCommunicator;
}

QString ModelManagerSupportProviderClang::id() const
{
    return QLatin1String(Constants::CLANG_MODELMANAGERSUPPORT_ID);
}

QString ModelManagerSupportProviderClang::displayName() const
{
    //: Display name
    return QCoreApplication::translate("ClangCodeModel::Internal::ModelManagerSupport",
                                       "Clang");
}

CppTools::ModelManagerSupport::Ptr ModelManagerSupportProviderClang::createModelManagerSupport()
{
    return CppTools::ModelManagerSupport::Ptr(new ModelManagerSupportClang);
}
