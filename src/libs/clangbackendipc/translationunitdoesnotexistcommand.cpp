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

#include "translationunitdoesnotexistcommand.h"

#include <QDataStream>
#include <QDebug>

#include <ostream>

namespace ClangBackEnd {

TranslationUnitDoesNotExistCommand::TranslationUnitDoesNotExistCommand(const FileContainer &fileContainer)
    : fileContainer_(fileContainer)
{
}

TranslationUnitDoesNotExistCommand::TranslationUnitDoesNotExistCommand(const Utf8String &filePath, const Utf8String &projectPartId)
    : fileContainer_(filePath, projectPartId)
{
}

const FileContainer &TranslationUnitDoesNotExistCommand::fileContainer() const
{
    return fileContainer_;
}

const Utf8String &TranslationUnitDoesNotExistCommand::filePath() const
{
    return fileContainer_.filePath();
}

const Utf8String &TranslationUnitDoesNotExistCommand::projectPartId() const
{
    return fileContainer_.projectPartId();
}

QDataStream &operator<<(QDataStream &out, const TranslationUnitDoesNotExistCommand &command)
{
    out << command.fileContainer_;

    return out;
}

QDataStream &operator>>(QDataStream &in, TranslationUnitDoesNotExistCommand &command)
{
    in >> command.fileContainer_;

    return in;
}

bool operator==(const TranslationUnitDoesNotExistCommand &first, const TranslationUnitDoesNotExistCommand &second)
{
    return first.fileContainer_ == second.fileContainer_;
}

bool operator<(const TranslationUnitDoesNotExistCommand &first, const TranslationUnitDoesNotExistCommand &second)
{
    return first.fileContainer_ < second.fileContainer_;
}

QDebug operator<<(QDebug debug, const TranslationUnitDoesNotExistCommand &command)
{
    debug.nospace() << "TranslationUnitDoesNotExistCommand(";

    debug.nospace() << command.fileContainer_;

    debug.nospace() << ")";

    return debug;
}

void PrintTo(const TranslationUnitDoesNotExistCommand &command, ::std::ostream* os)
{
    QString output;
    QDebug debug(&output);

    debug << command;

    *os << output.toUtf8().constData();
}

} // namespace ClangBackEnd

