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

#include "projectpartpch.h"

#include <filepath.h>
#include <projectpartid.h>

#include <utils/smallstringvector.h>
#include <utils/optional.h>

namespace ClangBackEnd {

class PrecompiledHeaderStorageInterface
{
public:
    PrecompiledHeaderStorageInterface() = default;

    PrecompiledHeaderStorageInterface(const PrecompiledHeaderStorageInterface &) = delete;
    PrecompiledHeaderStorageInterface &operator=(const PrecompiledHeaderStorageInterface &) = delete;

    virtual void insertProjectPrecompiledHeader(ProjectPartId projectPartId,
                                                Utils::SmallStringView pchPath,
                                                long long pchBuildTime)
        = 0;
    virtual void deleteProjectPrecompiledHeader(ProjectPartId projectPartName) = 0;
    virtual void insertSystemPrecompiledHeaders(const ProjectPartIds &projectPartIds,
                                                Utils::SmallStringView pchPath,
                                                long long pchBuildTime)
        = 0;
    virtual void deleteSystemPrecompiledHeaders(const ProjectPartIds &projectPartIds) = 0;
    virtual FilePath fetchSystemPrecompiledHeaderPath(ProjectPartId projectPartId) = 0;
    virtual Utils::optional<ProjectPartPch> fetchPrecompiledHeader(ProjectPartId projectPartId) const = 0;

protected:
    ~PrecompiledHeaderStorageInterface() = default;
};

} // namespace ClangBackEnd
