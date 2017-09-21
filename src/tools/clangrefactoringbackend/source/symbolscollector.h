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

#include "clangtool.h"
#include "collectmacrossourcefilecallbacks.h"
#include "collectsymbolsaction.h"
#include "symbolscollectorinterface.h"
#include "symbolentry.h"

#include <filepathcachingfwd.h>

namespace ClangBackEnd {

class SymbolsCollector : public ClangTool, public SymbolsCollectorInterface
{
public:
    SymbolsCollector(FilePathCachingInterface &filePathCache);

    void addFiles(const Utils::PathStringVector &filePaths,
                  const Utils::SmallStringVector &arguments) override;

    void addUnsavedFiles(const V2::FileContainers &unsavedFiles) override;

    void collectSymbols() override;

    const SymbolEntries &symbols() const override;
    const SourceLocationEntries &sourceLocations() const override;

private:
    CollectSymbolsAction m_collectSymbolsAction;
    CollectMacrosSourceFileCallbacks m_collectMacrosSourceFileCallbacks;
};

} // namespace ClangBackEnd
