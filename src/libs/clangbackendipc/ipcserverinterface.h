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

#ifndef CLANGBACKEND_IPCSERVERINTERFACE_H
#define CLANGBACKEND_IPCSERVERINTERFACE_H

#include "ipcinterface.h"

#include "ipcclientdispatcher.h"

namespace ClangBackEnd {

class IpcClientInterface;

class CMBIPC_EXPORT IpcServerInterface : public IpcInterface
{
public:
    void dispatch(const QVariant &command) override;

    virtual void end() = 0;
    virtual void registerTranslationUnitsForCodeCompletion(const RegisterTranslationUnitForCodeCompletionCommand &command) = 0;
    virtual void unregisterTranslationUnitsForCodeCompletion(const UnregisterTranslationUnitsForCodeCompletionCommand &command) = 0;
    virtual void registerProjectPartsForCodeCompletion(const RegisterProjectPartsForCodeCompletionCommand &command) = 0;
    virtual void unregisterProjectPartsForCodeCompletion(const UnregisterProjectPartsForCodeCompletionCommand &command) = 0;
    virtual void completeCode(const CompleteCodeCommand &command) = 0;

    void addClient(IpcClientInterface *client);
    void removeClient(IpcClientInterface *client);

    IpcClientInterface *client();

private:
    IpcClientDispatcher clientDispatcher;
};

} // namespace ClangBackEnd

#endif // CLANGBACKEND_IPCSERVERINTERFACE_H
