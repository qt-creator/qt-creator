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

#include <projectstorage/filestatus.h>
#include <projectstorage/projectstorageinterface.h>
#include <projectstorage/sourcepathcache.h>

class ProjectStorageMock : public QmlDesigner::ProjectStorageInterface
{
public:
    MOCK_METHOD(void,
                synchronize,
                (QmlDesigner::Storage::ModuleDependencies moduleDependencies,
                 QmlDesigner::Storage::Documents documents,
                 QmlDesigner::Storage::Types types,
                 QmlDesigner::SourceIds sourceIds,
                 QmlDesigner::FileStatuses fileStatuses),
                (override));

    MOCK_METHOD(QmlDesigner::FileStatus,
                fetchFileStatus,
                (QmlDesigner::SourceId sourceId),
                (const, override));

    MOCK_METHOD(QmlDesigner::SourceIds,
                fetchSourceDependencieIds,
                (QmlDesigner::SourceId sourceId),
                (const, override));

    MOCK_METHOD(QmlDesigner::SourceContextId,
                fetchSourceContextId,
                (Utils::SmallStringView SourceContextPath),
                ());
    MOCK_METHOD(QmlDesigner::SourceId,
                fetchSourceId,
                (QmlDesigner::SourceContextId SourceContextId, Utils::SmallStringView sourceName),
                ());
    MOCK_METHOD(QmlDesigner::SourceContextId,
                fetchSourceContextIdUnguarded,
                (Utils::SmallStringView SourceContextPath),
                ());
    MOCK_METHOD(QmlDesigner::SourceId,
                fetchSourceIdUnguarded,
                (QmlDesigner::SourceContextId SourceContextId, Utils::SmallStringView sourceName),
                ());
    MOCK_METHOD(Utils::PathString,
                fetchSourceContextPath,
                (QmlDesigner::SourceContextId sourceContextId));
    MOCK_METHOD(QmlDesigner::Cache::SourceNameAndSourceContextId,
                fetchSourceNameAndSourceContextId,
                (QmlDesigner::SourceId sourceId));
    MOCK_METHOD(std::vector<QmlDesigner::Cache::SourceContext>, fetchAllSourceContexts, (), ());
    MOCK_METHOD(std::vector<QmlDesigner::Cache::Source>, fetchAllSources, (), ());
};

