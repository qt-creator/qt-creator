/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include <filepathid.h>
#include <projectpartid.h>

#include <functional>

namespace Sqlite {
class TransactionInterface;
}

namespace ClangBackEnd {

class SymbolsCollectorInterface;
class SymbolStorageInterface;

class SymbolIndexerTask
{
public:
    using Callable = std::function<void(SymbolsCollectorInterface &symbolsCollector)>;

    SymbolIndexerTask(FilePathId filePathId, ProjectPartId projectPartId, Callable &&callable)
        : callable(std::move(callable))
        , filePathId(filePathId)
        , projectPartId(projectPartId)
    {
    }

    SymbolIndexerTask clone() const
    {
        return *this;
    }

    friend
    bool operator==(const SymbolIndexerTask &first, const SymbolIndexerTask &second)
    {
        return first.filePathId == second.filePathId && first.projectPartId == second.projectPartId;
    }

    friend
    bool operator<(const SymbolIndexerTask &first, const SymbolIndexerTask &second)
    {
        return std::tie(first.filePathId, first.projectPartId)
             < std::tie(second.filePathId, second.projectPartId);
    }

    operator Callable&&()
    {
        return std::move(callable);
    }

public:
    Callable callable;
    FilePathId filePathId;
    ProjectPartId projectPartId;
};

} // namespace ClangBackEnd
