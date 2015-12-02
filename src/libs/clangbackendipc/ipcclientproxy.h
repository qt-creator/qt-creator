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

#ifndef CODELMODELBACKEND_IPCCLIENTPROXY_H
#define CODELMODELBACKEND_IPCCLIENTPROXY_H

#include "clangbackendipc_global.h"
#include "ipcclientinterface.h"
#include "readmessageblock.h"
#include "writemessageblock.h"

#include <QtGlobal>

#include <memory>

QT_BEGIN_NAMESPACE
class QLocalServer;
class QIODevice;
QT_END_NAMESPACE

namespace ClangBackEnd {

class CMBIPC_EXPORT IpcClientProxy : public IpcClientInterface
{
public:
    explicit IpcClientProxy(IpcServerInterface *server, QIODevice *ioDevice);
    IpcClientProxy(const IpcClientProxy&) = delete;
    const IpcClientProxy &operator=(const IpcClientProxy&) = delete;

    IpcClientProxy(IpcClientProxy&&other);
    IpcClientProxy &operator=(IpcClientProxy&&other);

    void alive() override;
    void echo(const EchoMessage &message) override;
    void codeCompleted(const CodeCompletedMessage &message) override;
    void translationUnitDoesNotExist(const TranslationUnitDoesNotExistMessage &message) override;
    void projectPartsDoNotExist(const ProjectPartsDoNotExistMessage &message) override;
    void diagnosticsChanged(const DiagnosticsChangedMessage &message) override;
    void highlightingChanged(const HighlightingChangedMessage &message) override;

    void readMessages();

    bool isUsingThatIoDevice(QIODevice *ioDevice) const;

private:
    ClangBackEnd::WriteMessageBlock writeMessageBlock;
    ClangBackEnd::ReadMessageBlock readMessageBlock;
    IpcServerInterface *server = nullptr;
    QIODevice *ioDevice = nullptr;
};

} // namespace ClangBackEnd

#endif // CODELMODELBACKEND_IPCCLIENTPROXY_H
