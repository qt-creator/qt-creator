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

#include "projectpart.h"

#include <projectpartcontainer.h>

#include <utf8stringvector.h>

#include <cstring>

namespace ClangBackEnd {

class ProjectPartData {
public:
    ProjectPartData(const Utf8String &projectPartId);
    ~ProjectPartData();

public:
    TimePoint lastChangeTimePoint;
    Utf8StringVector arguments;
    Utf8String projectPartId;
};

ProjectPartData::ProjectPartData(const Utf8String &projectPartId)
    : lastChangeTimePoint(Clock::now()),
      projectPartId(projectPartId)
{
}

ProjectPartData::~ProjectPartData()
{
}

ProjectPart::ProjectPart(const Utf8String &projectPartId)
    : d(std::make_shared<ProjectPartData>(projectPartId))
{
}

ProjectPart::ProjectPart(const Utf8String &projectPartId,
                         const Utf8StringVector &arguments)
    : d(std::make_shared<ProjectPartData>(projectPartId))
{
    setArguments(arguments);
}

ProjectPart::ProjectPart(const ProjectPartContainer &projectContainer)
    : d(std::make_shared<ProjectPartData>(projectContainer.projectPartId()))
{
    setArguments(projectContainer.arguments());
}

ProjectPart::~ProjectPart() = default;
ProjectPart::ProjectPart(const ProjectPart &) = default;
ProjectPart &ProjectPart::operator=(const ProjectPart &) = default;

ProjectPart::ProjectPart(ProjectPart &&other)
    : d(std::move(other.d))
{
}

ProjectPart &ProjectPart::operator=(ProjectPart &&other)
{
    d = std::move(other.d);

    return *this;
}

void ProjectPart::clear()
{
    d->projectPartId.clear();
    d->arguments.clear();
    updateLastChangeTimePoint();
}

Utf8String ProjectPart::id() const
{
    return d->projectPartId;
}

void ProjectPart::setArguments(const Utf8StringVector &arguments)
{
    d->arguments = arguments;
    updateLastChangeTimePoint();
}

const Utf8StringVector ProjectPart::arguments() const
{
    return d->arguments;
}

const TimePoint &ProjectPart::lastChangeTimePoint() const
{
    return d->lastChangeTimePoint;
}

void ProjectPart::updateLastChangeTimePoint()
{
    d->lastChangeTimePoint = Clock::now();
}

bool operator==(const ProjectPart &first, const ProjectPart &second)
{
    return first.id() == second.id();
}

} // namespace ClangBackEnd

