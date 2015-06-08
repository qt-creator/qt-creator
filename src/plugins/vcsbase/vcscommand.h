/**************************************************************************
**
** Copyright (C) 2015 Brian McGillion and Hugues Delorme
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
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
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef VCSBASE_COMMAND_H
#define VCSBASE_COMMAND_H

#include "vcsbase_global.h"

#include <coreplugin/shellcommand.h>

namespace VcsBase {

class VCSBASE_EXPORT VcsCommand : public Core::ShellCommand
{
    Q_OBJECT

public:
    enum VcsRunFlags {
        SshPasswordPrompt = 0x1000, // Disable terminal on UNIX to force graphical prompt.
        ExpectRepoChanges = 0x2000, // Expect changes in repository by the command
    };

    VcsCommand(const QString &defaultWorkingDirectory, const QProcessEnvironment &environment);

    const QProcessEnvironment processEnvironment() const;

    Utils::SynchronousProcessResponse runCommand(const Utils::FileName &binary,
                                                 const QStringList &arguments, int timeoutS,
                                                 const QString &workDirectory = QString(),
                                                 Utils::ExitCodeInterpreter *interpreter = 0);

    bool runFullySynchronous(const Utils::FileName &binary, const QStringList &arguments,
                             int timeoutS, QByteArray *outputData, QByteArray *errorData,
                             const QString &workingDirectory = QString());
private:
    unsigned processFlags() const;
    void emitRepositoryChanged(const QString &workingDirectory);

    void coreAboutToClose();

    bool m_preventRepositoryChanged;
};

} // namespace VcsBase

#endif // VCSBASE_COMMAND_H
