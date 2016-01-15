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

#include <algorithm>
#include <cstring>

namespace ClangBackEnd {

class UnsavedFilesData
{
public:
    UnsavedFilesData();
    ~UnsavedFilesData();

public:
    time_point lastChangeTimePoint;
    std::vector<CXUnsavedFile> cxUnsavedFiles;
};

UnsavedFilesData::UnsavedFilesData()
    : lastChangeTimePoint(std::chrono::steady_clock::now())
{
}

UnsavedFilesData::~UnsavedFilesData()
{
    for (CXUnsavedFile &cxUnsavedFile : cxUnsavedFiles)
        UnsavedFiles::deleteCXUnsavedFile(cxUnsavedFile);

    cxUnsavedFiles.clear();
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
        updateCXUnsavedFileWithFileContainer(fileContainer);

    updateLastChangeTimePoint();
}

void UnsavedFiles::remove(const QVector<FileContainer> &fileContainers)
{
    for (const FileContainer &fileContainer : fileContainers)
        removeCXUnsavedFile(fileContainer);

    updateLastChangeTimePoint();
}

void UnsavedFiles::clear()
{
    for (CXUnsavedFile &cxUnsavedFile : d->cxUnsavedFiles)
        deleteCXUnsavedFile(cxUnsavedFile);

    d->cxUnsavedFiles.clear();
    updateLastChangeTimePoint();
}

uint UnsavedFiles::count() const
{
    return uint(d->cxUnsavedFiles.size());
}

CXUnsavedFile *UnsavedFiles::cxUnsavedFiles() const
{
    return d->cxUnsavedFiles.data();
}

const std::vector<CXUnsavedFile> &UnsavedFiles::cxUnsavedFileVector() const
{
    return d->cxUnsavedFiles;
}

const time_point &UnsavedFiles::lastChangeTimePoint() const
{
    return d->lastChangeTimePoint;
}

CXUnsavedFile UnsavedFiles::createCxUnsavedFile(const Utf8String &filePath, const Utf8String &fileContent)
{
    char *cxUnsavedFilePath = new char[filePath.byteSize() + 1];
    char *cxUnsavedFileContent = new char[fileContent.byteSize() + 1];

    std::memcpy(cxUnsavedFilePath, filePath.constData(), filePath.byteSize() + 1);
    std::memcpy(cxUnsavedFileContent, fileContent.constData(), fileContent.byteSize() + 1);

    return CXUnsavedFile { cxUnsavedFilePath, cxUnsavedFileContent, ulong(fileContent.byteSize())};
}

void UnsavedFiles::deleteCXUnsavedFile(CXUnsavedFile &cxUnsavedFile)
{
    delete [] cxUnsavedFile.Contents;
    delete [] cxUnsavedFile.Filename;
    cxUnsavedFile.Contents = nullptr;
    cxUnsavedFile.Filename = nullptr;
    cxUnsavedFile.Length = 0;
}

void UnsavedFiles::updateCXUnsavedFileWithFileContainer(const FileContainer &fileContainer)
{
    if (fileContainer.hasUnsavedFileContent()) {
        addOrUpdateCXUnsavedFile(fileContainer);
    } else {
        removeCXUnsavedFile(fileContainer);
    }
}

void UnsavedFiles::removeCXUnsavedFile(const FileContainer &fileContainer)
{
    const Utf8String filePath = fileContainer.filePath();
    auto removeBeginIterator = std::partition(d->cxUnsavedFiles.begin(),
                                              d->cxUnsavedFiles.end(),
                                              [filePath] (const CXUnsavedFile &cxUnsavedFile) { return filePath != cxUnsavedFile.Filename; });

    std::for_each(removeBeginIterator, d->cxUnsavedFiles.end(), UnsavedFiles::deleteCXUnsavedFile);
    d->cxUnsavedFiles.erase(removeBeginIterator, d->cxUnsavedFiles.end());
}

void UnsavedFiles::addOrUpdateCXUnsavedFile(const FileContainer &fileContainer)
{
    const Utf8String filePath = fileContainer.filePath();
    const Utf8String fileContent = fileContainer.unsavedFileContent();
    auto isSameFile = [filePath] (const CXUnsavedFile &cxUnsavedFile) { return filePath == cxUnsavedFile.Filename; };

    auto cxUnsavedFileIterator = std::find_if(d->cxUnsavedFiles.begin(), d->cxUnsavedFiles.end(), isSameFile);
    if (cxUnsavedFileIterator == d->cxUnsavedFiles.end())
        d->cxUnsavedFiles.push_back(createCxUnsavedFile(filePath, fileContent));
    else {
        deleteCXUnsavedFile(*cxUnsavedFileIterator);
        *cxUnsavedFileIterator = createCxUnsavedFile(filePath, fileContent);
    }
}

void UnsavedFiles::updateLastChangeTimePoint()
{
    d->lastChangeTimePoint = std::chrono::steady_clock::now();
}

} // namespace ClangBackEnd
