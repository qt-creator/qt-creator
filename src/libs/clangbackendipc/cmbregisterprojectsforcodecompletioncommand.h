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

#ifndef CLANGBACKEND_REGISTERPROJECTSFORCODECOMPLETIONCOMAND_H
#define CLANGBACKEND_REGISTERPROJECTSFORCODECOMPLETIONCOMAND_H

#include "projectpartcontainer.h"

#include <QMetaType>
#include <QVector>

namespace ClangBackEnd {

class CMBIPC_EXPORT RegisterProjectPartsForCodeCompletionCommand
{
    friend CMBIPC_EXPORT QDataStream &operator<<(QDataStream &out, const RegisterProjectPartsForCodeCompletionCommand &command);
    friend CMBIPC_EXPORT QDataStream &operator>>(QDataStream &in, RegisterProjectPartsForCodeCompletionCommand &command);
    friend CMBIPC_EXPORT bool operator==(const RegisterProjectPartsForCodeCompletionCommand &first, const RegisterProjectPartsForCodeCompletionCommand &second);
    friend CMBIPC_EXPORT bool operator<(const RegisterProjectPartsForCodeCompletionCommand &first, const RegisterProjectPartsForCodeCompletionCommand &second);
    friend void PrintTo(const RegisterProjectPartsForCodeCompletionCommand &command, ::std::ostream* os);
public:
    RegisterProjectPartsForCodeCompletionCommand() = default;
    RegisterProjectPartsForCodeCompletionCommand(const QVector<ProjectPartContainer> &projectContainers);

    const QVector<ProjectPartContainer> &projectContainers() const;

private:
    QVector<ProjectPartContainer> projectContainers_;
};

CMBIPC_EXPORT QDataStream &operator<<(QDataStream &out, const RegisterProjectPartsForCodeCompletionCommand &command);
CMBIPC_EXPORT QDataStream &operator>>(QDataStream &in, RegisterProjectPartsForCodeCompletionCommand &command);
CMBIPC_EXPORT bool operator==(const RegisterProjectPartsForCodeCompletionCommand &first, const RegisterProjectPartsForCodeCompletionCommand &second);
CMBIPC_EXPORT bool operator<(const RegisterProjectPartsForCodeCompletionCommand &first, const RegisterProjectPartsForCodeCompletionCommand &second);

CMBIPC_EXPORT QDebug operator<<(QDebug debug, const RegisterProjectPartsForCodeCompletionCommand &command);
void PrintTo(const RegisterProjectPartsForCodeCompletionCommand &command, ::std::ostream* os);
} // namespace ClangBackEnd

Q_DECLARE_METATYPE(ClangBackEnd::RegisterProjectPartsForCodeCompletionCommand)

#endif // CLANGBACKEND_REGISTERPROJECTSFORCODECOMPLETIONCOMAND_H
