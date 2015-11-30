/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef CLANGBACKEND_PROJECT_H
#define CLANGBACKEND_PROJECT_H

#include <chrono>
#include <memory>
#include <vector>

class Utf8String;
class Utf8StringVector;

namespace ClangBackEnd {

class ProjectPartContainer;
class ProjectPartData;

using time_point = std::chrono::steady_clock::time_point;

class ProjectPart
{
public:
    ProjectPart(const Utf8String &projectPartId);
    ProjectPart(const Utf8String &projectPartId,
                std::initializer_list<Utf8String> arguments);
    ProjectPart(const ProjectPartContainer &projectContainer);
    ~ProjectPart();

    ProjectPart(const ProjectPart &project);
    ProjectPart &operator=(const ProjectPart &project);

    ProjectPart(ProjectPart &&project);
    ProjectPart &operator=(ProjectPart &&project);

    void clear();

    const Utf8String &projectPartId() const;

    void setArguments(const Utf8StringVector &arguments_);

    const std::vector<const char*> &arguments() const;

    int argumentCount() const;
    const char *const *cxArguments() const;

    const time_point &lastChangeTimePoint() const;

private:
    void updateLastChangeTimePoint();

private:
    std::shared_ptr<ProjectPartData> d;
};

bool operator==(const ProjectPart &first, const ProjectPart &second);

} // namespace ClangBackEnd

#endif // CLANGBACKEND_PROJECT_H
