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

#include "ipcserverproxy.h"

#include <cmbalivemessage.h>
#include <cmbcompletecodemessage.h>
#include <cmbendmessage.h>
#include <cmbregisterprojectsforeditormessage.h>
#include <cmbregistertranslationunitsforeditormessage.h>
#include <cmbunregisterprojectsforeditormessage.h>
#include <cmbunregistertranslationunitsforeditormessage.h>
#include <ipcclientinterface.h>
#include <messageenvelop.h>
#include <registerunsavedfilesforeditormessage.h>
#include <requestdiagnosticsmessage.h>
#include <requesthighlightingmessage.h>
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
    for (const auto &message : readMessageBlock.readAll())
        client->dispatch(message);
}

void IpcServerProxy::resetCounter()
{
    writeMessageBlock.resetCounter();
    readMessageBlock.resetCounter();
}

void IpcServerProxy::end()
{
    writeMessageBlock.write(EndMessage());
}

void IpcServerProxy::registerTranslationUnitsForEditor(const RegisterTranslationUnitForEditorMessage &message)
{
    writeMessageBlock.write(message);
}

void IpcServerProxy::updateTranslationUnitsForEditor(const ClangBackEnd::UpdateTranslationUnitsForEditorMessage &message)
{
    writeMessageBlock.write(message);
}

void IpcServerProxy::unregisterTranslationUnitsForEditor(const UnregisterTranslationUnitsForEditorMessage &message)
{
    writeMessageBlock.write(message);
}

void IpcServerProxy::registerProjectPartsForEditor(const RegisterProjectPartsForEditorMessage &message)
{
    writeMessageBlock.write(message);
}

void IpcServerProxy::unregisterProjectPartsForEditor(const UnregisterProjectPartsForEditorMessage &message)
{
    writeMessageBlock.write(message);
}

void ClangBackEnd::IpcServerProxy::registerUnsavedFilesForEditor(const ClangBackEnd::RegisterUnsavedFilesForEditorMessage &message)
{
    writeMessageBlock.write(message);
}

void ClangBackEnd::IpcServerProxy::unregisterUnsavedFilesForEditor(const ClangBackEnd::UnregisterUnsavedFilesForEditorMessage &message)
{
    writeMessageBlock.write(message);
}

void IpcServerProxy::completeCode(const CompleteCodeMessage &message)
{
    writeMessageBlock.write(message);
}

void IpcServerProxy::requestDiagnostics(const ClangBackEnd::RequestDiagnosticsMessage &message)
{
    writeMessageBlock.write(message);
}

void IpcServerProxy::requestHighlighting(const RequestHighlightingMessage &message)
{
    writeMessageBlock.write(message);
}

void IpcServerProxy::updateVisibleTranslationUnits(const UpdateVisibleTranslationUnitsMessage &message)
{
    writeMessageBlock.write(message);
}

} // namespace ClangBackEnd

