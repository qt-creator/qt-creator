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

#include "googletest.h"

#include <refactoringserverinterface.h>

class MockRefactoringServer : public ClangBackEnd::RefactoringServerInterface
{
public:
    MOCK_METHOD0(end,
                 void());

    MOCK_METHOD1(requestSourceLocationsForRenamingMessage,
                 void (const ClangBackEnd::RequestSourceLocationsForRenamingMessage&));

    MOCK_METHOD1(requestSourceRangesAndDiagnosticsForQueryMessage,
                 void (const ClangBackEnd::RequestSourceRangesAndDiagnosticsForQueryMessage&));

    MOCK_METHOD1(requestSourceRangesForQueryMessage,
                 void (const ClangBackEnd::RequestSourceRangesForQueryMessage&));

    MOCK_METHOD1(updatePchProjectParts,
                 void (const ClangBackEnd::UpdatePchProjectPartsMessage&));

    MOCK_METHOD1(removePchProjectParts,
                 void (const ClangBackEnd::RemovePchProjectPartsMessage&));

    MOCK_METHOD0(cancel,
                 void());

    void requestSourceLocationsForRenamingMessage(ClangBackEnd::RequestSourceLocationsForRenamingMessage &&message) override
    {
        requestSourceLocationsForRenamingMessage(message);
    }

    void requestSourceRangesAndDiagnosticsForQueryMessage(ClangBackEnd::RequestSourceRangesAndDiagnosticsForQueryMessage &&message) override
    {
        requestSourceRangesAndDiagnosticsForQueryMessage(message);
    }

    void requestSourceRangesForQueryMessage(ClangBackEnd::RequestSourceRangesForQueryMessage &&message) override
    {
        requestSourceRangesForQueryMessage(message);
    }

    void updatePchProjectParts(ClangBackEnd::UpdatePchProjectPartsMessage &&message) override
    {
        updatePchProjectParts(message);
    }

    void removePchProjectParts(ClangBackEnd::RemovePchProjectPartsMessage &&message) override
    {
        removePchProjectParts(message);
    }
};
