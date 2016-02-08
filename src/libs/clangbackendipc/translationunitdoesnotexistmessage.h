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

#ifndef CLANGBACKEND_TRANSLATIONUNITDOESNOTEXISTSMESSAGE_H
#define CLANGBACKEND_TRANSLATIONUNITDOESNOTEXISTSMESSAGE_H

#include "filecontainer.h"

namespace ClangBackEnd {

class CMBIPC_EXPORT TranslationUnitDoesNotExistMessage
{
    friend CMBIPC_EXPORT QDataStream &operator<<(QDataStream &out, const TranslationUnitDoesNotExistMessage &message);
    friend CMBIPC_EXPORT QDataStream &operator>>(QDataStream &in, TranslationUnitDoesNotExistMessage &message);
    friend CMBIPC_EXPORT bool operator==(const TranslationUnitDoesNotExistMessage &first, const TranslationUnitDoesNotExistMessage &second);
    friend CMBIPC_EXPORT QDebug operator<<(QDebug debug, const TranslationUnitDoesNotExistMessage &message);
    friend void PrintTo(const TranslationUnitDoesNotExistMessage &message, ::std::ostream* os);
public:
    TranslationUnitDoesNotExistMessage() = default;
    TranslationUnitDoesNotExistMessage(const FileContainer &fileContainer);
    TranslationUnitDoesNotExistMessage(const Utf8String &filePath, const Utf8String &projectPartId);

    const FileContainer &fileContainer() const;
    const Utf8String &filePath() const;
    const Utf8String &projectPartId() const;

private:
    FileContainer fileContainer_;
};

CMBIPC_EXPORT QDataStream &operator<<(QDataStream &out, const TranslationUnitDoesNotExistMessage &message);
CMBIPC_EXPORT QDataStream &operator>>(QDataStream &in, TranslationUnitDoesNotExistMessage &message);
CMBIPC_EXPORT bool operator==(const TranslationUnitDoesNotExistMessage &first, const TranslationUnitDoesNotExistMessage &second);

CMBIPC_EXPORT QDebug operator<<(QDebug debug, const TranslationUnitDoesNotExistMessage &message);
void PrintTo(const TranslationUnitDoesNotExistMessage &message, ::std::ostream* os);

DECLARE_MESSAGE(TranslationUnitDoesNotExistMessage)
} // namespace ClangBackEnd

#endif // CLANGBACKEND_TRANSLATIONUNITDOESNOTEXISTSMESSAGE_H
