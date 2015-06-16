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

#include "cmbregistertranslationunitsforcodecompletioncommand.h"

#include "container_common.h"

#include <QDataStream>
#include <QDebug>

#include <ostream>

namespace ClangBackEnd {

RegisterTranslationUnitForCodeCompletionCommand::RegisterTranslationUnitForCodeCompletionCommand(const QVector<FileContainer> &fileContainers)
    : fileContainers_(fileContainers)
{
}

const QVector<FileContainer> &RegisterTranslationUnitForCodeCompletionCommand::fileContainers() const
{
    return fileContainers_;
}

QDataStream &operator<<(QDataStream &out, const RegisterTranslationUnitForCodeCompletionCommand &command)
{
    out << command.fileContainers_;

    return out;
}

QDataStream &operator>>(QDataStream &in, RegisterTranslationUnitForCodeCompletionCommand &command)
{
    in >> command.fileContainers_;

    return in;
}

bool operator==(const RegisterTranslationUnitForCodeCompletionCommand &first, const RegisterTranslationUnitForCodeCompletionCommand &second)
{
    return first.fileContainers_ == second.fileContainers_;
}

bool operator<(const RegisterTranslationUnitForCodeCompletionCommand &first, const RegisterTranslationUnitForCodeCompletionCommand &second)
{
    return compareContainer(first.fileContainers_, second.fileContainers_);
}

QDebug operator<<(QDebug debug, const RegisterTranslationUnitForCodeCompletionCommand &command)
{
    debug.nospace() << "RegisterTranslationUnitForCodeCompletionCommand(";

    for (const FileContainer &fileContainer : command.fileContainers())
        debug.nospace() << fileContainer<< ", ";

    debug.nospace() << ")";

    return debug;
}

void PrintTo(const RegisterTranslationUnitForCodeCompletionCommand &command, ::std::ostream* os)
{
    *os << "RegisterTranslationUnitForCodeCompletionCommand(";

    for (const FileContainer &fileContainer : command.fileContainers())
        PrintTo(fileContainer, os);

    *os << ")";
}

} // namespace ClangBackEnd

