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

#ifndef CLANGBACKEND_IPCCLIENTDISPATCHER_H
#define CLANGBACKEND_IPCCLIENTDISPATCHER_H

#include "ipcclientinterface.h"

#include <QVector>

namespace ClangBackEnd {

class CMBIPC_EXPORT IpcClientDispatcher : public ClangBackEnd::IpcClientInterface
{
public:
    void addClient(IpcClientInterface *client);
    void removeClient(IpcClientInterface *client);

    void alive() override;
    void echo(const EchoMessage &message) override;
    void codeCompleted(const CodeCompletedMessage &message) override;
    void translationUnitDoesNotExist(const TranslationUnitDoesNotExistMessage &message) override;
    void projectPartsDoNotExist(const ProjectPartsDoNotExistMessage &message) override;
    void diagnosticsChanged(const DiagnosticsChangedMessage &message) override;
    void highlightingChanged(const HighlightingChangedMessage &message) override;

private:
    QVector<IpcClientInterface*> clients;
};

} // namespace ClangBackEnd

#endif // CLANGBACKEND_IPCCLIENTDISPATCHER_H
