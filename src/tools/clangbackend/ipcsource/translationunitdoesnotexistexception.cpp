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

#include "translationunitdoesnotexistexception.h"

namespace ClangBackEnd {

TranslationUnitDoesNotExistException::TranslationUnitDoesNotExistException(const FileContainer &fileContainer)
    : fileContainer_(fileContainer)
{
}

TranslationUnitDoesNotExistException::TranslationUnitDoesNotExistException(const Utf8String &filePath, const Utf8String &projectPartId)
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

} // namespace ClangBackEnd

