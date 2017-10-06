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
#include "refactoringclientinterface.h"
#include "readmessageblock.h"
#include "writemessageblock.h"

#include <memory>

namespace ClangBackEnd {

class RefactoringServerInterface;

class CLANGSUPPORT_EXPORT RefactoringClientProxy : public RefactoringClientInterface
{
public:
    explicit RefactoringClientProxy(RefactoringServerInterface *server, QIODevice *ioDevice);
    RefactoringClientProxy(const RefactoringClientProxy&) = delete;
    const RefactoringClientProxy &operator=(const RefactoringClientProxy&) = delete;

    RefactoringClientProxy(RefactoringClientProxy&&other);
    RefactoringClientProxy &operator=(RefactoringClientProxy&&other);

    void readMessages();

    void alive() override;
    void sourceLocationsForRenamingMessage(SourceLocationsForRenamingMessage &&message) override;
    void sourceRangesAndDiagnosticsForQueryMessage(SourceRangesAndDiagnosticsForQueryMessage &&message) override;
    void sourceRangesForQueryMessage(SourceRangesForQueryMessage &&message) override;

    void setLocalRenamingCallback(RenameCallback &&) final {}

private:
    ClangBackEnd::WriteMessageBlock writeMessageBlock;
    ClangBackEnd::ReadMessageBlock readMessageBlock;
    RefactoringServerInterface *server = nullptr;
    QIODevice *ioDevice = nullptr;
};

} // namespace ClangBackEnd
