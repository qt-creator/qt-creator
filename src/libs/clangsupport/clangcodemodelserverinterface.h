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

#include "ipcserverinterface.h"

#include "clangcodemodelclientinterface.h"

namespace ClangBackEnd {

class ClangCodeModelClientInterface;

class CLANGSUPPORT_EXPORT ClangCodeModelServerInterface : public IpcServerInterface
{
public:
    ~ClangCodeModelServerInterface() override = default;

    void dispatch(const MessageEnvelop &messageEnvelop) override;

    virtual void end() = 0;

    virtual void documentsOpened(const DocumentsOpenedMessage &message) = 0;
    virtual void documentsChanged(const DocumentsChangedMessage &message) = 0;
    virtual void documentsClosed(const DocumentsClosedMessage &message) = 0;
    virtual void documentVisibilityChanged(const DocumentVisibilityChangedMessage &message) = 0;

    virtual void unsavedFilesUpdated(const UnsavedFilesUpdatedMessage &message) = 0;
    virtual void unsavedFilesRemoved(const UnsavedFilesRemovedMessage &message) = 0;

    virtual void requestCompletions(const RequestCompletionsMessage &message) = 0;
    virtual void requestAnnotations(const RequestAnnotationsMessage &message) = 0;
    virtual void requestReferences(const RequestReferencesMessage &message) = 0;
    virtual void requestFollowSymbol(const RequestFollowSymbolMessage &message) = 0;
    virtual void requestToolTip(const RequestToolTipMessage &message) = 0;
};

} // namespace ClangBackEnd
