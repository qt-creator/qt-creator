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

#include "clangsupport_global.h"
#include "clangcodemodelclientinterface.h"
#include "readmessageblock.h"
#include "writemessageblock.h"

#include <QtGlobal>

#include <memory>

QT_BEGIN_NAMESPACE
class QLocalServer;
class QIODevice;
QT_END_NAMESPACE

namespace ClangBackEnd {

class CLANGSUPPORT_EXPORT ClangCodeModelClientProxy : public ClangCodeModelClientInterface
{
public:
    explicit ClangCodeModelClientProxy(ClangCodeModelServerInterface *server, QIODevice *ioDevice);
    ClangCodeModelClientProxy(const ClangCodeModelClientProxy&) = delete;
    const ClangCodeModelClientProxy &operator=(const ClangCodeModelClientProxy&) = delete;

    ClangCodeModelClientProxy(ClangCodeModelClientProxy&&other);
    ClangCodeModelClientProxy &operator=(ClangCodeModelClientProxy&&other);

    void alive() override;
    void echo(const EchoMessage &message) override;
    void codeCompleted(const CodeCompletedMessage &message) override;
    void documentAnnotationsChanged(const DocumentAnnotationsChangedMessage &message) override;
    void references(const ReferencesMessage &message) override;
    void followSymbol(const FollowSymbolMessage &message) override;

    void readMessages();

    bool isUsingThatIoDevice(QIODevice *ioDevice) const;

private:
    ClangBackEnd::WriteMessageBlock m_writeMessageBlock;
    ClangBackEnd::ReadMessageBlock m_readMessageBlock;
    ClangCodeModelServerInterface *m_server = nullptr;
    QIODevice *m_ioDevice = nullptr;
};

} // namespace ClangBackEnd
