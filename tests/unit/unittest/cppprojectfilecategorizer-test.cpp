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
#include <utils/mimetypes/mimedatabase.h>

#include <QDebug>
#include <QFileInfo>

using CppTools::ProjectFile;
using CppTools::ProjectFiles;
using CppTools::ProjectFileCategorizer;

using testing::IsNull;
using testing::NotNull;
using testing::Eq;
using testing::Gt;
using testing::Contains;
using testing::EndsWith;
using testing::AllOf;

namespace CppTools {

void PrintTo(const ProjectFile &projectFile, std::ostream *os)
{
    *os << "ProjectFile(";
    QString output;
    QDebug(&output) << projectFile;
    *os << qPrintable(output);
    *os << ")";
}

} // namespace CppTools

namespace {

class ProjectFileCategorizer : public ::testing::Test
{
protected:
    void SetUp() override;

    static ProjectFiles singleFile(const QString &filePath, ProjectFile::Kind kind);

protected:
    const QString dummyProjectPartName;
};

using ProjectFileCategorizerVerySlowTest = ProjectFileCategorizer;

TEST_F(ProjectFileCategorizerVerySlowTest, C)
{
    const QStringList inputFilePaths = QStringList() << "foo.c" << "foo.h";
    const ProjectFiles expected {
        ProjectFile("foo.c", ProjectFile::CSource),
        ProjectFile("foo.h", ProjectFile::CHeader),
    };

    ::ProjectFileCategorizer categorizer{dummyProjectPartName, inputFilePaths};

    ASSERT_THAT(categorizer.cSources(), Eq(expected));
    ASSERT_TRUE(categorizer.cxxSources().isEmpty());
    ASSERT_TRUE(categorizer.objcSources().isEmpty());
    ASSERT_TRUE(categorizer.objcxxSources().isEmpty());
}

TEST_F(ProjectFileCategorizerVerySlowTest, CxxWithUnambiguousHeaderSuffix)
{
    const QStringList inputFilePaths = QStringList() << "foo.cpp" << "foo.hpp";
    const ProjectFiles expected {
        ProjectFile("foo.cpp", ProjectFile::CXXSource),
        ProjectFile("foo.hpp", ProjectFile::CXXHeader),
    };

    ::ProjectFileCategorizer categorizer{dummyProjectPartName, inputFilePaths};

    ASSERT_THAT(categorizer.cxxSources(), Eq(expected));
    ASSERT_TRUE(categorizer.cSources().isEmpty());
    ASSERT_TRUE(categorizer.objcSources().isEmpty());
    ASSERT_TRUE(categorizer.objcxxSources().isEmpty());
}

TEST_F(ProjectFileCategorizerVerySlowTest, CxxWithAmbiguousHeaderSuffix)
{
    const QStringList inputFilePaths = QStringList() << "foo.cpp" << "foo.h";
    const ProjectFiles expected {
        ProjectFile("foo.cpp", ProjectFile::CXXSource),
        ProjectFile("foo.h", ProjectFile::CXXHeader),
    };

    ::ProjectFileCategorizer categorizer{dummyProjectPartName, inputFilePaths};

    ASSERT_THAT(categorizer.cxxSources(), Eq(expected));
    ASSERT_TRUE(categorizer.cSources().isEmpty());
    ASSERT_TRUE(categorizer.objcSources().isEmpty());
    ASSERT_TRUE(categorizer.objcxxSources().isEmpty());
}

TEST_F(ProjectFileCategorizerVerySlowTest, ObjectiveC)
{
    const QStringList inputFilePaths = QStringList() << "foo.m" << "foo.h";
    const ProjectFiles expected {
        ProjectFile("foo.m", ProjectFile::ObjCSource),
        ProjectFile("foo.h", ProjectFile::ObjCHeader),
    };

    ::ProjectFileCategorizer categorizer{dummyProjectPartName, inputFilePaths};

    ASSERT_THAT(categorizer.objcSources(), Eq(expected));
    ASSERT_TRUE(categorizer.cxxSources().isEmpty());
    ASSERT_TRUE(categorizer.cSources().isEmpty());
    ASSERT_TRUE(categorizer.objcxxSources().isEmpty());
}

TEST_F(ProjectFileCategorizerVerySlowTest, ObjectiveCxx)
{
    const QStringList inputFilePaths = QStringList() << "foo.mm" << "foo.h";
    const ProjectFiles expected {
        ProjectFile("foo.mm", ProjectFile::ObjCXXSource),
        ProjectFile("foo.h", ProjectFile::ObjCXXHeader),
    };

    ::ProjectFileCategorizer categorizer{dummyProjectPartName, inputFilePaths};

    ASSERT_THAT(categorizer.objcxxSources(), Eq(expected));
    ASSERT_TRUE(categorizer.objcSources().isEmpty());
    ASSERT_TRUE(categorizer.cSources().isEmpty());
    ASSERT_TRUE(categorizer.cxxSources().isEmpty());
}

TEST_F(ProjectFileCategorizerVerySlowTest, MixedCAndCxx)
{
    const QStringList inputFilePaths = QStringList() << "foo.cpp" << "foo.h"
                                                     << "bar.c" << "bar.h";
    const ProjectFiles expectedCxxSources {
        ProjectFile("foo.cpp", ProjectFile::CXXSource),
        ProjectFile("foo.h", ProjectFile::CXXHeader),
        ProjectFile("bar.h", ProjectFile::CXXHeader),
    };
    const ProjectFiles expectedCSources {
        ProjectFile("bar.c", ProjectFile::CSource),
        ProjectFile("foo.h", ProjectFile::CHeader),
        ProjectFile("bar.h", ProjectFile::CHeader),
    };

    ::ProjectFileCategorizer categorizer{dummyProjectPartName, inputFilePaths};

    ASSERT_THAT(categorizer.cxxSources(), Eq(expectedCxxSources));
    ASSERT_THAT(categorizer.cSources(), Eq(expectedCSources));
    ASSERT_TRUE(categorizer.objcSources().isEmpty());
    ASSERT_TRUE(categorizer.objcxxSources().isEmpty());
}

TEST_F(ProjectFileCategorizerVerySlowTest, AmbiguousHeaderOnly)
{
    ::ProjectFileCategorizer categorizer{dummyProjectPartName, QStringList() << "foo.h"};

    ASSERT_THAT(categorizer.cSources(), Eq(singleFile("foo.h", ProjectFile::CHeader)));
    ASSERT_THAT(categorizer.cxxSources(), Eq(singleFile("foo.h", ProjectFile::CXXHeader)));
    ASSERT_THAT(categorizer.objcSources(), Eq(singleFile("foo.h", ProjectFile::ObjCHeader)));
    ASSERT_THAT(categorizer.objcxxSources(), Eq(singleFile("foo.h", ProjectFile::ObjCXXHeader)));
}

void ProjectFileCategorizer::SetUp()
{
    ASSERT_TRUE(MimeDataBaseUtilities::addCppToolsMimeTypes());
}

QVector<CppTools::ProjectFile>ProjectFileCategorizer::singleFile(const QString &filePath,
                                                                 ProjectFile::Kind kind)
{
    return { ProjectFile(filePath, kind) };
}

} // anonymous namespace
