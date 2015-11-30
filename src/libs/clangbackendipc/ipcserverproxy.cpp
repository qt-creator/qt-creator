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

#include "ipcserverproxy.h"

#include <cmbalivemessage.h>
#include <cmbcompletecodemessage.h>
#include <cmbendmessage.h>
#include <cmbregisterprojectsforeditormessage.h>
#include <cmbregistertranslationunitsforeditormessage.h>
#include <cmbunregisterprojectsforeditormessage.h>
#include <cmbunregistertranslationunitsforeditormessage.h>
#include <ipcclientinterface.h>
#include <registerunsavedfilesforeditormessage.h>
#include <requestdiagnosticsmessage.h>
#include <unregisterunsavedfilesforeditormessage.h>
#include <updatetranslationunitsforeditormessage.h>
#include <updatevisibletranslationunitsmessage.h>

#include <QLocalServer>
#include <QLocalSocket>
#include <QProcess>

namespace ClangBackEnd {

IpcServerProxy::IpcServerProxy(IpcClientInterface *client, QIODevice *ioDevice)
    : writeMessageBlock(ioDevice),
      readMessageBlock(ioDevice),
      client(client)
{
    QObject::connect(ioDevice, &QIODevice::readyRead, [this] () {IpcServerProxy::readMessages();});
}

void IpcServerProxy::readMessages()
{
    for (const QVariant &message : readMessageBlock.readAll())
        client->dispatch(message);
}

void IpcServerProxy::resetCounter()
{
    writeMessageBlock.resetCounter();
    readMessageBlock.resetCounter();
}

void IpcServerProxy::end()
{
    writeMessageBlock.write(QVariant::fromValue(EndMessage()));
}

void IpcServerProxy::registerTranslationUnitsForEditor(const RegisterTranslationUnitForEditorMessage &message)
{
    writeMessageBlock.write(QVariant::fromValue(message));
}

void IpcServerProxy::updateTranslationUnitsForEditor(const ClangBackEnd::UpdateTranslationUnitsForEditorMessage &message)
{
    writeMessageBlock.write(QVariant::fromValue(message));
}

void IpcServerProxy::unregisterTranslationUnitsForEditor(const UnregisterTranslationUnitsForEditorMessage &message)
{
    writeMessageBlock.write(QVariant::fromValue(message));
}

void IpcServerProxy::registerProjectPartsForEditor(const RegisterProjectPartsForEditorMessage &message)
{
    writeMessageBlock.write(QVariant::fromValue(message));
}

void IpcServerProxy::unregisterProjectPartsForEditor(const UnregisterProjectPartsForEditorMessage &message)
{
    writeMessageBlock.write(QVariant::fromValue(message));
}

void ClangBackEnd::IpcServerProxy::registerUnsavedFilesForEditor(const ClangBackEnd::RegisterUnsavedFilesForEditorMessage &message)
{
    writeMessageBlock.write(QVariant::fromValue(message));
}

void ClangBackEnd::IpcServerProxy::unregisterUnsavedFilesForEditor(const ClangBackEnd::UnregisterUnsavedFilesForEditorMessage &message)
{
    writeMessageBlock.write(QVariant::fromValue(message));
}

void IpcServerProxy::completeCode(const CompleteCodeMessage &message)
{
    writeMessageBlock.write(QVariant::fromValue(message));
}

void IpcServerProxy::requestDiagnostics(const ClangBackEnd::RequestDiagnosticsMessage &message)
{
    writeMessageBlock.write(QVariant::fromValue(message));
}

void IpcServerProxy::updateVisibleTranslationUnits(const UpdateVisibleTranslationUnitsMessage &message)
{
    writeMessageBlock.write(QVariant::fromValue(message));
}

} // namespace ClangBackEnd

