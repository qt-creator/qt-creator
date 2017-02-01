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

#include "qtcreatorprojectupdater.h"

#include <cpptools/abstracteditorsupport.h>
#include <cpptools/cppmodelmanager.h>

#include <projectexplorer/project.h>

namespace ClangPchManager {

static CppTools::CppModelManager *cppModelManager()
{
    return CppTools::CppModelManager::instance();
}

QtCreatorProjectUpdater::QtCreatorProjectUpdater(ClangBackEnd::PchManagerServerInterface &server,
                                                 PchManagerClient &client)
    : ProjectUpdater(server, client)
{
    connectToCppModelManager();
}

namespace {

std::vector<ClangBackEnd::V2::FileContainer> createGeneratedFiles()
{
    auto abstractEditors = CppTools::CppModelManager::instance()->abstractEditorSupports();
    std::vector<ClangBackEnd::V2::FileContainer> generatedFiles;
    generatedFiles.reserve(std::size_t(abstractEditors.size()));

    auto toFileContainer = [] (const CppTools::AbstractEditorSupport *abstractEditor) {
        return  ClangBackEnd::V2::FileContainer(ClangBackEnd::FilePath(abstractEditor->fileName()),
                                                Utils::SmallString::fromQByteArray(abstractEditor->contents()),
                                                {});
    };

    std::transform(abstractEditors.begin(),
                   abstractEditors.end(),
                   std::back_inserter(generatedFiles),
                   toFileContainer);

    return generatedFiles;
}

std::vector<CppTools::ProjectPart*> createProjectParts(ProjectExplorer::Project *project)
{
    const CppTools::ProjectInfo projectInfo = cppModelManager()->projectInfo(project);

    const QVector<CppTools::ProjectPart::Ptr> projectPartSharedPointers = projectInfo.projectParts();

    std::vector<CppTools::ProjectPart*> projectParts;
    projectParts.reserve(std::size_t(projectPartSharedPointers.size()));

    auto convertToRawPointer = [] (const CppTools::ProjectPart::Ptr &sharedPointer) {
        return sharedPointer.data();
    };

    std::transform(projectPartSharedPointers.begin(),
                   projectPartSharedPointers.end(),
                   std::back_inserter(projectParts),
                   convertToRawPointer);

    return projectParts;
}

}

void QtCreatorProjectUpdater::projectPartsUpdated(ProjectExplorer::Project *project)
{
    updateProjectParts(createProjectParts(project), createGeneratedFiles());
}

void QtCreatorProjectUpdater::projectPartsRemoved(const QStringList &projectPartIds)
{
    removeProjectParts(projectPartIds);
}

void QtCreatorProjectUpdater::connectToCppModelManager()
{
    connect(cppModelManager(),
            &CppTools::CppModelManager::projectPartsUpdated,
            this,
            &QtCreatorProjectUpdater::projectPartsUpdated);
    connect(cppModelManager(),
            &CppTools::CppModelManager::projectPartsRemoved,
            this,
            &QtCreatorProjectUpdater::projectPartsRemoved);
}

} // namespace ClangPchManager
