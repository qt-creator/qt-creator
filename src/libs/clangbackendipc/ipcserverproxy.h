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

#ifndef CLANGBACKEND_IPCSERVERPROXY_H
#define CLANGBACKEND_IPCSERVERPROXY_H
#include "ipcserverinterface.h"
#include "readcommandblock.h"
#include "writecommandblock.h"

#include <QtGlobal>
#include <QTimer>

#include <memory>

QT_BEGIN_NAMESPACE
class QVariant;
class QProcess;
class QLocalServer;
class QLocalSocket;
QT_END_NAMESPACE

namespace ClangBackEnd {

class CMBIPC_EXPORT IpcServerProxy : public IpcServerInterface
{
public:
    IpcServerProxy(IpcClientInterface *client, QIODevice *ioDevice);
    IpcServerProxy(const IpcServerProxy&) = delete;
    IpcServerProxy &operator=(const IpcServerProxy&) = delete;

    void end() override;
    void registerTranslationUnitsForCodeCompletion(const RegisterTranslationUnitForCodeCompletionCommand &command) override;
    void unregisterTranslationUnitsForCodeCompletion(const UnregisterTranslationUnitsForCodeCompletionCommand &command) override;
    void registerProjectPartsForCodeCompletion(const RegisterProjectPartsForCodeCompletionCommand &command) override;
    void unregisterProjectPartsForCodeCompletion(const UnregisterProjectPartsForCodeCompletionCommand &command) override;
    void completeCode(const CompleteCodeCommand &command) override;

    void readCommands();

    void resetCounter();

private:
    ClangBackEnd::WriteCommandBlock writeCommandBlock;
    ClangBackEnd::ReadCommandBlock readCommandBlock;
    IpcClientInterface *client;
};

} // namespace ClangBackEnd

#endif // CLANGBACKEND_IPCSERVERPROXY_H
