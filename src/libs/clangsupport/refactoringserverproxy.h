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
#include "refactoringserverinterface.h"
#include "readmessageblock.h"
#include "writemessageblock.h"

#include <QtGlobal>

#include <memory>

QT_BEGIN_NAMESPACE
class QIODevice;
QT_END_NAMESPACE

namespace ClangBackEnd {

class RefactoringClientInterface;

class CLANGSUPPORT_EXPORT RefactoringServerProxy final : public RefactoringServerInterface
{
public:
    explicit RefactoringServerProxy(RefactoringClientInterface *client, QIODevice *ioDevice);
    RefactoringServerProxy(const RefactoringServerProxy&) = delete;
    const RefactoringServerProxy &operator=(const RefactoringServerProxy&) = delete;

    void end() override;
    void requestSourceLocationsForRenamingMessage(RequestSourceLocationsForRenamingMessage &&message) override;
    void requestSourceRangesAndDiagnosticsForQueryMessage(RequestSourceRangesAndDiagnosticsForQueryMessage &&message) override;
    void requestSourceRangesForQueryMessage(RequestSourceRangesForQueryMessage &&message) override;
    void updatePchProjectParts(UpdatePchProjectPartsMessage &&message) override;
    void removePchProjectParts(RemovePchProjectPartsMessage &&message) override;
    void cancel() override;

    void readMessages();

    void resetCounter();

private:
    ClangBackEnd::WriteMessageBlock writeMessageBlock;
    ClangBackEnd::ReadMessageBlock readMessageBlock;
    RefactoringClientInterface *client = nullptr;
};

} // namespace ClangBackEnd
