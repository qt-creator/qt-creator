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

#include <utf8stringvector.h>

#include <clang-c/Index.h>

#include <exception>

namespace ClangBackEnd {

class ClangBaseException : public std::exception
{
public:
    const char *what() const Q_DECL_NOEXCEPT override;

protected:
    Utf8String m_info;
};

class ProjectPartDoNotExistException : public ClangBaseException
{
public:
    ProjectPartDoNotExistException(const Utf8StringVector &projectPartIds);
};

class DocumentAlreadyExistsException : public ClangBaseException
{
public:
    DocumentAlreadyExistsException(const Utf8String &filePath,
                                   const Utf8String &projectPartId);
};

class DocumentDoesNotExistException : public ClangBaseException
{
public:
    DocumentDoesNotExistException(const Utf8String &filePath,
                                  const Utf8String &projectPartId);
};

class DocumentFileDoesNotExistException : public ClangBaseException
{
public:
    DocumentFileDoesNotExistException(const Utf8String &filePath);
};

class DocumentIsNullException : public ClangBaseException
{
public:
    DocumentIsNullException();
};

class DocumentProcessorAlreadyExists : public ClangBaseException
{
public:
    DocumentProcessorAlreadyExists(const Utf8String &filePath,
                                   const Utf8String &projectPartId);
};

class DocumentProcessorDoesNotExist : public ClangBaseException
{
public:
    DocumentProcessorDoesNotExist(const Utf8String &filePath,
                                  const Utf8String &projectPartId);
};

class TranslationUnitDoesNotExist : public ClangBaseException
{
public:
    TranslationUnitDoesNotExist(const Utf8String &filePath);
};

} // namespace ClangBackEnd
