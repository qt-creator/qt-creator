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

#include <clangpchmanager_global.h>

#include <filecontainerv2.h>

namespace CppTools {
class ProjectPart;
class ProjectFile;
}

namespace ClangBackEnd {
class PchManagerServerInterface;

namespace V2 {
class ProjectPartContainer;
}
}

QT_FORWARD_DECLARE_CLASS(QStringList)

#include <vector>

namespace ClangPchManager {

class HeaderAndSources;
class PchManagerClient;

class ProjectUpdater
{
public:
    ProjectUpdater(ClangBackEnd::PchManagerServerInterface &server,
                   PchManagerClient &client);

    void updateProjectParts(const std::vector<CppTools::ProjectPart *> &projectParts,
                            ClangBackEnd::V2::FileContainers &&generatedFiles);
    void removeProjectParts(const QStringList &projectPartIds);

unittest_public:
    void setExcludedPaths(Utils::PathStringVector &&excludedPaths);

    HeaderAndSources headerAndSourcesFromProjectPart(CppTools::ProjectPart *projectPart) const;
    ClangBackEnd::V2::ProjectPartContainer toProjectPartContainer(
            CppTools::ProjectPart *projectPart) const;
    std::vector<ClangBackEnd::V2::ProjectPartContainer> toProjectPartContainers(
            std::vector<CppTools::ProjectPart *> projectParts) const;
    void addToHeaderAndSources(HeaderAndSources &headerAndSources,
                               const CppTools::ProjectFile &projectFile) const;
    static QStringList compilerArguments(CppTools::ProjectPart *projectPart);
    static Utils::PathStringVector createExcludedPaths(
            const ClangBackEnd::V2::FileContainers &generatedFiles);

private:
    Utils::PathStringVector m_excludedPaths;
    ClangBackEnd::PchManagerServerInterface &m_server;
    PchManagerClient &m_client;
};

} // namespace ClangPchManager
