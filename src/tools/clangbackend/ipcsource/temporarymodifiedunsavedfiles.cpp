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

#include "temporarymodifiedunsavedfiles.h"

#include <cstring>

namespace ClangBackEnd {

TemporaryModifiedUnsavedFiles::TemporaryModifiedUnsavedFiles(
        const std::vector<CXUnsavedFile> &unsavedFilesVector)
    : m_unsavedFileVector(unsavedFilesVector)
{
}

bool TemporaryModifiedUnsavedFiles::replaceInFile(const Utf8String &filePath,
                                                  uint offset,
                                                  uint length,
                                                  const Utf8String &replacement)
{
    const auto isMatchingFile = [filePath] (const CXUnsavedFile &unsavedFile) {
        return std::strcmp(unsavedFile.Filename, filePath.constData()) == 0;
    };
    const auto unsavedFileIterator = std::find_if(m_unsavedFileVector.begin(),
                                                  m_unsavedFileVector.end(),
                                                  isMatchingFile);

    if (unsavedFileIterator == m_unsavedFileVector.end())
        return false;

    return replaceInFile_internal(*unsavedFileIterator, offset, length, replacement);
}

CXUnsavedFile TemporaryModifiedUnsavedFiles::cxUnsavedFileAt(uint index)
{
    return m_unsavedFileVector[index];
}

CXUnsavedFile *TemporaryModifiedUnsavedFiles::cxUnsavedFiles()
{
    return m_unsavedFileVector.data();
}

uint TemporaryModifiedUnsavedFiles::count()
{
    return uint(m_unsavedFileVector.size());
}

bool TemporaryModifiedUnsavedFiles::replaceInFile_internal(CXUnsavedFile &unsavedFile,
                                                           uint offset,
                                                           uint length,
                                                           const Utf8String &replacement)
{
    auto modifiedContent = Utf8String::fromUtf8(unsavedFile.Contents);
    modifiedContent.replace(int(offset), int(length), replacement);

    unsavedFile.Contents = modifiedContent.constData();
    unsavedFile.Length = uint(modifiedContent.byteSize() + 1);

    m_modifiedContents.push_back(modifiedContent); // Keep the modified copy.

    return true;
}

} // namespace ClangBackEnd
