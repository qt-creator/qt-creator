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
#include "testenvironment.h"

#include <clangsupport_global.h>
#include <clangreferencescollector.h>
#include <clangdocument.h>
#include <clangdocuments.h>
#include <clangtranslationunit.h>
#include <fixitcontainer.h>
#include <projectpart.h>
#include <projects.h>
#include <sourcelocationcontainer.h>
#include <sourcerangecontainer.h>
#include <unsavedfiles.h>

#include <utils/qtcassert.h>

#include <clang-c/Index.h>

using ::testing::Contains;
using ::testing::Not;
using ::testing::ContainerEq;
using ::testing::Eq;

using ::ClangBackEnd::ProjectPart;
using ::ClangBackEnd::SourceLocationContainer;
using ::ClangBackEnd::Document;
using ::ClangBackEnd::UnsavedFiles;
using ::ClangBackEnd::ReferencesResult;
using ::ClangBackEnd::SourceRangeContainer;

using References = QVector<SourceRangeContainer>;

namespace {

std::ostream &operator<<(std::ostream &os, const ReferencesResult &value)
{
    os << "ReferencesResult(";
    os << value.isLocalVariable << ", {";
    for (const SourceRangeContainer &r : value.references) {
        os << r.start().line() << ",";
        os << r.start().column() << ",";
        QTC_CHECK(r.start().line() == r.end().line());
        os << r.end().column() - r.start().column() << ",";
    }
    os << "})";

    return os;
}

struct Data {
    Data()
    {
        document.parse();
    }

    ProjectPart projectPart{
        Utf8StringLiteral("projectPartId"),
        TestEnvironment::addPlatformArguments({Utf8StringLiteral("-std=c++14")})};
    ClangBackEnd::ProjectParts projects;
    ClangBackEnd::UnsavedFiles unsavedFiles;
    ClangBackEnd::Documents documents{projects, unsavedFiles};
    Document document{Utf8StringLiteral(TESTDATA_DIR"/references.cpp"),
                      projectPart,
                      Utf8StringVector(),
                      documents};
};

class ReferencesCollector : public ::testing::Test
{
protected:
    ReferencesResult getReferences(uint line, uint column)
    {
        return d->document.translationUnit().references(line, column);
    }

    SourceLocationContainer createSourceLocation(uint line, uint column) const
    {
        return SourceLocationContainer(d->document.filePath(), line, column);
    }

    SourceRangeContainer createSourceRange(uint line, uint column, uint length) const
    {
        return SourceRangeContainer {
            createSourceLocation(line, column),
            createSourceLocation(line, column + length)
        };
    }

    static void SetUpTestCase();
    static void TearDownTestCase();

private:
    static std::unique_ptr<Data> d;
};

// This test is not strictly needed as the plugin is supposed to put the cursor
// on the identifier start.
TEST_F(ReferencesCollector, CursorNotOnIdentifier)
{
    const ReferencesResult expected { false, {}, };

    const ReferencesResult actual = getReferences(3, 5);

    ASSERT_THAT(actual, expected);
}

TEST_F(ReferencesCollector, LocalVariableWithSingleUse)
{
    const ReferencesResult expected {
        true,
        {createSourceRange(3, 9, 3)},
    };

    const ReferencesResult actual = getReferences(3, 9);

    ASSERT_THAT(actual, expected);
}

TEST_F(ReferencesCollector, LocalVariableWithTwoUses)
{
    const ReferencesResult expected {
        true,
        {createSourceRange(10, 9, 3),
         createSourceRange(11, 12, 3)},
    };

    const ReferencesResult actual = getReferences(10, 9);

    ASSERT_THAT(actual, expected);
}

TEST_F(ReferencesCollector, ClassName)
{
    const ReferencesResult expected {
        false,
        {createSourceRange(16, 7, 3),
         createSourceRange(19, 5, 3)},
    };

    const ReferencesResult actual = getReferences(16, 7);

    ASSERT_THAT(actual, expected);
}

TEST_F(ReferencesCollector, Namespace)
{
    const ReferencesResult expected {
        false,
        {createSourceRange(24, 11, 1),
         createSourceRange(25, 11, 1),
         createSourceRange(26, 1, 1)},
    };

    const ReferencesResult actual = getReferences(24, 11);

    ASSERT_THAT(actual, expected);
}

TEST_F(ReferencesCollector, ClassNameDeclaredWithUsing)
{
    const ReferencesResult expected {
        false,
        {createSourceRange(30, 21, 3),
         createSourceRange(31, 10, 3)},
    };

    const ReferencesResult actual = getReferences(30, 21);

    ASSERT_THAT(actual, expected);
}

TEST_F(ReferencesCollector, ClassNameForwardDeclared)
{
    const ReferencesResult expected {
        false,
        {createSourceRange(35, 7, 3),
         createSourceRange(36, 14, 3)},
    };

    const ReferencesResult actual = getReferences(35, 7);

    ASSERT_THAT(actual, expected);
}

TEST_F(ReferencesCollector, ClassNameAndNewExpression)
{
    const ReferencesResult expected {
        false,
        {createSourceRange(40, 7, 3),
         createSourceRange(43, 9, 3)},
    };

    const ReferencesResult actual = getReferences(40, 7);

    ASSERT_THAT(actual, expected);
}

TEST_F(ReferencesCollector, InstantiatedTemplateObject)
{
    const ReferencesResult expected {
        true,
        {createSourceRange(52, 19, 3),
         createSourceRange(53, 5, 3)},
    };

    const ReferencesResult actual = getReferences(52, 19);

    ASSERT_THAT(actual, expected);
}

TEST_F(ReferencesCollector, VariableInTemplate)
{
    const ReferencesResult expected {
        true,
        {createSourceRange(62, 13, 3),
         createSourceRange(63, 11, 3)},
    };

    const ReferencesResult actual = getReferences(62, 13);

    ASSERT_THAT(actual, expected);
}

TEST_F(ReferencesCollector, MemberInTemplate)
{
    const ReferencesResult expected {
        false,
        {createSourceRange(64, 16, 3),
         createSourceRange(67, 7, 3)},
    };

    const ReferencesResult actual = getReferences(67, 7);

    ASSERT_THAT(actual, expected);
}

TEST_F(ReferencesCollector, TemplateType)
{
    const ReferencesResult expected {
        false,
        {createSourceRange(58, 19, 1),
         createSourceRange(60, 5, 1),
         createSourceRange(67, 5, 1)},
    };

    const ReferencesResult actual = getReferences(58, 19);

    ASSERT_THAT(actual, expected);
}

TEST_F(ReferencesCollector, MemberAccessIntoTemplateParameter)
{
    const ReferencesResult expected { false, {}, };

    const ReferencesResult actual = getReferences(76, 9);

    ASSERT_THAT(actual, expected);
}

TEST_F(ReferencesCollector, ConstructorAsType)
{
    const ReferencesResult expected {
        false,
        {createSourceRange(81, 8, 3),
         createSourceRange(82, 5, 3),
         createSourceRange(83, 6, 3)},
    };

    const ReferencesResult actual = getReferences(82, 5);

    ASSERT_THAT(actual, expected);
}

TEST_F(ReferencesCollector, OverloadsFreeStanding)
{
    const ReferencesResult expected {
        false,
        {createSourceRange(88, 5, 3),
         createSourceRange(89, 5, 3)},
    };

    const ReferencesResult actual = getReferences(88, 5);

    ASSERT_THAT(actual, expected);
}

TEST_F(ReferencesCollector, OverloadsMemberFunctions)
{
    const ReferencesResult expected {
        false,
        {createSourceRange(94, 9, 3),
         createSourceRange(95, 9, 3)},
    };

    const ReferencesResult actual = getReferences(94, 9);

    ASSERT_THAT(actual, expected);
}

TEST_F(ReferencesCollector, FunctionAndTemplateFunction)
{
    const ReferencesResult expected {
        false,
        {createSourceRange(100, 26, 3),
         createSourceRange(101, 5, 3)},
    };

    const ReferencesResult actual = getReferences(100, 26);

    ASSERT_THAT(actual, expected);
}

TEST_F(ReferencesCollector, FunctionAndTemplateFunctionAsMember)
{
    const ReferencesResult expected {
        false,
        {createSourceRange(106, 30, 3),
         createSourceRange(107, 9, 3)},
    };

    const ReferencesResult actual = getReferences(106, 30);

    ASSERT_THAT(actual, expected);
}

TEST_F(ReferencesCollector, EnumType)
{
    const ReferencesResult expected {
        false,
        {createSourceRange(112, 6, 2),
         createSourceRange(113, 8, 2)},
    };

    const ReferencesResult actual = getReferences(112, 6);

    ASSERT_THAT(actual, expected);
}

TEST_F(ReferencesCollector, LambdaCapturedObject)
{
    const ReferencesResult expected {
        true,
        {createSourceRange(122, 15, 3),
         createSourceRange(122, 33, 3)},
    };

    const ReferencesResult actual = getReferences(122, 15);

    ASSERT_THAT(actual, expected);
}

//// Disabled because it looks like the lambda initializer is not yet exposed by libclang.
TEST_F(ReferencesCollector, DISABLED_LambdaCaptureInitializer)
{
    const ReferencesResult expected {
        true,
        {createSourceRange(121, 19, 3),
         createSourceRange(122, 19, 3)},
    };

    const ReferencesResult actual = getReferences(122, 19);

    ASSERT_THAT(actual, expected);
}

TEST_F(ReferencesCollector, TemplateSpecialization)
{
    const ReferencesResult expected {
        false,
        {createSourceRange(127, 25, 3),
         createSourceRange(128, 25, 3),
         createSourceRange(129, 18, 3)},
    };

    const ReferencesResult actual = getReferences(127, 25);

    ASSERT_THAT(actual, expected);
}

TEST_F(ReferencesCollector, TemplateDependentName)
{
    const ReferencesResult expected {
        false,
        {createSourceRange(133, 34, 3)},
    };

    const ReferencesResult actual = getReferences(133, 34);

    ASSERT_THAT(actual, expected);
}

TEST_F(ReferencesCollector, FunctionCallAndDefinition)
{
    const ReferencesResult expected {
        false,
        {createSourceRange(140, 5, 3),
         createSourceRange(142, 25, 3)},
    };

    const ReferencesResult actual = getReferences(140, 5);

    ASSERT_THAT(actual, expected);
}

TEST_F(ReferencesCollector, ObjectLikeMacro)
{
    const ReferencesResult expected {
        false,
        {createSourceRange(147, 9, 3),
         createSourceRange(150, 12, 3)},
    };

    const ReferencesResult actual = getReferences(147, 9);

    ASSERT_THAT(actual, expected);
}

TEST_F(ReferencesCollector, FunctionLikeMacro)
{
    const ReferencesResult expected {
        false,
        {createSourceRange(155, 9, 3),
         createSourceRange(158, 12, 3)},
    };

    const ReferencesResult actual = getReferences(155, 9);

    ASSERT_THAT(actual, expected);
}

TEST_F(ReferencesCollector, ArgumentToFunctionLikeMacro)
{
    const ReferencesResult expected {
        true,
        {createSourceRange(156, 27, 3),
         createSourceRange(158, 16, 3)},
    };

    const ReferencesResult actual = getReferences(156, 27);

    ASSERT_THAT(actual, expected);
}

std::unique_ptr<Data> ReferencesCollector::d;

void ReferencesCollector::SetUpTestCase()
{
    d.reset(new Data);
}

void ReferencesCollector::TearDownTestCase()
{
    d.reset();
}

} // anonymous namespace
