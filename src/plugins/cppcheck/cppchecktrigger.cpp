/****************************************************************************
**
** Copyright (C) 2018 Sergey Morozov
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

#include "cppchecktextmarkmanager.h"
#include "cppchecktool.h"
#include "cppchecktrigger.h"

#include <cpptools/cppmodelmanager.h>

#include <utils/qtcassert.h>

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>

#include <projectexplorer/project.h>
#include <projectexplorer/session.h>

namespace Cppcheck {
namespace Internal {

CppcheckTrigger::CppcheckTrigger(CppcheckTextMarkManager &marks, CppcheckTool &tool) :
      m_marks(marks),
      m_tool(tool)
{
    using EditorManager = Core::EditorManager;
    using SessionManager = ProjectExplorer::SessionManager;
    using CppModelManager = CppTools::CppModelManager;

    connect(EditorManager::instance(), &EditorManager::editorOpened,
            this, [this](Core::IEditor *editor) {checkEditors({editor});});
    connect(EditorManager::instance(), &EditorManager::editorsClosed,
            this, &CppcheckTrigger::removeEditors);
    connect(EditorManager::instance(), &EditorManager::aboutToSave,
            this, &CppcheckTrigger::checkChangedDocument);

    connect(SessionManager::instance(), &SessionManager::startupProjectChanged,
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

void CppcheckTrigger::checkEditors(const QList<Core::IEditor *> &editors)
{
    if (!m_currentProject)
        return;

    using CppModelManager = CppTools::CppModelManager;
    const CppTools::ProjectInfo info = CppModelManager::instance()->projectInfo(m_currentProject);
    if (!info.isValid())
        return;

    const QList<Core::IEditor *> editorList = !editors.isEmpty()
            ? editors : Core::DocumentModel::editorsForOpenedDocuments();

    Utils::FileNameList toCheck;
    for (const Core::IEditor *editor : editorList) {
        QTC_ASSERT(editor, continue);
        Core::IDocument *document = editor->document();
        QTC_ASSERT(document, continue);
        const Utils::FileName &path = document->filePath();
        if (path.isEmpty())
            continue;

        if (m_checkedFiles.contains(path))
            continue;

        if (!m_currentProject->isKnownFile(path))
            continue;

        const QString &pathString = path.toString();
        if (!info.sourceFiles().contains(pathString))
            continue;

        connect(document, &Core::IDocument::aboutToReload,
                this, [this, document]{checkChangedDocument(document);});
        connect(document, &Core::IDocument::contentsChanged,
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

void CppcheckTrigger::removeEditors(const QList<Core::IEditor *> &editors)
{
    if (!m_currentProject)
        return;

    const QList<Core::IEditor *> editorList = !editors.isEmpty()
            ? editors : Core::DocumentModel::editorsForOpenedDocuments();

    Utils::FileNameList toRemove;
    for (const Core::IEditor *editor : editorList) {
        QTC_ASSERT(editor, return);
        const Core::IDocument *document = editor->document();
        QTC_ASSERT(document, return);
        const Utils::FileName &path = document->filePath();
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

void CppcheckTrigger::checkChangedDocument(Core::IDocument *document)
{
    QTC_ASSERT(document, return);

    if (!m_currentProject)
        return;

    const Utils::FileName &path = document->filePath();
    QTC_ASSERT(!path.isEmpty(), return);
    if (!m_checkedFiles.contains(path))
        return;

    remove({path});
    check({path});
}

void CppcheckTrigger::changeCurrentProject(ProjectExplorer::Project *project)
{
    m_currentProject = project;
    m_checkedFiles.clear();
    remove({});
    m_tool.setProject(project);
    checkEditors(Core::DocumentModel::editorsForOpenedDocuments());
}

void CppcheckTrigger::updateProjectFiles(ProjectExplorer::Project *project)
{
    if (project != m_currentProject)
        return;

    m_checkedFiles.clear();
    remove({});
    m_tool.setProject(project);
    checkEditors(Core::DocumentModel::editorsForOpenedDocuments());
}

void CppcheckTrigger::check(const Utils::FileNameList &files)
{
    m_tool.check(files);
}

void CppcheckTrigger::remove(const Utils::FileNameList &files)
{
    m_marks.clearFiles(files);
    m_tool.stop(files);
}

} // namespace Internal
} // namespace Cppcheck
