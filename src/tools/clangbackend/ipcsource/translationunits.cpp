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
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
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
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "translationunits.h"

#include <projectpartsdonotexistexception.h>
#include <projects.h>
#include <translationunitdoesnotexistexception.h>

namespace ClangBackEnd {

bool operator==(const FileContainer &fileContainer, const TranslationUnit &translationUnit)
{
    return fileContainer.filePath() == translationUnit.filePath() && fileContainer.projectPartId() == translationUnit.projectPartId();
}

bool operator==(const TranslationUnit &translationUnit, const FileContainer &fileContainer)
{
    return fileContainer == translationUnit;
}


TranslationUnits::TranslationUnits(ProjectParts &projects, UnsavedFiles &unsavedFiles)
    : projects(projects),
      unsavedFiles(unsavedFiles)
{
}

void TranslationUnits::createOrUpdate(const QVector<FileContainer> &fileContainers)
{
    for (const FileContainer &fileContainer : fileContainers)
        createOrUpdateTranslationUnit(fileContainer);
}

static bool removeFromFileContainer(QVector<FileContainer> &fileContainers, const TranslationUnit &translationUnit)
{
    auto position = std::remove(fileContainers.begin(), fileContainers.end(), translationUnit);

    bool entryIsRemoved = position != fileContainers.end();

    fileContainers.erase(position, fileContainers.end());

    return entryIsRemoved;
}


void TranslationUnits::remove(const QVector<FileContainer> &fileContainers)
{
    checkIfProjectPartsExists(fileContainers);

    QVector<FileContainer> processedFileContainers = fileContainers;

    auto removeBeginIterator = std::remove_if(translationUnits_.begin(), translationUnits_.end(), [&processedFileContainers] (const TranslationUnit &translationUnit) {
        return removeFromFileContainer(processedFileContainers, translationUnit);
    });

    translationUnits_.erase(removeBeginIterator, translationUnits_.end());

    if (!processedFileContainers.isEmpty())
        throw TranslationUnitDoesNotExistException(processedFileContainers.first());

}

const TranslationUnit &TranslationUnits::translationUnit(const Utf8String &filePath, const Utf8String &projectPartId) const
{
    checkIfProjectPartExists(projectPartId);

    auto findIterator = findTranslationUnit(filePath, projectPartId);

    if (findIterator == translationUnits_.end())
        throw TranslationUnitDoesNotExistException(FileContainer(filePath, projectPartId));

    return *findIterator;
}

const std::vector<TranslationUnit> &TranslationUnits::translationUnits() const
{
    return translationUnits_;
}

void TranslationUnits::createOrUpdateTranslationUnit(const FileContainer &fileContainer)
{
    TranslationUnit::FileExistsCheck checkIfFileExists = fileContainer.hasUnsavedFileContent() ? TranslationUnit::DoNotCheckIfFileExists : TranslationUnit::CheckIfFileExists;
    auto findIterator = findTranslationUnit(fileContainer);
    if (findIterator == translationUnits_.end())
        translationUnits_.push_back(TranslationUnit(fileContainer.filePath(),
                                                    unsavedFiles,
                                                    projects.project(fileContainer.projectPartId()),
                                                    checkIfFileExists));
}

std::vector<TranslationUnit>::iterator TranslationUnits::findTranslationUnit(const FileContainer &fileContainer)
{
    return std::find(translationUnits_.begin(), translationUnits_.end(), fileContainer);
}

std::vector<TranslationUnit>::const_iterator TranslationUnits::findTranslationUnit(const Utf8String &filePath, const Utf8String &projectPartId) const
{
    FileContainer fileContainer(filePath, projectPartId);
    return std::find(translationUnits_.begin(), translationUnits_.end(), fileContainer);
}

void TranslationUnits::checkIfProjectPartExists(const Utf8String &projectFileName) const
{
    projects.project(projectFileName);
}

void TranslationUnits::checkIfProjectPartsExists(const QVector<FileContainer> &fileContainers) const
{
    Utf8StringVector notExistingProjectParts;

    for (const FileContainer &fileContainer : fileContainers) {
        if (!projects.hasProjectPart(fileContainer.projectPartId()))
            notExistingProjectParts.push_back(fileContainer.projectPartId());
    }

    if (!notExistingProjectParts.isEmpty())
        throw ProjectPartDoNotExistException(notExistingProjectParts);

}

} // namespace ClangBackEnd

