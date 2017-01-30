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

#include "clangbackendipc_global.h"
#include "pchmanagerclientinterface.h"
#include "readmessageblock.h"
#include "writemessageblock.h"

namespace ClangBackEnd {

class PchManagerServerInterface;

class CMBIPC_EXPORT PchManagerClientProxy : public PchManagerClientInterface
{
public:
    explicit PchManagerClientProxy(PchManagerServerInterface *server, QIODevice *ioDevice);
    PchManagerClientProxy(const PchManagerClientProxy&) = delete;
    const PchManagerClientProxy &operator=(const PchManagerClientProxy&) = delete;

    PchManagerClientProxy(PchManagerClientProxy&&other);
    PchManagerClientProxy &operator=(PchManagerClientProxy&&other);

    void readMessages();

    void alive() override;
    void precompiledHeadersUpdated(PrecompiledHeadersUpdatedMessage &&message) override;

private:
    ClangBackEnd::WriteMessageBlock writeMessageBlock;
    ClangBackEnd::ReadMessageBlock readMessageBlock;
    PchManagerServerInterface *server = nullptr;
    QIODevice *ioDevice = nullptr;
};

} // namespace ClangBackEnd
