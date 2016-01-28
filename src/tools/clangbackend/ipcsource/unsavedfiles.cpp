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

#include "unsavedfiles.h"

#include "unsavedfile.h"

#include <algorithm>

namespace ClangBackEnd {

class UnsavedFilesData
{
public:
    UnsavedFilesData();

public:
    time_point lastChangeTimePoint;
    std::vector<UnsavedFile> unsavedFiles;
};

UnsavedFilesData::UnsavedFilesData()
    : lastChangeTimePoint(std::chrono::steady_clock::now())
{
}

UnsavedFiles::UnsavedFiles()
    : d(std::make_shared<UnsavedFilesData>())
{
}

UnsavedFiles::~UnsavedFiles() = default;

UnsavedFiles::UnsavedFiles(const UnsavedFiles &) = default;
UnsavedFiles &UnsavedFiles::operator=(const UnsavedFiles &) = default;

UnsavedFiles::UnsavedFiles(UnsavedFiles &&other)
    : d(std::move(other.d))
{
}

UnsavedFiles &UnsavedFiles::operator=(UnsavedFiles &&other)
{
    d = std::move(other.d);

    return *this;
}

void UnsavedFiles::createOrUpdate(const QVector<FileContainer> &fileContainers)
{
    for (const FileContainer &fileContainer : fileContainers)
        updateUnsavedFileWithFileContainer(fileContainer);

    updateLastChangeTimePoint();
}

void UnsavedFiles::remove(const QVector<FileContainer> &fileContainers)
{
    for (const FileContainer &fileContainer : fileContainers)
        removeUnsavedFile(fileContainer);

    updateLastChangeTimePoint();
}

UnsavedFile &UnsavedFiles::unsavedFile(const Utf8String &filePath)
{
    const auto isMatchingFile = [filePath] (const UnsavedFile &unsavedFile) {
        return unsavedFile.filePath() == filePath;
    };
    const auto unsavedFileIterator = std::find_if(d->unsavedFiles.begin(),
                                                  d->unsavedFiles.end(),
                                                  isMatchingFile);

    if (unsavedFileIterator != d->unsavedFiles.end())
        return *unsavedFileIterator;

    static UnsavedFile defaultUnsavedFile = UnsavedFile(Utf8String(), Utf8String());
    return defaultUnsavedFile;
}

uint UnsavedFiles::count() const
{
    return uint(d->unsavedFiles.size());
}

CXUnsavedFile *UnsavedFiles::cxUnsavedFiles() const
{
    return d->unsavedFiles.data()->data();
}

const time_point &UnsavedFiles::lastChangeTimePoint() const
{
    return d->lastChangeTimePoint;
}

void UnsavedFiles::updateUnsavedFileWithFileContainer(const FileContainer &fileContainer)
{
    if (fileContainer.hasUnsavedFileContent())
        addOrUpdateUnsavedFile(fileContainer);
    else
        removeUnsavedFile(fileContainer);
}

void UnsavedFiles::removeUnsavedFile(const FileContainer &fileContainer)
{
    const Utf8String filePath = fileContainer.filePath();
    auto removeBeginIterator = std::partition(d->unsavedFiles.begin(),
                                              d->unsavedFiles.end(),
                                              [filePath] (const UnsavedFile &unsavedFile) { return filePath != unsavedFile.filePath(); });

    d->unsavedFiles.erase(removeBeginIterator, d->unsavedFiles.end());
}

void UnsavedFiles::addOrUpdateUnsavedFile(const FileContainer &fileContainer)
{
    const Utf8String filePath = fileContainer.filePath();
    const Utf8String fileContent = fileContainer.unsavedFileContent();
    auto isSameFile = [filePath] (const UnsavedFile &unsavedFile) { return filePath == unsavedFile.filePath(); };

    auto unsavedFileIterator = std::find_if(d->unsavedFiles.begin(), d->unsavedFiles.end(), isSameFile);
    if (unsavedFileIterator == d->unsavedFiles.end())
        d->unsavedFiles.emplace_back(filePath, fileContent);
    else
        *unsavedFileIterator = UnsavedFile(filePath, fileContent);
}

void UnsavedFiles::updateLastChangeTimePoint()
{
    d->lastChangeTimePoint = std::chrono::steady_clock::now();
}

} // namespace ClangBackEnd
