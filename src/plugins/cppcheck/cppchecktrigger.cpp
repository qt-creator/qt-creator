// Copyright (C) 2018 Sergey Morozov
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cppchecktextmarkmanager.h"
#include "cppchecktool.h"
#include "cppchecktrigger.h"

#include <cppeditor/cppmodelmanager.h>

#include <utils/qtcassert.h>

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>

#include <projectexplorer/project.h>
#include <projectexplorer/projectmanager.h>

using namespace Core;
using namespace ProjectExplorer;
using namespace Utils;

namespace Cppcheck::Internal {

CppcheckTrigger::CppcheckTrigger(CppcheckTextMarkManager &marks, CppcheckTool &tool) :
      m_marks(marks),
      m_tool(tool)
{
    using CppModelManager = CppEditor::CppModelManager;

    connect(EditorManager::instance(), &EditorManager::editorOpened,
            this, [this](IEditor *editor) {checkEditors({editor});});
    connect(EditorManager::instance(), &EditorManager::editorsClosed,
            this, &CppcheckTrigger::removeEditors);
    connect(EditorManager::instance(), &EditorManager::aboutToSave,
            this, &CppcheckTrigger::checkChangedDocument);

    connect(ProjectManager::instance(), &ProjectManager::startupProjectChanged,
            this, &CppcheckTrigger::changeCurrentProject);

    connect(CppModelManager::instance(), &CppModelManager::projectPartsUpdated,
            this, &CppcheckTrigger::updateProjectFiles);
}

CppcheckTrigger::~CppcheckTrigger() = default;

void CppcheckTrigger::recheck()
{
    removeEditors();
    checkEditors();
}

void CppcheckTrigger::checkEditors(const QList<IEditor *> &editors)
{
    if (!m_currentProject)
        return;

    using CppModelManager = CppEditor::CppModelManager;
    const CppEditor::ProjectInfo::ConstPtr info
            = CppModelManager::projectInfo(m_currentProject);
    if (!info)
        return;

    const QList<IEditor *> editorList = !editors.isEmpty()
            ? editors : DocumentModel::editorsForOpenedDocuments();

    FilePaths toCheck;
    for (const IEditor *editor : editorList) {
        QTC_ASSERT(editor, continue);
        IDocument *document = editor->document();
        QTC_ASSERT(document, continue);
        const FilePath &path = document->filePath();
        if (path.isEmpty())
            continue;

        if (m_checkedFiles.contains(path))
            continue;

        if (!m_currentProject->isKnownFile(path))
            continue;

        if (!info->sourceFiles().contains(path))
            continue;

        connect(document, &IDocument::aboutToReload,
                this, [this, document]{checkChangedDocument(document);});
        connect(document, &IDocument::contentsChanged,
                this, [this, document] {
            if (!document->isModified())
                checkChangedDocument(document);
        });

        m_checkedFiles.insert(path, QDateTime::currentDateTime());
        toCheck.push_back(path);
    }

    if (!toCheck.isEmpty()) {
        remove(toCheck);
        check(toCheck);
    }
}

void CppcheckTrigger::removeEditors(const QList<IEditor *> &editors)
{
    if (!m_currentProject)
        return;

    const QList<IEditor *> editorList = !editors.isEmpty()
            ? editors : DocumentModel::editorsForOpenedDocuments();

    FilePaths toRemove;
    for (const IEditor *editor : editorList) {
        QTC_ASSERT(editor, return);
        const IDocument *document = editor->document();
        QTC_ASSERT(document, return);
        const FilePath &path = document->filePath();
        if (path.isEmpty())
            return;

        if (!m_checkedFiles.contains(path))
            continue;

        disconnect(document, nullptr, this, nullptr);
        m_checkedFiles.remove(path);
        toRemove.push_back(path);
    }

    if (!toRemove.isEmpty())
        remove(toRemove);
}

void CppcheckTrigger::checkChangedDocument(IDocument *document)
{
    QTC_ASSERT(document, return);

    if (!m_currentProject)
        return;

    const FilePath &path = document->filePath();
    QTC_ASSERT(!path.isEmpty(), return);
    if (!m_checkedFiles.contains(path))
        return;

    remove({path});
    check({path});
}

void CppcheckTrigger::changeCurrentProject(Project *project)
{
    m_currentProject = project;
    m_checkedFiles.clear();
    remove({});
    m_tool.setProject(project);
    checkEditors(DocumentModel::editorsForOpenedDocuments());
}

void CppcheckTrigger::updateProjectFiles(Project *project)
{
    if (project != m_currentProject)
        return;

    m_checkedFiles.clear();
    remove({});
    m_tool.setProject(project);
    checkEditors(DocumentModel::editorsForOpenedDocuments());
}

void CppcheckTrigger::check(const FilePaths &files)
{
    m_tool.check(files);
}

void CppcheckTrigger::remove(const FilePaths &files)
{
    m_marks.clearFiles(files);
    m_tool.stop(files);
}

} // Cppcheck::Internal
