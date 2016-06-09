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

#include "clangcodemodelserverproxy.h"

#include <cmbalivemessage.h>
#include <cmbcompletecodemessage.h>
#include <cmbendmessage.h>
#include <cmbregisterprojectsforeditormessage.h>
#include <cmbregistertranslationunitsforeditormessage.h>
#include <cmbunregisterprojectsforeditormessage.h>
#include <cmbunregistertranslationunitsforeditormessage.h>
#include <clangcodemodelclientinterface.h>
#include <messageenvelop.h>
#include <registerunsavedfilesforeditormessage.h>
#include <requestdocumentannotations.h>
#include <unregisterunsavedfilesforeditormessage.h>
#include <updatetranslationunitsforeditormessage.h>
#include <updatevisibletranslationunitsmessage.h>

#include <QLocalServer>
#include <QLocalSocket>
#include <QProcess>

namespace ClangBackEnd {

ClangCodeModelServerProxy::ClangCodeModelServerProxy(ClangCodeModelClientInterface *client, QIODevice *ioDevice)
    : writeMessageBlock(ioDevice),
      readMessageBlock(ioDevice),
      client(client)
{
    QObject::connect(ioDevice, &QIODevice::readyRead, [this] () {ClangCodeModelServerProxy::readMessages();});
}

void ClangCodeModelServerProxy::readMessages()
{
    for (const auto &message : readMessageBlock.readAll())
        client->dispatch(message);
}

void ClangCodeModelServerProxy::resetCounter()
{
    writeMessageBlock.resetCounter();
    readMessageBlock.resetCounter();
}

void ClangCodeModelServerProxy::end()
{
    writeMessageBlock.write(EndMessage());
}

void ClangCodeModelServerProxy::registerTranslationUnitsForEditor(const RegisterTranslationUnitForEditorMessage &message)
{
    writeMessageBlock.write(message);
}

void ClangCodeModelServerProxy::updateTranslationUnitsForEditor(const ClangBackEnd::UpdateTranslationUnitsForEditorMessage &message)
{
    writeMessageBlock.write(message);
}

void ClangCodeModelServerProxy::unregisterTranslationUnitsForEditor(const UnregisterTranslationUnitsForEditorMessage &message)
{
    writeMessageBlock.write(message);
}

void ClangCodeModelServerProxy::registerProjectPartsForEditor(const RegisterProjectPartsForEditorMessage &message)
{
    writeMessageBlock.write(message);
}

void ClangCodeModelServerProxy::unregisterProjectPartsForEditor(const UnregisterProjectPartsForEditorMessage &message)
{
    writeMessageBlock.write(message);
}

void ClangBackEnd::ClangCodeModelServerProxy::registerUnsavedFilesForEditor(const ClangBackEnd::RegisterUnsavedFilesForEditorMessage &message)
{
    writeMessageBlock.write(message);
}

void ClangBackEnd::ClangCodeModelServerProxy::unregisterUnsavedFilesForEditor(const ClangBackEnd::UnregisterUnsavedFilesForEditorMessage &message)
{
    writeMessageBlock.write(message);
}

void ClangCodeModelServerProxy::completeCode(const CompleteCodeMessage &message)
{
    writeMessageBlock.write(message);
}

void ClangCodeModelServerProxy::requestDocumentAnnotations(const RequestDocumentAnnotationsMessage &message)
{
    writeMessageBlock.write(message);
}

void ClangCodeModelServerProxy::updateVisibleTranslationUnits(const UpdateVisibleTranslationUnitsMessage &message)
{
    writeMessageBlock.write(message);
}

} // namespace ClangBackEnd

