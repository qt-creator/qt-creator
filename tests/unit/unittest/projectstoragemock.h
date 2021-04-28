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

#include "sqlitedatabasemock.h"

#include <metainfo/projectstoragetypes.h>
#include <projectstorageids.h>

class ProjectStorageMock
{
public:
    ProjectStorageMock(SqliteDatabaseMock &databaseMock)
        : databaseMock{databaseMock}
    {}

    MOCK_METHOD1(fetchSourceContextId,
                 QmlDesigner::SourceContextId(Utils::SmallStringView SourceContextPath));
    MOCK_METHOD2(fetchSourceId,
                 QmlDesigner::SourceId(QmlDesigner::SourceContextId SourceContextId,
                                       Utils::SmallStringView sourceName));
    MOCK_METHOD1(fetchSourceContextIdUnguarded,
                 QmlDesigner::SourceContextId(Utils::SmallStringView SourceContextPath));
    MOCK_METHOD2(fetchSourceIdUnguarded,
                 QmlDesigner::SourceId(QmlDesigner::SourceContextId SourceContextId,
                                       Utils::SmallStringView sourceName));
    MOCK_METHOD1(fetchSourceContextPath,
                 Utils::PathString(QmlDesigner::SourceContextId sourceContextId));
    MOCK_METHOD1(fetchSourceNameAndSourceContextId,
                 QmlDesigner::Sources::SourceNameAndSourceContextId(QmlDesigner::SourceId sourceId));
    MOCK_METHOD0(fetchAllSourceContexts, std::vector<QmlDesigner::Sources::SourceContext>());
    MOCK_METHOD0(fetchAllSources, std::vector<ClangBackEnd::Sources::Source>());

    SqliteDatabaseMock &database() { return databaseMock; }

    SqliteDatabaseMock &databaseMock;
};

