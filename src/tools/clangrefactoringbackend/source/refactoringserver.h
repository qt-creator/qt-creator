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

#include "clangquerygatherer.h"

#include <refactoringserverinterface.h>

#include <ipcclientprovider.h>
#include <filepathcachinginterface.h>

#include <utils/smallstring.h>

#include <QTimer>

#include <future>
#include <mutex>
#include <vector>

namespace ClangBackEnd {

class SourceRangesForQueryMessage;
class SymbolIndexingInterface;

namespace V2 {
class FileContainer;
}

class RefactoringServer : public RefactoringServerInterface,
                          public IpcClientProvider<RefactoringClientInterface>
{
    using Future = std::future<SourceRangesForQueryMessage>;
public:
    RefactoringServer(SymbolIndexingInterface &symbolIndexing,
                      FilePathCachingInterface &filePathCache);

    void end() override;
    void requestSourceLocationsForRenamingMessage(RequestSourceLocationsForRenamingMessage &&message) override;
    void requestSourceRangesAndDiagnosticsForQueryMessage(RequestSourceRangesAndDiagnosticsForQueryMessage &&message) override;
    void requestSourceRangesForQueryMessage(RequestSourceRangesForQueryMessage &&message) override;
    void updatePchProjectParts(UpdatePchProjectPartsMessage &&message) override;
    void removePchProjectParts(RemovePchProjectPartsMessage &&message) override;
    void cancel() override;

    bool isCancelingJobs() const;

    void pollSourceRangesForQueryMessages();
    void waitThatSourceRangesForQueryMessagesAreFinished();

    bool pollTimerIsActive() const;

    void setGathererProcessingSlotCount(uint count);

private:
    void gatherSourceRangesForQueryMessages(std::vector<V2::FileContainer> &&sources,
                                                          std::vector<V2::FileContainer> &&unsaved,
                                                          Utils::SmallString &&query);

private:
    ClangQueryGatherer m_gatherer;
    QTimer m_pollTimer;
    SymbolIndexingInterface &m_symbolIndexing;
    FilePathCachingInterface &m_filePathCache;
};

} // namespace ClangBackEnd
