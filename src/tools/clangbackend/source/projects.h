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

#include "projectpart.h"

#include <projectpartcontainer.h>

#include <vector>

namespace ClangBackEnd {

class ProjectParts
{
public:
    void createOrUpdate(const QVector<ProjectPartContainer> &projectConainers);
    void remove(const Utf8StringVector &projectPartIds);

    bool hasProjectPart(const Utf8String &projectPartId) const;

    const ProjectPart &project(const Utf8String &projectPartId) const;

    std::vector<ProjectPart>::const_iterator findProjectPart(const Utf8String &projectPartId) const;
    std::vector<ProjectPart>::iterator findProjectPart(const Utf8String &projectPartId);

    const std::vector<ProjectPart> &projects() const;

private:
    void createOrUpdateProjectPart(const ProjectPartContainer &projectConainer);

private:
    std::vector<ProjectPart> projects_;
};

} // namespace ClangBackEnd
