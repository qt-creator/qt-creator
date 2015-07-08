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

#include "cmbunregistertranslationunitsforcodecompletioncommand.h"

#include "container_common.h"

#ifdef CLANGBACKEND_TESTS
#include <gtest/gtest-printers.h>
#endif

#include <QDataStream>
#include <QDebug>

namespace ClangBackEnd {


UnregisterTranslationUnitsForCodeCompletionCommand::UnregisterTranslationUnitsForCodeCompletionCommand(const QVector<FileContainer> &fileContainers)
    : fileContainers_(fileContainers)
{
}

const QVector<FileContainer> &UnregisterTranslationUnitsForCodeCompletionCommand::fileContainers() const
{
    return fileContainers_;
}

QDataStream &operator<<(QDataStream &out, const UnregisterTranslationUnitsForCodeCompletionCommand &command)
{
    out << command.fileContainers_;

    return out;
}

QDataStream &operator>>(QDataStream &in, UnregisterTranslationUnitsForCodeCompletionCommand &command)
{
    in >> command.fileContainers_;

    return in;
}

bool operator==(const UnregisterTranslationUnitsForCodeCompletionCommand &first, const UnregisterTranslationUnitsForCodeCompletionCommand &second)
{
    return first.fileContainers_ == second.fileContainers_;
}

bool operator<(const UnregisterTranslationUnitsForCodeCompletionCommand &first, const UnregisterTranslationUnitsForCodeCompletionCommand &second)
{
    return compareContainer(first.fileContainers_, second.fileContainers_);
}

QDebug operator<<(QDebug debug, const UnregisterTranslationUnitsForCodeCompletionCommand &command)
{
    debug.nospace() << "UnregisterTranslationUnitsForCodeCompletionCommand(";

    for (const FileContainer &fileContainer : command.fileContainers())
        debug.nospace() << fileContainer << ", ";

    debug.nospace() << ")";

    return debug;
}

#ifdef CLANGBACKEND_TESTS
void PrintTo(const UnregisterTranslationUnitsForCodeCompletionCommand &command, ::std::ostream* os)
{
    *os << "UnregisterTranslationUnitsForCodeCompletionCommand(";

    for (const FileContainer &fileContainer : command.fileContainers())
        *os << ::testing::PrintToString(fileContainer) << ", ";

    *os << ")";
}
#endif


} // namespace ClangBackEnd

