/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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

#include <symbolscollectorinterface.h>

namespace Sqlite {
class Database;
}

class MockSymbolsCollector : public ClangBackEnd::SymbolsCollectorInterface
{
public:
    MockSymbolsCollector()
    {
    }

    MockSymbolsCollector(const Sqlite::Database &)
    {
        ON_CALL(*this, setIsUsed(_)).WillByDefault(Invoke(this, &MockSymbolsCollector::setIsUsed2));
        ON_CALL(*this, isUsed()).WillByDefault(Invoke(this, &MockSymbolsCollector::isUsed2));
        ON_CALL(*this, setUnsavedFiles(_)).WillByDefault(Invoke(this, &MockSymbolsCollector::setHasUnsavedFiles));
    }

    MOCK_METHOD0(collectSymbols, bool());

    MOCK_METHOD2(setFile,
                 void(ClangBackEnd::FilePathId filePathId,
                      const Utils::SmallStringVector &arguments));

    MOCK_METHOD1(setUnsavedFiles,
                 void(const ClangBackEnd::V2::FileContainers &unsavedFiles));

    MOCK_METHOD0(clear,
                 void());

    MOCK_METHOD0(doInMainThreadAfterFinished,
                 void());

    MOCK_CONST_METHOD0(symbols,
                       const ClangBackEnd::SymbolEntries &());

    MOCK_CONST_METHOD0(sourceLocations,
                       const ClangBackEnd::SourceLocationEntries &());

    MOCK_CONST_METHOD0(sourceFiles,
                       const ClangBackEnd::FilePathIds &());

    MOCK_CONST_METHOD0(usedMacros,
                       const ClangBackEnd::UsedMacros &());

    MOCK_CONST_METHOD0(sourceDependencies,
                       const ClangBackEnd::SourceDependencies &());

    MOCK_CONST_METHOD0(isUsed,
                       bool());

    MOCK_METHOD1(setIsUsed,
                 void(bool));

    void setIsUsed2(bool isUsed)
    {
        used = isUsed;
    }

    bool isUsed2() const
    {
        return used;
    }

    void setHasUnsavedFiles(const ClangBackEnd::V2::FileContainers &unsavedFiles)
    {
        hasUnsavedFiles = true;
    }

public:
    bool used = false;
    bool hasUnsavedFiles = false;
};
