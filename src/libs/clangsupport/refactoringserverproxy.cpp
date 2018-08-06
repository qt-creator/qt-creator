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

#include "refactoringserverproxy.h"

#include "messageenvelop.h"
#include "refactoringclientinterface.h"
#include "clangrefactoringservermessages.h"

#include <QIODevice>
#include <QVector>

namespace ClangBackEnd {

RefactoringServerProxy::RefactoringServerProxy(RefactoringClientInterface *client, QIODevice *ioDevice)
    : BaseServerProxy(client, ioDevice)
{
}

void RefactoringServerProxy::end()
{
    m_writeMessageBlock.write(EndMessage());
}

void RefactoringServerProxy::requestSourceLocationsForRenamingMessage(RequestSourceLocationsForRenamingMessage &&message)
{
    m_writeMessageBlock.write(message);
}

void RefactoringServerProxy::requestSourceRangesAndDiagnosticsForQueryMessage(RequestSourceRangesAndDiagnosticsForQueryMessage &&message)
{
    m_writeMessageBlock.write(message);
}

void RefactoringServerProxy::requestSourceRangesForQueryMessage(RequestSourceRangesForQueryMessage &&message)
{
    m_writeMessageBlock.write(message);
}

void RefactoringServerProxy::updateProjectParts(UpdateProjectPartsMessage &&message)
{
    m_writeMessageBlock.write(message);
}

void RefactoringServerProxy::removeProjectParts(RemoveProjectPartsMessage &&message)
{
    m_writeMessageBlock.write(message);
}

void RefactoringServerProxy::updateGeneratedFiles(UpdateGeneratedFilesMessage &&message)
{
    m_writeMessageBlock.write(message);
}

void RefactoringServerProxy::removeGeneratedFiles(RemoveGeneratedFilesMessage &&message)
{
    m_writeMessageBlock.write(message);
}

void RefactoringServerProxy::cancel()
{
    m_writeMessageBlock.write(CancelMessage());
}

} // namespace ClangBackEnd
