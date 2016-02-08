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

#ifndef CLANGBACKEND_IPCSERVERINTERFACE_H
#define CLANGBACKEND_IPCSERVERINTERFACE_H

#include "ipcinterface.h"

#include "ipcclientdispatcher.h"

namespace ClangBackEnd {

class IpcClientInterface;

class CMBIPC_EXPORT IpcServerInterface : public IpcInterface
{
public:
    void dispatch(const MessageEnvelop &messageEnvelop) override;

    virtual void end() = 0;
    virtual void registerTranslationUnitsForEditor(const RegisterTranslationUnitForEditorMessage &message) = 0;
    virtual void updateTranslationUnitsForEditor(const UpdateTranslationUnitsForEditorMessage &message) = 0;
    virtual void unregisterTranslationUnitsForEditor(const UnregisterTranslationUnitsForEditorMessage &message) = 0;
    virtual void registerProjectPartsForEditor(const RegisterProjectPartsForEditorMessage &message) = 0;
    virtual void unregisterProjectPartsForEditor(const UnregisterProjectPartsForEditorMessage &message) = 0;
    virtual void registerUnsavedFilesForEditor(const RegisterUnsavedFilesForEditorMessage &message) = 0;
    virtual void unregisterUnsavedFilesForEditor(const UnregisterUnsavedFilesForEditorMessage &message) = 0;
    virtual void completeCode(const CompleteCodeMessage &message) = 0;
    virtual void requestDiagnostics(const RequestDiagnosticsMessage &message) = 0;
    virtual void requestHighlighting(const RequestHighlightingMessage &message) = 0;
    virtual void updateVisibleTranslationUnits(const UpdateVisibleTranslationUnitsMessage &message) = 0;

    void addClient(IpcClientInterface *client);
    void removeClient(IpcClientInterface *client);

    IpcClientInterface *client();

private:
    IpcClientDispatcher clientDispatcher;
};

} // namespace ClangBackEnd

#endif // CLANGBACKEND_IPCSERVERINTERFACE_H
