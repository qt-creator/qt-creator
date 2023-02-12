// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "common.h"

#include "projectitem/converters.h"

//#define REGENERATE_DATA_SETS

using namespace QmlProjectManager;

class DataSet
{
public:
    DataSet(const QString &dataSetName)
        : m_dataSetDirectory(testDataRootDir.path() + "/converter/" + dataSetName)
        , m_qmlProjectFile(Utils::FilePath::fromString(
              QString(m_dataSetDirectory.absolutePath()).append("/testfile.qmlproject")))
        , m_jsonToQmlProjectFile(Utils::FilePath::fromString(
              QString(m_dataSetDirectory.absolutePath()).append("/testfile.jsontoqml")))
        , m_qmlProjectToJsonFile(Utils::FilePath::fromString(
              QString(m_dataSetDirectory.absolutePath()).append("/testfile.qmltojson")))
    {}

    QString qmlProjectContent() const
    {
        return (m_qmlProjectFile.fileContents() ? m_qmlProjectFile.fileContents().value() : QString{});
    }
    QString jsonToQmlProjectContent() const
    {
        return m_jsonToQmlProjectFile.fileContents() ? m_jsonToQmlProjectFile.fileContents().value()
                                                     : QString{};
    }
    QString qmlProjectToJsonContent() const
    {
        return m_qmlProjectToJsonFile.fileContents() ? m_qmlProjectToJsonFile.fileContents().value()
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

QVector<DataSet> getDataSets()
{
    QVector<DataSet> dataSets;
    QDir testDataDir(testDataRootDir.path().append("/converter"));
    testDataDir.setNameFilters({"test-set-*"});
    foreach (const QString &directory, testDataDir.entryList()) {
        dataSets.append(DataSet{directory});
    }
    return dataSets;
}

#ifndef REGENERATE_DATA_SETS
TEST(QmlProjectConverterTests, QmlProjectToJson)
{
    foreach (const DataSet &dataSet, getDataSets()) {
        qDebug() << "Data set name:" << dataSet.dataSetName();

        QString targetContent = dataSet.qmlProjectToJsonContent().replace("\r\n", "\n");

        QJsonObject jsonObject{
            QmlProjectManager::Converters::qmlProjectTojson(dataSet.qmlProjectFile())};
        QString convertedContent{QJsonDocument(jsonObject).toJson()};

        ASSERT_EQ(convertedContent.toStdString(), targetContent.toStdString());
    }
}

TEST(QmlProjectConverterTests, JsonToQmlProject)
{
    foreach (const DataSet &dataSet, getDataSets()) {
        qDebug() << "Data set name:" << dataSet.dataSetName();

        QString targetContent = dataSet.jsonToQmlProjectContent().replace("\r\n", "\n");

        QString jsonContent = dataSet.qmlProjectToJsonContent();
        QJsonObject jsonObject{QJsonDocument::fromJson(jsonContent.toLatin1()).object()};
        QString convertedContent = QmlProjectManager::Converters::jsonToQmlProject(jsonObject);

        ASSERT_EQ(convertedContent.toStdString(), targetContent.toStdString());
    }
}

#else
TEST(QmlProjectConverterTests, RegenerateDataSets)
{
    foreach (const DataSet &dataSet, getDataSets()) {
        qDebug() << "Regenerating data set:" << dataSet.dataSetName();
        QJsonObject qml2json = Converters::qmlProjectTojson(dataSet.qmlProjectFile());
        QString json2qml = Converters::jsonToQmlProject(qml2json);

        dataSet.qmlProjectToJsonFile().writeFileContents(QJsonDocument(qml2json).toJson());
        dataSet.jsonToQmlProjectFile().writeFileContents(json2qml.toUtf8());
    }
    SUCCEED();
}
#endif
