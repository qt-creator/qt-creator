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

#ifndef CLANGBACKEND_PROJECTS_H
#define CLANGBACKEND_PROJECTS_H

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

#endif // CLANGBACKEND_PROJECTS_H
