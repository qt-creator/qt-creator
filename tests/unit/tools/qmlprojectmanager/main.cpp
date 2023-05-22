// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QJsonDocument>
#include <qmlprojectmanager/buildsystem/projectitem/converters.h>

class DataSet
{
public:
    DataSet(const QString &rootDir)
        : m_rootDir(rootDir)
    {}
    void setDataSource(const QString &dataSetName)
    {
        m_dataSetDirectory.setPath(m_rootDir.path() + "/" + dataSetName);

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
    QDir m_rootDir;
    QDir m_dataSetDirectory;
    Utils::FilePath m_qmlProjectFile;
    Utils::FilePath m_jsonToQmlProjectFile;
    Utils::FilePath m_qmlProjectToJsonFile;
};

int main(int argc, char **argv)
{
    const QString helpText{"./dataSetGenerator [path]\n"
                           "[path]:     Path to the data set folders. The default is current dir.\n"
                           "            Folder names should be in the form of test-set-x.\n"};

    QDir dataSetPath{QDir::currentPath()};
    if (argc >= 2) {
        dataSetPath.setPath(argv[1]);
    }

    if (!dataSetPath.exists()) {
        qDebug() << "Data path does not exist:" << dataSetPath.path() << Qt::endl;
        qDebug().noquote() << helpText;
        return -1;
    }

    QStringList dataSetList{dataSetPath.entryList({"test-set-*"})};
    if (!dataSetList.size()) {
        qDebug() << "No test sets are available under" << dataSetPath.path() << Qt::endl;
        qDebug().noquote() << helpText;
        return -1;
    }

    DataSet dataSet(dataSetPath.path());
    for (const auto &dataSetName : dataSetList) {
        dataSet.setDataSource(dataSetName);

        qDebug() << "Regenerating data set:" << dataSet.dataSetName();
        QJsonObject qml2json = QmlProjectManager::Converters::qmlProjectTojson(
            dataSet.qmlProjectFile());
        QString json2qml = QmlProjectManager::Converters::jsonToQmlProject(qml2json);

        dataSet.qmlProjectToJsonFile().writeFileContents(QJsonDocument(qml2json).toJson());
        dataSet.jsonToQmlProjectFile().writeFileContents(json2qml.toUtf8());
    }
    return 0;
}
