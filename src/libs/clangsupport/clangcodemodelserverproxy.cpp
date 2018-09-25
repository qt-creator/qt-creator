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

#include <clangcodemodelclientinterface.h>
#include <clangcodemodelservermessages.h>
#include <messageenvelop.h>

namespace ClangBackEnd {

ClangCodeModelServerProxy::ClangCodeModelServerProxy(ClangCodeModelClientInterface *client,
                                                     QIODevice *ioDevice)
    : BaseServerProxy(client, ioDevice)
{
}

void ClangCodeModelServerProxy::end()
{
    m_writeMessageBlock.write(EndMessage());
}

void ClangCodeModelServerProxy::documentsOpened(const DocumentsOpenedMessage &message)
{
    m_writeMessageBlock.write(message);
}

void ClangCodeModelServerProxy::documentsChanged(const DocumentsChangedMessage &message)
{
    m_writeMessageBlock.write(message);
}

void ClangCodeModelServerProxy::documentsClosed(const DocumentsClosedMessage &message)
{
    m_writeMessageBlock.write(message);
}

void ClangCodeModelServerProxy::unsavedFilesUpdated(const UnsavedFilesUpdatedMessage &message)
{
    m_writeMessageBlock.write(message);
}

void ClangCodeModelServerProxy::unsavedFilesRemoved(const UnsavedFilesRemovedMessage &message)
{
    m_writeMessageBlock.write(message);
}

void ClangCodeModelServerProxy::requestCompletions(const RequestCompletionsMessage &message)
{
    m_writeMessageBlock.write(message);
}

void ClangCodeModelServerProxy::requestAnnotations(const RequestAnnotationsMessage &message)
{
    m_writeMessageBlock.write(message);
}

void ClangCodeModelServerProxy::requestReferences(const RequestReferencesMessage &message)
{
    m_writeMessageBlock.write(message);
}

void ClangCodeModelServerProxy::requestFollowSymbol(const RequestFollowSymbolMessage &message)
{
    m_writeMessageBlock.write(message);
}

void ClangCodeModelServerProxy::requestToolTip(const RequestToolTipMessage &message)
{
    m_writeMessageBlock.write(message);
}

void ClangCodeModelServerProxy::documentVisibilityChanged(
    const DocumentVisibilityChangedMessage &message)
{
    m_writeMessageBlock.write(message);
}

} // namespace ClangBackEnd

