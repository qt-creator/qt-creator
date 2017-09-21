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

#include "clangpathwatcherinterface.h"
#include "clangpathwatchernotifier.h"
#include "pchcreatorinterface.h"
#include "pchgeneratornotifierinterface.h"
#include "pchmanagerserverinterface.h"
#include "projectpartsinterface.h"

#include <ipcclientprovider.h>

namespace ClangBackEnd {

class SourceRangesAndDiagnosticsForQueryMessage;

class PchManagerServer : public PchManagerServerInterface,
                         public ClangPathWatcherNotifier,
                         public PchGeneratorNotifierInterface,
                         public IpcClientProvider<PchManagerClientInterface>

{
public:
    PchManagerServer(ClangPathWatcherInterface &fileSystemWatcher,
                     PchCreatorInterface &pchCreator,
                     ProjectPartsInterface &projectParts);


    void end() override;
    void updatePchProjectParts(UpdatePchProjectPartsMessage &&message) override;
    void removePchProjectParts(RemovePchProjectPartsMessage &&message) override;

    void pathsWithIdsChanged(const Utils::SmallStringVector &ids) override;
    void taskFinished(TaskFinishStatus status, const ProjectPartPch &projectPartPch) override;

private:
    ClangPathWatcherInterface &m_fileSystemWatcher;
    PchCreatorInterface &m_pchCreator;
    ProjectPartsInterface &m_projectParts;
};

} // namespace ClangBackEnd
