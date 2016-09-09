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

ProjectPartDoNotExistException::ProjectPartDoNotExistException(
        const Utf8StringVector &projectPartIds)
    : projectPartIds_(projectPartIds)
{
}

const Utf8StringVector &ProjectPartDoNotExistException::projectPartIds() const
{
    return projectPartIds_;
}

const char *ProjectPartDoNotExistException::what() const Q_DECL_NOEXCEPT
{
    if (what_.isEmpty())
        what_ += Utf8StringLiteral("ProjectPart files ")
                + projectPartIds().join(Utf8StringLiteral(", "))
                + Utf8StringLiteral(" does not exist!");

    return what_.constData();
}

TranslationUnitAlreadyExistsException::TranslationUnitAlreadyExistsException(
        const FileContainer &fileContainer)
    : fileContainer_(fileContainer)
{
}

TranslationUnitAlreadyExistsException::TranslationUnitAlreadyExistsException(
        const Utf8String &filePath,
        const Utf8String &projectPartId)
    : fileContainer_(filePath, projectPartId)
{
}

const FileContainer &TranslationUnitAlreadyExistsException::fileContainer() const
{
    return fileContainer_;
}

const char *TranslationUnitAlreadyExistsException::what() const Q_DECL_NOEXCEPT
{
    if (what_.isEmpty()) {
        what_ += Utf8StringLiteral("Translation unit '")
                + fileContainer_.filePath()
                + Utf8StringLiteral("' with the project part id '")
                + fileContainer_.projectPartId()
                + Utf8StringLiteral("' already exists!");
    }

    return what_.constData();
}

TranslationUnitDoesNotExistException::TranslationUnitDoesNotExistException(
        const FileContainer &fileContainer)
    : fileContainer_(fileContainer)
{
}

TranslationUnitDoesNotExistException::TranslationUnitDoesNotExistException(
        const Utf8String &filePath,
        const Utf8String &projectPartId)
    : fileContainer_(filePath, projectPartId)
{
}

const FileContainer &TranslationUnitDoesNotExistException::fileContainer() const
{
    return fileContainer_;
}

const char *TranslationUnitDoesNotExistException::what() const Q_DECL_NOEXCEPT
{
    if (what_.isEmpty())
        what_ += Utf8StringLiteral("Translation unit '")
                + fileContainer_.filePath()
                + Utf8StringLiteral("' with the project part id '")
                + fileContainer_.projectPartId()
                + Utf8StringLiteral("' does not exits!");

    return what_.constData();
}

TranslationUnitFileNotExitsException::TranslationUnitFileNotExitsException(
        const Utf8String &filePath)
    : filePath_(filePath)
{
}

const Utf8String &TranslationUnitFileNotExitsException::filePath() const
{
    return filePath_;
}

const char *TranslationUnitFileNotExitsException::what() const Q_DECL_NOEXCEPT
{
    if (what_.isEmpty())
        what_ += Utf8StringLiteral("File ")
                + filePath()
                + Utf8StringLiteral(" does not exist in file system!");

    return what_.constData();
}

const char *TranslationUnitIsNullException::what() const Q_DECL_NOEXCEPT
{
    return "Tried to access a null TranslationUnit!";
}

TranslationUnitParseErrorException::TranslationUnitParseErrorException(
        const Utf8String &filePath,
        const Utf8String &projectPartId,
        CXErrorCode errorCode)
    : filePath_(filePath),
      projectPartId_(projectPartId),
      errorCode_(errorCode)
{
}

const Utf8String &TranslationUnitParseErrorException::filePath() const
{
    return filePath_;
}

const Utf8String &TranslationUnitParseErrorException::projectPartId() const
{
    return projectPartId_;
}

#define RETURN_TEXT_FOR_CASE(enumValue) case enumValue: return #enumValue
static const char *errorCodeToText(CXErrorCode errorCode)
{
    switch (errorCode) {
        RETURN_TEXT_FOR_CASE(CXError_Success);
        RETURN_TEXT_FOR_CASE(CXError_Failure);
        RETURN_TEXT_FOR_CASE(CXError_Crashed);
        RETURN_TEXT_FOR_CASE(CXError_InvalidArguments);
        RETURN_TEXT_FOR_CASE(CXError_ASTReadError);
    }

    return "UnknownCXErrorCode";
}
#undef RETURN_TEXT_FOR_CASE

const char *TranslationUnitParseErrorException::what() const Q_DECL_NOEXCEPT
{
    if (what_.isEmpty()) {
        what_ += Utf8StringLiteral("clang_parseTranslationUnit() failed for file ")
                + filePath()
                + Utf8StringLiteral(" in project ")
                + projectPartId()
                + Utf8StringLiteral(": ")
                + Utf8String::fromUtf8(errorCodeToText(errorCode_))
                + Utf8StringLiteral(".");
    }

    return what_.constData();
}

TranslationUnitReparseErrorException::TranslationUnitReparseErrorException(
        const Utf8String &filePath,
        const Utf8String &projectPartId,
        int errorCode)
    : filePath_(filePath),
      projectPartId_(projectPartId),
      errorCode_(errorCode)
{
}

const Utf8String &TranslationUnitReparseErrorException::filePath() const
{
    return filePath_;
}

const Utf8String &TranslationUnitReparseErrorException::projectPartId() const
{
    return projectPartId_;
}

const char *TranslationUnitReparseErrorException::what() const Q_DECL_NOEXCEPT
{
    if (what_.isEmpty()) {
        what_ += Utf8StringLiteral("clang_reparseTranslationUnit() failed for file ")
                + filePath()
                + Utf8StringLiteral(" in project ")
                + projectPartId()
                + Utf8StringLiteral(": ")
                + Utf8String::fromString(QString::number(errorCode_))
                + Utf8StringLiteral(".");
    }

    return what_.constData();
}

} // namespace ClangBackEnd
