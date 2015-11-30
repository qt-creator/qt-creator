/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "translationunitdoesnotexistmessage.h"

#include <QDataStream>
#include <QDebug>

#include <ostream>

namespace ClangBackEnd {

TranslationUnitDoesNotExistMessage::TranslationUnitDoesNotExistMessage(const FileContainer &fileContainer)
    : fileContainer_(fileContainer)
{
}

TranslationUnitDoesNotExistMessage::TranslationUnitDoesNotExistMessage(const Utf8String &filePath, const Utf8String &projectPartId)
    : fileContainer_(filePath, projectPartId)
{
}

const FileContainer &TranslationUnitDoesNotExistMessage::fileContainer() const
{
    return fileContainer_;
}

const Utf8String &TranslationUnitDoesNotExistMessage::filePath() const
{
    return fileContainer_.filePath();
}

const Utf8String &TranslationUnitDoesNotExistMessage::projectPartId() const
{
    return fileContainer_.projectPartId();
}

QDataStream &operator<<(QDataStream &out, const TranslationUnitDoesNotExistMessage &message)
{
    out << message.fileContainer_;

    return out;
}

QDataStream &operator>>(QDataStream &in, TranslationUnitDoesNotExistMessage &message)
{
    in >> message.fileContainer_;

    return in;
}

bool operator==(const TranslationUnitDoesNotExistMessage &first, const TranslationUnitDoesNotExistMessage &second)
{
    return first.fileContainer_ == second.fileContainer_;
}

bool operator<(const TranslationUnitDoesNotExistMessage &first, const TranslationUnitDoesNotExistMessage &second)
{
    return first.fileContainer_ < second.fileContainer_;
}

QDebug operator<<(QDebug debug, const TranslationUnitDoesNotExistMessage &message)
{
    debug.nospace() << "TranslationUnitDoesNotExistMessage(";

    debug.nospace() << message.fileContainer_;

    debug.nospace() << ")";

    return debug;
}

void PrintTo(const TranslationUnitDoesNotExistMessage &message, ::std::ostream* os)
{
    QString output;
    QDebug debug(&output);

    debug << message;

    *os << output.toUtf8().constData();
}

} // namespace ClangBackEnd

