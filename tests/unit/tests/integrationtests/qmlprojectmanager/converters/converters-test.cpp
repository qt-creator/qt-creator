// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <gmock/gmock-matchers.h>
#include <gmock/gmock.h>
#include <gtest/gtest-printers.h>
#include <gtest/gtest-typed-test.h>
#include <gtest/gtest.h>

#include <qmlprojectmanager/buildsystem/projectitem/converters.h>

#include <3rdparty/googletest/googletest/include/gtest/gtest.h>
#include <QJsonDocument>

namespace {
constexpr QLatin1String localTestDataDir{"data"};

class QmlProjectConverter : public testing::TestWithParam<QString>
{
public:
    void setDataSource(const QString &dataSetName)
    {
        m_dataSetDirectory = Utils::FilePath::fromUserInput(localTestDataDir) / dataSetName;
    }

    Utils::FilePath sourceQmlProject() const { return m_dataSetDirectory / "source.qmlproject"; }

    Utils::FilePath convertedQmlProject() const { return m_dataSetDirectory / "converted.json"; }

    Utils::FilePath sourceJsonProject() const { return m_dataSetDirectory / "source.json"; }

    Utils::FilePath convertedJsonProject() const
    {
        return m_dataSetDirectory / "converted.qmlproject";
    }

    QString convertedQmlProjectContent() const
    {
        return QString::fromUtf8(*convertedQmlProject().fileContents()).replace("\r\n", "\n");
    }

    QString convertedJsonProjectContent() const
    {
        return QString::fromUtf8(*convertedJsonProject().fileContents()).replace("\r\n", "\n");
    }

private:
    Utils::FilePath m_dataSetDirectory;
};

INSTANTIATE_TEST_SUITE_P(QmlProjectItem,
                         QmlProjectConverter,
                         ::testing::Values(QString("test-set-1"),
                                           QString("test-set-2"),
                                           QString("test-set-3"),
                                           QString("test-set-mcu-1"),
                                           QString("test-set-mcu-2"),
                                           QString("test-set-font-files")));

TEST_P(QmlProjectConverter, qml_project_to_json)
{
    setDataSource(GetParam());

    const auto converted = QmlProjectManager::Converters::qmlProjectTojson(sourceQmlProject());

    const auto convertedContent = QString::fromUtf8(QJsonDocument(converted).toJson());
    ASSERT_EQ(convertedContent, convertedQmlProjectContent());
}

TEST_P(QmlProjectConverter, json_to_qml_project)
{
    setDataSource(GetParam());
    const auto sourceContent = QJsonDocument::fromJson(*sourceJsonProject().fileContents()).object();

    const auto convertedContent = QmlProjectManager::Converters::jsonToQmlProject(sourceContent);

    ASSERT_EQ(convertedContent, convertedJsonProjectContent());
}

} // namespace
