// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <gmock/gmock-matchers.h>
#include <gmock/gmock.h>
#include <gtest/gtest-printers.h>
#include <gtest/gtest-typed-test.h>
#include <gtest/gtest.h>
#include <printers/gtest-qt-printing.h>
#include <utils/google-using-declarations.h>

#include <3rdparty/googletest/googletest/include/gtest/gtest.h>

#include <utils/fileutils.h>

#include <astcheck/astcheck.h>

namespace QmlJS::StaticAnalysis {

static std::ostream &operator<<(std::ostream &os, const Message &message)
{
    return os << message.toDiagnosticMessage().message.toStdString();
}
} // namespace QmlJS::StaticAnalysis

namespace {
constexpr QLatin1String localTestDataDir{"data"};

enum { IsUiFile = true, NoUiFile = false };

enum { HasError = true, HasNoError = false };

struct Parameters
{
    QString filename;
    bool hasError;
    bool isUiFile;
    QmlJS::StaticAnalysis::Type errorType;
};

std::ostream &operator<<(std::ostream &os, const Parameters &obj)
{
    return os << "FileTestCase(filename: " << obj.filename << ", has error: " << std::boolalpha
              << obj.hasError << ")"
              << ", is ui file: " << std::boolalpha << obj.isUiFile << ")"
              << ", error type: " << obj.errorType;
}

class AstCheckTest : public testing::TestWithParam<Parameters>
{
protected:
    AstCheckTest() { setDataSource(parameter); }

    void setDataSource(const Parameters &dataSetName)
    {
        dataSetDirectory.setPath(localTestDataDir + "/" + dataSetName.filename);

        qmlFile = Utils::FilePath::fromString(QString(dataSetDirectory.absolutePath()).append(".qml"));

        EXPECT_TRUE(qmlFile.exists());

        parseDocument();
    }

    QString qmlFileContent() const
    {
        auto content = qmlFile.fileContents();
        return (content ? QString::fromLatin1(content.value()) : QString{});
    }

    void parseDocument()
    {
        auto doc = QmlJS::Document::create(qmlFile, QmlJS::Dialect::QmlQtQuick2Ui);

        doc->setSource(qmlFileContent());
        doc->parseQml();
        document = doc;

        EXPECT_TRUE(document->isParsedCorrectly());
    }

    QString dataSetPath() const { return dataSetDirectory.absolutePath(); }

    QString dataSetName() const { return dataSetDirectory.dirName(); }

    auto createAstCheck()
    {
        QmlDesigner::AstCheck astCheck(document);

        if (!parameter.isUiFile)
            astCheck.disableQmlDesignerUiFileChecks();

        return astCheck;
    }

protected:
    QDir dataSetDirectory;
    Utils::FilePath qmlFile;
    QmlJS::Document::Ptr document;
    Parameters parameter = GetParam();
};

auto ast_check_printer = [](const testing::TestParamInfo<Parameters> &info) {
    auto &parameter = info.param;

    std::string name = parameter.filename.toStdString();
    name += parameter.hasError ? "_has_error_" : "_no_error_";
    name += parameter.isUiFile ? "ui_file_" : "no_uifile_";
    name += std::to_string(parameter.errorType);

    return name;
};

INSTANTIATE_TEST_SUITE_P(
    AstCheck,
    AstCheckTest,
    ::testing::Values(
        Parameters{"functions_not_supported_in_ui", HasNoError, NoUiFile, QmlJS::StaticAnalysis::UnknownType},
        Parameters{"functions_not_supported_in_ui",
                   HasError,
                   IsUiFile,
                   QmlJS::StaticAnalysis::ErrFunctionsNotSupportedInQmlUi},
        Parameters{"no_code_and_no_blocks",
                   HasError,
                   NoUiFile,
                   QmlJS::StaticAnalysis::WarnImperativeCodeNotEditableInVisualDesigner},
        Parameters{"no_code_and_no_blocks",
                   HasError,
                   IsUiFile,
                   QmlJS::StaticAnalysis::ErrBlocksNotSupportedInQmlUi},
        Parameters{"states_only_in_root_node",
                   HasError,
                   NoUiFile,
                   QmlJS::StaticAnalysis::WarnStatesOnlyInRootItemForVisualDesigner},
        Parameters{"states_only_in_root_node",
                   HasError,
                   IsUiFile,
                   QmlJS::StaticAnalysis::WarnStatesOnlyInRootItemForVisualDesigner},
        Parameters{"invalid_id", HasError, NoUiFile, QmlJS::StaticAnalysis::ErrInvalidIdeInVisualDesigner},
        Parameters{"invalid_id", HasError, IsUiFile, QmlJS::StaticAnalysis::ErrInvalidIdeInVisualDesigner},
        Parameters{"unsupported_root_type_in_ui", HasNoError, NoUiFile, QmlJS::StaticAnalysis::UnknownType},
        Parameters{"unsupported_root_type_in_ui",
                   HasError,
                   IsUiFile,
                   QmlJS::StaticAnalysis::ErrUnsupportedRootTypeInQmlUi},
        Parameters{"unsupported_root_type",
                   HasError,
                   NoUiFile,
                   QmlJS::StaticAnalysis::ErrUnsupportedRootTypeInVisualDesigner},
        Parameters{"unsupported_root_type",
                   HasError,
                   IsUiFile,
                   QmlJS::StaticAnalysis::ErrUnsupportedRootTypeInVisualDesigner},
        Parameters{"unsupported_type_in_ui", HasNoError, NoUiFile, QmlJS::StaticAnalysis::UnknownType},
        Parameters{"unsupported_type_in_ui",
                   HasError,
                   IsUiFile,
                   QmlJS::StaticAnalysis::ErrUnsupportedTypeInQmlUi},
        Parameters{"no_reference_to_parent_in_ui",
                   HasError,
                   NoUiFile,
                   QmlJS::StaticAnalysis::WarnReferenceToParentItemNotSupportedByVisualDesigner},
        Parameters{"no_reference_to_parent_in_ui",
                   HasError,
                   IsUiFile,
                   QmlJS::StaticAnalysis::ErrReferenceToParentItemNotSupportedInQmlUi}

        ),
    ast_check_printer);

TEST_P(AstCheckTest, qml_simple_check)
{
    auto astcheck = createAstCheck();

    auto messages = astcheck();

    ASSERT_EQ(messages.isEmpty(), !parameter.hasError) << messages;
    if (parameter.hasError) {
        ASSERT_THAT(messages,
                    Contains(Field(&QmlJS::StaticAnalysis::Message::type, parameter.errorType)));
    }
}

} // namespace
