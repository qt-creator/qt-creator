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

#include "clangexceptions.h"

namespace ClangBackEnd {

const char *ClangBaseException::what() const Q_DECL_NOEXCEPT
{
    return m_info.constData();
}

ProjectPartDoNotExistException::ProjectPartDoNotExistException(
        const Utf8StringVector &projectPartIds)
{
    m_info += Utf8StringLiteral("ProjectPart files ")
           + projectPartIds.join(Utf8StringLiteral(", "))
           + Utf8StringLiteral(" does not exist!");
}

DocumentAlreadyExistsException::DocumentAlreadyExistsException(
        const Utf8String &filePath,
        const Utf8String &projectPartId)
{
    m_info += Utf8StringLiteral("Document '")
            + filePath
            + Utf8StringLiteral("' with the project part id '")
            + projectPartId
            + Utf8StringLiteral("' already exists!");
}

DocumentDoesNotExistException::DocumentDoesNotExistException(const Utf8String &filePath,
                                                             const Utf8String &projectPartId)
{
    m_info += Utf8StringLiteral("Document '")
            + filePath
            + Utf8StringLiteral("' with the project part id '")
            + projectPartId
            + Utf8StringLiteral("' does not exists!");
}

DocumentFileDoesNotExistException::DocumentFileDoesNotExistException(
        const Utf8String &filePath)
{
    m_info += Utf8StringLiteral("File ")
            + filePath
            + Utf8StringLiteral(" does not exist in file system!");
}

DocumentIsNullException::DocumentIsNullException()
{
    m_info = Utf8String::fromUtf8("Tried to access a null Document!");
}

DocumentProcessorAlreadyExists::DocumentProcessorAlreadyExists(const Utf8String &filePath,
                                                               const Utf8String &projectPartId)
{
    m_info = Utf8StringLiteral("Document processor for file '")
           + filePath
           + Utf8StringLiteral("' and project part id '")
           + projectPartId
           + Utf8StringLiteral("' already exists!");
}

DocumentProcessorDoesNotExist::DocumentProcessorDoesNotExist(const Utf8String &filePath,
                                                             const Utf8String &projectPartId)
{
    m_info = Utf8StringLiteral("Document processor for file '")
           + filePath
           + Utf8StringLiteral("' and project part id '")
           + projectPartId
           + Utf8StringLiteral("' does not exist!");
}

TranslationUnitDoesNotExist::TranslationUnitDoesNotExist(const Utf8String &filePath)
{
    m_info += Utf8StringLiteral("TranslationUnit for file '")
            + filePath
            + Utf8StringLiteral("' does not exist.");
}

} // namespace ClangBackEnd
