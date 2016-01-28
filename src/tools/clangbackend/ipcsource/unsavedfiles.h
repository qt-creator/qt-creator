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

#ifndef CLANGBACKEND_UNSAVEDFILES_H
#define CLANGBACKEND_UNSAVEDFILES_H

#include <filecontainer.h>

#include <QVector>

#include <clang-c/Index.h>

#include <chrono>
#include <memory>

namespace ClangBackEnd {

using time_point = std::chrono::steady_clock::time_point;

class UnsavedFile;
class UnsavedFilesData;

class UnsavedFiles
{
    friend class UnsavedFilesData;
public:
    UnsavedFiles();
    ~UnsavedFiles();

    UnsavedFiles(const UnsavedFiles &unsavedFiles);
    UnsavedFiles &operator=(const UnsavedFiles &unsavedFiles);

    UnsavedFiles(UnsavedFiles &&unsavedFiles);
    UnsavedFiles &operator=(UnsavedFiles &&unsavedFiles);

    void createOrUpdate(const QVector<FileContainer> &fileContainers);
    void remove(const QVector<FileContainer> &fileContainers);

    UnsavedFile &unsavedFile(const Utf8String &filePath);

    uint count() const;
    CXUnsavedFile *cxUnsavedFiles() const;

    const time_point &lastChangeTimePoint() const;

private:
    void updateUnsavedFileWithFileContainer(const FileContainer &fileContainer);
    void removeUnsavedFile(const FileContainer &fileContainer);
    void addOrUpdateUnsavedFile(const FileContainer &fileContainer);
    void updateLastChangeTimePoint();

private:
    mutable std::shared_ptr<UnsavedFilesData> d;
};

} // namespace ClangBackEnd

#endif // CLANGBACKEND_UNSAVEDFILES_H
