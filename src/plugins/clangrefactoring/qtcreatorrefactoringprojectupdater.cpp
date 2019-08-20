/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include "qtcreatorrefactoringprojectupdater.h"

#include <cpptools/abstracteditorsupport.h>
#include <cpptools/cppmodelmanager.h>

namespace ClangRefactoring {

namespace {

CppTools::CppModelManager *cppModelManager()
{
    return CppTools::CppModelManager::instance();
}

std::vector<ClangBackEnd::V2::FileContainer> createGeneratedFiles(
    ClangBackEnd::FilePathCachingInterface &filePathCache)
{
    auto abstractEditors = CppTools::CppModelManager::instance()->abstractEditorSupports();
    std::vector<ClangBackEnd::V2::FileContainer> generatedFiles;
    generatedFiles.reserve(std::size_t(abstractEditors.size()));

    auto toFileContainer = [&](const CppTools::AbstractEditorSupport *abstractEditor) {
        ClangBackEnd::FilePath filePath{abstractEditor->fileName()};
        ClangBackEnd::FilePathId filePathId = filePathCache.filePathId(filePath);
        return ClangBackEnd::V2::FileContainer(std::move(filePath),
                                               filePathId,
                                               Utils::SmallString::fromQByteArray(
                                                   abstractEditor->contents()),
                                               {});
    };

    std::transform(abstractEditors.begin(),
                   abstractEditors.end(),
                   std::back_inserter(generatedFiles),
                   toFileContainer);

    std::sort(generatedFiles.begin(), generatedFiles.end());

    return generatedFiles;
}
}

QtCreatorRefactoringProjectUpdater::QtCreatorRefactoringProjectUpdater(
    ClangBackEnd::ProjectManagementServerInterface &server,
    ClangPchManager::PchManagerClient &pchManagerClient,
    ClangBackEnd::FilePathCachingInterface &filePathCache,
    ClangBackEnd::ProjectPartsStorageInterface &projectPartsStorage,
    ClangPchManager::ClangIndexingSettingsManager &settingsManager)
    : RefactoringProjectUpdater(server,
                                pchManagerClient,
                                *cppModelManager(),
                                filePathCache,
                                projectPartsStorage,
                                settingsManager)
{
    connectToCppModelManager();
}

void QtCreatorRefactoringProjectUpdater::abstractEditorUpdated(const QString &qFilePath,
                                                               const QByteArray &contents)
{
    ClangBackEnd::FilePath filePath{qFilePath};
    ClangBackEnd::FilePathId filePathId = m_filePathCache.filePathId(filePath);
    RefactoringProjectUpdater::updateGeneratedFiles({{std::move(filePath), filePathId, contents}});
}

void QtCreatorRefactoringProjectUpdater::abstractEditorRemoved(const QString &filePath)
{
    RefactoringProjectUpdater::removeGeneratedFiles({ClangBackEnd::FilePath{filePath}});
}

void QtCreatorRefactoringProjectUpdater::connectToCppModelManager()
{
    RefactoringProjectUpdater::updateGeneratedFiles(createGeneratedFiles(m_filePathCache));

    QObject::connect(cppModelManager(),
                     &CppTools::CppModelManager::abstractEditorSupportContentsUpdated,
                     [&] (const QString &filePath, const QString &, const QByteArray &contents) {
        abstractEditorUpdated(filePath, contents);
    });

    QObject::connect(cppModelManager(),
                     &CppTools::CppModelManager::abstractEditorSupportRemoved,
                     [&] (const QString &filePath) {
        abstractEditorRemoved(filePath);
    });
}

} // namespace ClangRefactoring
