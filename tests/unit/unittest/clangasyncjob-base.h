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

#include "dummyclangipcclient.h"
#include "mockclangcodemodelclient.h"

#include <clangdocument.h>
#include <clangiasyncjob.h>
#include <clangjobrequest.h>
#include <projects.h>
#include <clangdocuments.h>
#include <unsavedfiles.h>

class ClangAsyncJobTest : public ::testing::Test
{
protected:
    void BaseSetUp(ClangBackEnd::JobRequest::Type jobRequestType,
                   ClangBackEnd::IAsyncJob &asyncJob);

    ClangBackEnd::JobRequest createJobRequest(const Utf8String &filePath,
                                              ClangBackEnd::JobRequest::Type type) const;

    bool waitUntilJobFinished(const ClangBackEnd::IAsyncJob &asyncJob,
                              int timeOutInMs = 10000) const;

protected:
    ClangBackEnd::ProjectParts projects;
    ClangBackEnd::UnsavedFiles unsavedFiles;
    ClangBackEnd::Documents documents{projects, unsavedFiles};
    ClangBackEnd::Document document;

    MockClangCodeModelClient mockIpcClient;
    DummyIpcClient dummyIpcClient;

    Utf8String filePath{Utf8StringLiteral(TESTDATA_DIR"/translationunits.cpp")};
    Utf8String projectPartId{Utf8StringLiteral("/path/to/projectfile")};

    ClangBackEnd::JobRequest jobRequest;
    ClangBackEnd::JobContext jobContext;
    ClangBackEnd::JobContext jobContextWithMockClient;
};
