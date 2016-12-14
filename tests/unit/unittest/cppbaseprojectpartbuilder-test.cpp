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

#include "googletest.h"
#include "gtest-qt-printing.h"
#include "mimedatabase-utilities.h"

#include <cpptools/cppprojectfilecategorizer.h>
#include <cpptools/cppbaseprojectpartbuilder.h>
#include <cpptools/cppprojectinterface.h>
#include <cpptools/projectinfo.h>

#include <projectexplorer/headerpath.h>
#include <projectexplorer/project.h>

#include <utils/mimetypes/mimedatabase.h>

#include <QObject>

using CppTools::BaseProjectPartBuilder;
using CppTools::ProjectFile;
using CppTools::ProjectFiles;
using CppTools::ProjectInfo;
using CppTools::ProjectInterface;
using CppTools::ProjectPart;
using CppTools::ToolChainInterface;
using CppTools::ToolChainInterfacePtr;

using testing::Eq;

namespace {

class EditableToolChain : public CppTools::ToolChainInterface
{
public:
    void setCompilerFlags(ProjectExplorer::ToolChain::CompilerFlags compilerFlags)
    {
        m_compilerFlags = compilerFlags;
    }

private:
    Core::Id type() const override { return Core::Id(); }
    bool isMsvc2015Toolchain() const override { return false; }
    unsigned wordWidth() const override { return 64; }
    QString targetTriple() const override {  return QString(); }

    QByteArray predefinedMacros() const override { return QByteArray(); }
    QList<ProjectExplorer::HeaderPath> systemHeaderPaths() const override
    { return QList<ProjectExplorer::HeaderPath>(); }

    ProjectExplorer::WarningFlags warningFlags() const override
    { return ProjectExplorer::WarningFlags(); }

    ProjectExplorer::ToolChain::CompilerFlags compilerFlags() const override
    { return m_compilerFlags; }

private:
    ProjectExplorer::ToolChain::CompilerFlags m_compilerFlags;
};

class EditableProject : public CppTools::ProjectInterface
{
public:
    void setToolChain(ToolChainInterface *toolChain)
    {
        m_toolChain = toolChain;
    }

private:
    QString displayName() const override { return QString(); }
    QString projectFilePath() const override { return QString(); }

    ToolChainInterfacePtr toolChain(ProjectExplorer::ToolChain::Language,
                                    const QStringList &) const override
    { return ToolChainInterfacePtr(m_toolChain); }

private:
    CppTools::ToolChainInterface *m_toolChain = nullptr;
};

class BaseProjectPartBuilder : public ::testing::Test
{
protected:
    void SetUp() override;

    QObject dummyProjectExplorerProject;
    ProjectInfo projectInfo{static_cast<ProjectExplorer::Project *>(&dummyProjectExplorerProject)};
};

TEST_F(BaseProjectPartBuilder, CreateNoPartsForEmptyFileList)
{
    ::BaseProjectPartBuilder builder(new EditableProject, projectInfo);

    builder.createProjectPartsForFiles(QStringList());

    ASSERT_TRUE(projectInfo.projectParts().isEmpty());
}

TEST_F(BaseProjectPartBuilder, CreateSinglePart)
{
    ::BaseProjectPartBuilder builder(new EditableProject, projectInfo);

    builder.createProjectPartsForFiles(QStringList() << "foo.cpp" << "foo.h");

    ASSERT_THAT(projectInfo.projectParts().size(), Eq(1));
}

TEST_F(BaseProjectPartBuilder, CreateMultipleParts)
{
    ::BaseProjectPartBuilder builder(new EditableProject, projectInfo);

    builder.createProjectPartsForFiles(QStringList() << "foo.cpp" << "foo.h"
                                                     << "bar.c" << "bar.h");

    ASSERT_THAT(projectInfo.projectParts().size(), Eq(2));
}

TEST_F(BaseProjectPartBuilder, ProjectPartIndicatesObjectiveCExtensionsByDefault)
{
    ::BaseProjectPartBuilder builder(new EditableProject, projectInfo);

    builder.createProjectPartsForFiles(QStringList() << "foo.mm");

    ASSERT_THAT(projectInfo.projectParts().size(), Eq(1));
    const ProjectPart &projectPart = *projectInfo.projectParts().at(0);
    ASSERT_TRUE(projectPart.languageExtensions & ProjectPart::ObjectiveCExtensions);
}

TEST_F(BaseProjectPartBuilder, ProjectPartHasLatestLanguageVersionByDefault)
{
    ::BaseProjectPartBuilder builder(new EditableProject, projectInfo);

    builder.createProjectPartsForFiles(QStringList() << "foo.cpp");

    ASSERT_THAT(projectInfo.projectParts().size(), Eq(1));
    const ProjectPart &projectPart = *projectInfo.projectParts().at(0);
    ASSERT_THAT(projectPart.languageVersion, Eq(ProjectPart::LatestCxxVersion));
}

TEST_F(BaseProjectPartBuilder, ToolChainSetsLanguageVersion)
{
    auto toolChain = new EditableToolChain;
    toolChain->setCompilerFlags(ProjectExplorer::ToolChain::StandardCxx98);
    auto project = new EditableProject;
    project->setToolChain(toolChain);
    ::BaseProjectPartBuilder builder(project, projectInfo);

    builder.createProjectPartsForFiles(QStringList() << "foo.cpp");

    ASSERT_THAT(projectInfo.projectParts().size(), Eq(1));
    const ProjectPart &projectPart = *projectInfo.projectParts().at(0);
    ASSERT_THAT(projectPart.languageVersion, Eq(ProjectPart::CXX98));
}

TEST_F(BaseProjectPartBuilder, ToolChainSetsLanguageExtensions)
{
    auto toolChain = new EditableToolChain;
    toolChain->setCompilerFlags(ProjectExplorer::ToolChain::MicrosoftExtensions);
    auto project = new EditableProject;
    project->setToolChain(toolChain);
    ::BaseProjectPartBuilder builder(project, projectInfo);

    builder.createProjectPartsForFiles(QStringList() << "foo.cpp");

    ASSERT_THAT(projectInfo.projectParts().size(), Eq(1));
    const ProjectPart &projectPart = *projectInfo.projectParts().at(0);
    ASSERT_TRUE(projectPart.languageExtensions & ProjectPart::MicrosoftExtensions);
}

TEST_F(BaseProjectPartBuilder, ProjectFileKindsMatchProjectPartVersion)
{
    ::BaseProjectPartBuilder builder(new EditableProject, projectInfo);

    builder.createProjectPartsForFiles(QStringList() << "foo.h");

    const QVector<ProjectPart::Ptr> projectParts = projectInfo.projectParts();
    ASSERT_THAT(projectParts.size(), Eq(4));
    ASSERT_THAT(projectParts.at(0)->languageVersion, Eq(ProjectPart::LatestCxxVersion));
    ASSERT_THAT(projectParts.at(0)->files.at(0).kind, Eq(ProjectFile::CXXHeader));
    ASSERT_THAT(projectParts.at(1)->languageVersion, Eq(ProjectPart::LatestCxxVersion));
    ASSERT_THAT(projectParts.at(1)->files.at(0).kind, Eq(ProjectFile::ObjCXXHeader));
    ASSERT_THAT(projectParts.at(2)->languageVersion, Eq(ProjectPart::LatestCVersion));
    ASSERT_THAT(projectParts.at(2)->files.at(0).kind, Eq(ProjectFile::CHeader));
    ASSERT_THAT(projectParts.at(3)->languageVersion, Eq(ProjectPart::LatestCVersion));
    ASSERT_THAT(projectParts.at(3)->files.at(0).kind, Eq(ProjectFile::ObjCHeader));
}

void BaseProjectPartBuilder::SetUp()
{
    ASSERT_TRUE(MimeDataBaseUtilities::addCppToolsMimeTypes());
}

} // anonymous namespace
