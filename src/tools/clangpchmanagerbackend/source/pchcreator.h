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

#include "pchcreatorinterface.h"

#include "stringcache.h"
#include "idpaths.h"

#include <projectpartpch.h>
#include <projectpartcontainerv2.h>

#include <vector>

QT_FORWARD_DECLARE_CLASS(QFile)
QT_FORWARD_DECLARE_CLASS(QCryptographicHash)
QT_FORWARD_DECLARE_CLASS(QProcess)

namespace ClangBackEnd {

class Environment;

class PchCreator final : public PchCreatorInterface
{
public:
    PchCreator(Environment &environment,
               StringCache<Utils::SmallString> &filePathCache);
    PchCreator(V2::ProjectPartContainers &&projectsParts,
               Environment &environment,
               StringCache<Utils::SmallString> &filePathCache);

    void generatePchs(V2::ProjectPartContainers &&projectsParts) override;
    std::vector<ProjectPartPch> takeProjectPartPchs() override;
    std::vector<IdPaths> takeProjectsIncludes() override;

unitttest_public:
    Utils::SmallStringVector generateGlobalHeaderPaths() const;
    Utils::SmallStringVector generateGlobalSourcePaths() const;
    Utils::SmallStringVector generateGlobalHeaderAndSourcePaths() const;
    Utils::SmallStringVector generateGlobalArguments() const;
    Utils::SmallStringVector generateGlobalCommandLine() const;
    Utils::SmallStringVector generateGlobalPchCompilerArguments() const;
    Utils::SmallStringVector generateGlobalClangCompilerArguments() const;

    std::vector<uint> generateGlobalPchIncludeIds() const;

    Utils::SmallString generatePchIncludeFileContent(const std::vector<uint> &includeIds) const;
    Utils::SmallString generateGlobalPchHeaderFileContent() const;
    std::unique_ptr<QFile> generateGlobalPchHeaderFile();
    void generatePch(const Utils::SmallStringVector &commandLineArguments);
    void generateGlobalPch();

    Utils::SmallString globalPchContent() const;

    static QStringList convertToQStringList(const Utils::SmallStringVector &convertToQStringList);

    Utils::SmallString generateGlobalPchFilePathWithoutExtension() const;
    Utils::SmallString generateGlobalPchHeaderFilePath() const;
    Utils::SmallString generateGlobalPchFilePath() const;

    Utils::SmallStringVector generateProjectPartCommandLine(
            const V2::ProjectPartContainer &projectPart) const;
    Utils::SmallString generateProjectPartPchFilePathWithoutExtension(
            const V2::ProjectPartContainer &projectPart) const;
    static Utils::SmallStringVector generateProjectPartHeaderAndSourcePaths(
            const V2::ProjectPartContainer &projectPart);
    std::vector<uint> generateProjectPartPchIncludes(
            const V2::ProjectPartContainer &projectPart) const;
    Utils::SmallString generateProjectPathPchHeaderFilePath(
            const V2::ProjectPartContainer &projectPart) const;
    Utils::SmallString  generateProjectPartPchFilePath(
           const V2::ProjectPartContainer &projectPart) const;
    Utils::SmallStringVector generateProjectPartPchCompilerArguments(
            const V2::ProjectPartContainer &projectPart) const;
    Utils::SmallStringVector generateProjectPartClangCompilerArguments(
             const V2::ProjectPartContainer &projectPart) const;
    std::pair<ProjectPartPch, IdPaths> generateProjectPartPch(
            const V2::ProjectPartContainer &projectPart);
    static std::unique_ptr<QFile> generatePchHeaderFile(
            const Utils::SmallString &filePath,
            const Utils::SmallString &content);

    void generatePchs();

private:
    static QByteArray projectPartHash(const V2::ProjectPartContainer &projectPart);
    QByteArray globalProjectHash() const;
    static void checkIfProcessHasError(const QProcess &process);

private:
    V2::ProjectPartContainers m_projectParts;
    std::vector<ProjectPartPch> m_projectPartPchs;
    std::vector<IdPaths> m_projectsIncludeIds;
    Environment &m_environment;
    StringCache<Utils::SmallString> &m_filePathCache;
};

} // namespace ClangBackEnd
