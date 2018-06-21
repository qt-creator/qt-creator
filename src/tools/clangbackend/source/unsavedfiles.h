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

#pragma once

#include "clangclock.h"

#include <filecontainer.h>

#include <QSharedPointer>
#include <QVector>

#include <clang-c/Index.h>

namespace ClangBackEnd {

class UnsavedFile;
class UnsavedFilesData;
class UnsavedFilesShallowArguments;

class UnsavedFiles
{
public:
    UnsavedFiles();
    ~UnsavedFiles();

    UnsavedFiles(const UnsavedFiles &other);
    UnsavedFiles &operator=(const UnsavedFiles &other);

    void createOrUpdate(const QVector<FileContainer> &fileContainers);
    void remove(const QVector<FileContainer> &fileContainers);

    uint count() const;
    const UnsavedFile &at(int index) const;

    UnsavedFile &unsavedFile(const Utf8String &filePath);

    UnsavedFilesShallowArguments shallowArguments() const;

    const TimePoint lastChangeTimePoint() const;

private:
    void updateUnsavedFileWithFileContainer(const FileContainer &fileContainer);
    void removeUnsavedFile(const FileContainer &fileContainer);
    void addOrUpdateUnsavedFile(const FileContainer &fileContainer);
    void updateLastChangeTimePoint();

private:
    QSharedDataPointer<UnsavedFilesData> d;
};

} // namespace ClangBackEnd
