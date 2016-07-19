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

#include "filecontainer.h"

namespace ClangBackEnd {

class TranslationUnitDoesNotExistMessage
{
public:
    TranslationUnitDoesNotExistMessage() = default;
    TranslationUnitDoesNotExistMessage(const FileContainer &fileContainer)
        : fileContainer_(fileContainer)
    {
    }

    TranslationUnitDoesNotExistMessage(const Utf8String &filePath, const Utf8String &projectPartId)
        : fileContainer_(filePath, projectPartId)
    {
    }

    const FileContainer &fileContainer() const
    {
        return fileContainer_;
    }

    const Utf8String &filePath() const
    {
        return fileContainer_.filePath();
    }

    const Utf8String &projectPartId() const
    {
        return fileContainer_.projectPartId();
    }

    friend QDataStream &operator<<(QDataStream &out, const TranslationUnitDoesNotExistMessage &message)
    {
        out << message.fileContainer_;

        return out;
    }

    friend QDataStream &operator>>(QDataStream &in, TranslationUnitDoesNotExistMessage &message)
    {
        in >> message.fileContainer_;

        return in;
    }

    friend bool operator==(const TranslationUnitDoesNotExistMessage &first, const TranslationUnitDoesNotExistMessage &second)
    {
        return first.fileContainer_ == second.fileContainer_;
    }

private:
    FileContainer fileContainer_;
};

CMBIPC_EXPORT QDebug operator<<(QDebug debug, const TranslationUnitDoesNotExistMessage &message);
void PrintTo(const TranslationUnitDoesNotExistMessage &message, ::std::ostream* os);

DECLARE_MESSAGE(TranslationUnitDoesNotExistMessage)
} // namespace ClangBackEnd
