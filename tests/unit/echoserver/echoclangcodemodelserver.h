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

#pragma once

#include <clangsupport/clangcodemodelserverinterface.h>
#include <ipcclientprovider.h>

namespace ClangBackEnd {

class EchoClangCodeModelServer : public ClangCodeModelServerInterface,
                                 public IpcClientProvider<ClangCodeModelClientInterface>
{
public:
    void dispatch(const MessageEnvelop &message) override;
    void end() override;

    void documentsOpened(const DocumentsOpenedMessage &message) override;
    void documentsChanged(const DocumentsChangedMessage &message) override;
    void documentsClosed(const DocumentsClosedMessage &message) override;
    void documentVisibilityChanged(const DocumentVisibilityChangedMessage &message) override;

    void unsavedFilesUpdated(const UnsavedFilesUpdatedMessage &message) override;
    void unsavedFilesRemoved(const UnsavedFilesRemovedMessage &message) override;

    void requestCompletions(const RequestCompletionsMessage &message) override;
    void requestAnnotations(const RequestAnnotationsMessage &message) override;
    void requestReferences(const RequestReferencesMessage &message) override;
    void requestFollowSymbol(const RequestFollowSymbolMessage &message) override;
    void requestToolTip(const RequestToolTipMessage &message) override;

private:
    void echoMessage(const MessageEnvelop &message);
};

} // namespace ClangBackEnd
