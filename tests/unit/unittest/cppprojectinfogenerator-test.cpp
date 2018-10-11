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

#include "googletest.h"
#include "gtest-qt-printing.h"
#include "mimedatabase-utilities.h"

#include <cpptools/cppprojectinfogenerator.h>
#include <cpptools/projectinfo.h>

#include <utils/mimetypes/mimedatabase.h>

using CppTools::Internal::ProjectInfoGenerator;
using CppTools::ProjectFile;
using CppTools::ProjectInfo;
using CppTools::ProjectUpdateInfo;
using CppTools::ProjectPart;
using CppTools::RawProjectPart;

using ProjectExplorer::Macros;
using ProjectExplorer::ToolChain;

using testing::Eq;
using testing::UnorderedElementsAre;
using testing::PrintToString;

namespace {

MATCHER_P2(IsProjectPart, languageVersion, fileKind,
           std::string(negation ? "isn't" : "is")
           + " project part with language version " + PrintToString(languageVersion)
           + " and file kind " + PrintToString(fileKind))
{
    const ProjectPart::Ptr &projectPart = arg;

    return projectPart->languageVersion == languageVersion
        && projectPart->files.at(0).kind == fileKind;
}

class ProjectInfoGenerator : public ::testing::Test
{
protected:
    void SetUp() override;
    ProjectInfo generate();

protected:
    ProjectUpdateInfo projectUpdateInfo;
    RawProjectPart rawProjectPart;
};

TEST_F(ProjectInfoGenerator, CreateNoProjectPartsForEmptyFileList)
{
    const ProjectInfo projectInfo = generate();

    ASSERT_TRUE(projectInfo.projectParts().isEmpty());
}

TEST_F(ProjectInfoGenerator, CreateSingleProjectPart)
{
    rawProjectPart.files = QStringList{ "foo.cpp", "foo.h"};

    const ProjectInfo projectInfo = generate();

    ASSERT_THAT(projectInfo.projectParts().size(), Eq(1));
}

TEST_F(ProjectInfoGenerator, CreateMultipleProjectParts)
{
    rawProjectPart.files = QStringList{ "foo.cpp", "foo.h", "bar.c", "bar.h" };

    const ProjectInfo projectInfo = generate();

    ASSERT_THAT(projectInfo.projectParts().size(), Eq(2));
}

TEST_F(ProjectInfoGenerator, ProjectPartIndicatesObjectiveCExtensionsByDefault)
{
    rawProjectPart.files = QStringList{ "foo.mm" };

    const ProjectInfo projectInfo = generate();

    ASSERT_THAT(projectInfo.projectParts().size(), Eq(1));
    const ProjectPart &projectPart = *projectInfo.projectParts().at(0);
    ASSERT_TRUE(projectPart.languageExtensions & ProjectExplorer::LanguageExtension::ObjectiveC);
}

TEST_F(ProjectInfoGenerator, ProjectPartHasLatestLanguageVersionByDefault)
{
    rawProjectPart.files = QStringList{ "foo.cpp" };

    const ProjectInfo projectInfo = generate();

    ASSERT_THAT(projectInfo.projectParts().size(), Eq(1));
    const ProjectPart &projectPart = *projectInfo.projectParts().at(0);
    ASSERT_THAT(projectPart.languageVersion, Eq(ProjectExplorer::LanguageVersion::LatestCxx));
}

TEST_F(ProjectInfoGenerator, UseMacroInspectionReportForLanguageVersion)
{
    projectUpdateInfo.cxxToolChainInfo.macroInspectionRunner = [](const QStringList &) {
        return ToolChain::MacroInspectionReport{Macros(), ProjectExplorer::LanguageVersion::CXX17};
    };
    rawProjectPart.files = QStringList{ "foo.cpp" };

    const ProjectInfo projectInfo = generate();

    ASSERT_THAT(projectInfo.projectParts().size(), Eq(1));
    const ProjectPart &projectPart = *projectInfo.projectParts().at(0);
    ASSERT_THAT(projectPart.languageVersion, Eq(ProjectExplorer::LanguageVersion::CXX17));
}

TEST_F(ProjectInfoGenerator, UseCompilerFlagsForLanguageExtensions)
{
    rawProjectPart.files = QStringList{ "foo.cpp" };
    rawProjectPart.flagsForCxx.languageExtensions = ProjectExplorer::LanguageExtension::Microsoft;

    const ProjectInfo projectInfo = generate();

    ASSERT_THAT(projectInfo.projectParts().size(), Eq(1));
    const ProjectPart &projectPart = *projectInfo.projectParts().at(0);
    ASSERT_TRUE(projectPart.languageExtensions & ProjectExplorer::LanguageExtension::Microsoft);
}

TEST_F(ProjectInfoGenerator, ProjectFileKindsMatchProjectPartVersion)
{
    rawProjectPart.files = QStringList{ "foo.h" };

    const ProjectInfo projectInfo = generate();

    ASSERT_THAT(projectInfo.projectParts(),
                UnorderedElementsAre(IsProjectPart(ProjectExplorer::LanguageVersion::LatestC, ProjectFile::CHeader),
                                     IsProjectPart(ProjectExplorer::LanguageVersion::LatestC, ProjectFile::ObjCHeader),
                                     IsProjectPart(ProjectExplorer::LanguageVersion::LatestCxx, ProjectFile::CXXHeader),
                                     IsProjectPart(ProjectExplorer::LanguageVersion::LatestCxx, ProjectFile::ObjCXXHeader)));
}

void ProjectInfoGenerator::SetUp()
{
    ASSERT_TRUE(MimeDataBaseUtilities::addCppToolsMimeTypes());
}

ProjectInfo ProjectInfoGenerator::generate()
{
    QFutureInterface<void> fi;

    projectUpdateInfo.rawProjectParts += rawProjectPart;
    ::ProjectInfoGenerator generator(fi, projectUpdateInfo);

    return generator.generate();
}

} // anonymous namespace
