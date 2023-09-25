// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "../utils/googletest.h" // IWYU pragma: keep

#include <qmlprojectmanager/buildsystem/projectitem/converters.h>

#include <QJsonDocument>

namespace {
constexpr QLatin1String localTestDataDir{UNITTEST_DIR "/qmlprojectmanager/data"};

class QmlProjectConverter : public testing::TestWithParam<QString>
{
public:
    void setDataSource(const QString &dataSetName)
    {
        m_dataSetDirectory.setPath(localTestDataDir + "/converter/" + dataSetName);

        m_qmlProjectFile = Utils::FilePath::fromString(
            QString(m_dataSetDirectory.absolutePath()).append("/testfile.qmlproject"));
        m_jsonToQmlProjectFile = Utils::FilePath::fromString(
            QString(m_dataSetDirectory.absolutePath()).append("/testfile.jsontoqml"));
        m_qmlProjectToJsonFile = Utils::FilePath::fromString(
            QString(m_dataSetDirectory.absolutePath()).append("/testfile.qmltojson"));
    }

    QString qmlProjectContent() const
    {
        return (m_qmlProjectFile.fileContents()
                    ? QString::fromLatin1(m_qmlProjectFile.fileContents().value())
                    : QString{});
    }

    QString jsonToQmlProjectContent() const
    {
        return m_jsonToQmlProjectFile.fileContents()
                   ? QString::fromLatin1(m_jsonToQmlProjectFile.fileContents().value())
                   : QString{};
    }

    QString qmlProjectToJsonContent() const
    {
        return m_qmlProjectToJsonFile.fileContents()
                   ? QString::fromLatin1(m_qmlProjectToJsonFile.fileContents().value())
                   : QString{};
    }

    QString dataSetPath() const { return m_dataSetDirectory.absolutePath(); }

    QString dataSetName() const { return m_dataSetDirectory.dirName(); }

    Utils::FilePath qmlProjectFile() const { return m_qmlProjectFile; }

    Utils::FilePath jsonToQmlProjectFile() const { return m_jsonToQmlProjectFile; }

    Utils::FilePath qmlProjectToJsonFile() const { return m_qmlProjectToJsonFile; }

private:
    QDir m_dataSetDirectory;
    Utils::FilePath m_qmlProjectFile;
    Utils::FilePath m_jsonToQmlProjectFile;
    Utils::FilePath m_qmlProjectToJsonFile;
};

INSTANTIATE_TEST_SUITE_P(QmlProjectItem,
                         QmlProjectConverter,
                         ::testing::Values(QString("test-set-1"),
                                           QString("test-set-2"),
                                           QString("test-set-3")));

TEST_P(QmlProjectConverter, qml_project_to_json)
{
    // GIVEN
    setDataSource(GetParam());
    QString targetContent = qmlProjectToJsonContent().replace("\r\n", "\n");
    auto qmlFile = qmlProjectFile();

    // WHEN
    auto jsonObject = QmlProjectManager::Converters::qmlProjectTojson(qmlFile);

    // THEN
    QString convertedContent{QString::fromLatin1(QJsonDocument(jsonObject).toJson())};
    ASSERT_THAT(convertedContent, Eq(targetContent));
}

TEST_P(QmlProjectConverter, json_to_qml_project)
{
    // GIVEN
    setDataSource(GetParam());
    QString targetContent = jsonToQmlProjectContent().replace("\r\n", "\n");
    auto jsonContent = qmlProjectToJsonContent().toLatin1();

    // WHEN
    auto jsonObject{QJsonDocument::fromJson(jsonContent).object()};

    // THEN
    QString convertedContent = QmlProjectManager::Converters::jsonToQmlProject(jsonObject);
    ASSERT_THAT(convertedContent, Eq(targetContent));
}

} // namespace
