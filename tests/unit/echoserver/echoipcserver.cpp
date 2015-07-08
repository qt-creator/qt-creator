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

#include "echoipcserver.h"

#include "cmbcodecompletedcommand.h"
#include "cmbcompletecodecommand.h"
#include "cmbechocommand.h"
#include "cmbendcommand.h"
#include "cmbregisterprojectsforcodecompletioncommand.h"
#include "cmbregistertranslationunitsforcodecompletioncommand.h"
#include "cmbunregisterprojectsforcodecompletioncommand.h"
#include "cmbunregistertranslationunitsforcodecompletioncommand.h"
#include "connectionserver.h"

#include <QCoreApplication>
#include <QDebug>


namespace ClangBackEnd {

void EchoIpcServer::dispatch(const QVariant &command)
{
    IpcServerInterface::dispatch(command);
}

void EchoIpcServer::end()
{
    ConnectionServer::removeServer();
    QCoreApplication::quit();
}

void EchoIpcServer::registerTranslationUnitsForCodeCompletion(const RegisterTranslationUnitForCodeCompletionCommand &command)
{
    echoCommand(QVariant::fromValue(command));
}

void EchoIpcServer::unregisterTranslationUnitsForCodeCompletion(const UnregisterTranslationUnitsForCodeCompletionCommand &command)
{
    echoCommand(QVariant::fromValue(command));
}

void EchoIpcServer::registerProjectPartsForCodeCompletion(const RegisterProjectPartsForCodeCompletionCommand &command)
{
    echoCommand(QVariant::fromValue(command));
}

void EchoIpcServer::unregisterProjectPartsForCodeCompletion(const UnregisterProjectPartsForCodeCompletionCommand &command)
{
    echoCommand(QVariant::fromValue(command));
}

void EchoIpcServer::completeCode(const CompleteCodeCommand &command)
{
    echoCommand(QVariant::fromValue(command));
}

void EchoIpcServer::echoCommand(const QVariant &command)
{
    client()->echo(EchoCommand(command));
}

} // namespace ClangBackEnd

