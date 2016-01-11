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

#ifndef CLANGBACKEND_TEMPORARYMODIFIEDUNSAVEDFILES_H
#define CLANGBACKEND_TEMPORARYMODIFIEDUNSAVEDFILES_H

#include <clang-c/Index.h>

#include <utf8string.h>

#include <vector>

namespace ClangBackEnd {

class TemporaryModifiedUnsavedFiles
{
public:
    TemporaryModifiedUnsavedFiles(const std::vector<CXUnsavedFile> &unsavedFilesVector);

    TemporaryModifiedUnsavedFiles(const TemporaryModifiedUnsavedFiles &) = delete;

    bool replaceInFile(const Utf8String &filePath,
                       uint offset,
                       uint length,
                       const Utf8String &replacement);

    CXUnsavedFile cxUnsavedFileAt(uint index);
    CXUnsavedFile *cxUnsavedFiles();
    uint count();

private:
    bool replaceInFile_internal(CXUnsavedFile &unsavedFile,
                                uint offset,
                                uint length,
                                const Utf8String &replacement);

private:
    std::vector<CXUnsavedFile> m_unsavedFileVector;
    std::vector<Utf8String> m_modifiedContents;
};

} // namespace ClangBackEnd

#endif // CLANGBACKEND_TEMPORARYMODIFIEDUNSAVEDFILES_H
