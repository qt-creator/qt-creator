// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "languageclientmanager_test.h"

#include "client.h"
#include "languageclientinterface.h"
#include "languageclientmanager.h"
#include "languageclientsettings.h"

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/buildinfo.h>
#include <projectexplorer/buildsystem.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/projectnodes.h>
#include <projectexplorer/target.h>

#include <texteditor/textdocument.h>

#include <utils/temporarydirectory.h>

#include <QJsonDocument>
#include <QJsonObject>
#include <QPointer>
#include <QTest>

#include <atomic>

using namespace Core;
using namespace LanguageServerProtocol;
using namespace ProjectExplorer;
using namespace TextEditor;
using namespace Utils;

namespace LanguageClient::Internal {

const char testProjectType[] = "LanguageClientManagerTest.Project";
const char testFilePattern[] = "*.lcmtest";

// Answers initialize and shutdown requests like a language server, and finishes on exit.
class FakeServerInterface final : public BaseClientInterface
{
public:
    // Used from the interface thread
    static inline std::atomic_bool answerShutdown = true;

    FilePath serverDeviceTemplate() const final { return {}; }

protected:
    void sendData(const QByteArray &data) final
    {
        // The header and the content arrive separately, only the content parses as JSON.
        const QJsonObject message = QJsonDocument::fromJson(data).object();
        const QString method = message.value("method").toString();
        if (method == "initialize") {
            const QJsonObject capabilities{{"textDocumentSync", 1}};
            respond(message.value("id"), QJsonObject{{"capabilities", capabilities}});
        } else if (method == "shutdown") {
            if (answerShutdown)
                respond(message.value("id"), QJsonValue::Null);
        } else if (method == "exit") {
            emit finished();
        }
    }

private:
    void respond(const QJsonValue &id, const QJsonValue &result)
    {
        emit messageReceived(QJsonObject{{"jsonrpc", "2.0"}, {"id", id}, {"result", result}});
    }
};

class TestSettings final : public BaseSettings
{
public:
    TestSettings()
    {
        name.setValue("LanguageClientManagerTest");
        id.setValue("LanguageClientManagerTest.Settings");
        filePattern.setValue(testFilePattern);
        startBehavior.setValue(RequiresProjectOrFile);
        enabled.setValue(true);
        activatable.setValue(true);
    }

    BaseSettings *create() const final { return new TestSettings; }

protected:
    BaseClientInterface *createInterface(BuildConfiguration *) const final
    {
        return new FakeServerInterface;
    }
};

class TestBuildSystem final : public BuildSystem
{
public:
    using BuildSystem::BuildSystem;

    static QString name() { return "LanguageClientManagerTest"; }
    void triggerParsing() final {}
};

class TestBuildConfigurationFactory final : public BuildConfigurationFactory
{
public:
    TestBuildConfigurationFactory()
    {
        setSupportedProjectType(testProjectType);
        setBuildGenerator([](const Kit *, const FilePath &projectFilePath, bool) {
            BuildInfo info;
            info.buildSystemName = TestBuildSystem::name();
            info.buildDirectory = projectFilePath.parentDir();
            info.displayName = "default";
            return QList<BuildInfo>{info};
        });
        registerBuildConfiguration<BuildConfiguration>(
            "LanguageClientManagerTest.BuildConfiguration");
    }
};

class TestProject final : public Project
{
public:
    explicit TestProject(const FilePath &projectFilePath)
        : Project("text/plain", projectFilePath)
    {
        setType(testProjectType);
        setDisplayName("LanguageClientManagerTest");
        setBuildSystemCreator<TestBuildSystem>();
    }

    bool needsConfiguration() const final { return false; }

    // Like a finished parse: makes the files known to the project.
    void setFiles(const FilePaths &files)
    {
        auto root = std::make_unique<ProjectNode>(projectDirectory());
        std::vector<std::unique_ptr<FileNode>> nodes;
        nodes.emplace_back(std::make_unique<FileNode>(projectFilePath(), FileType::Project));
        for (const FilePath &file : files)
            nodes.emplace_back(std::make_unique<FileNode>(file, FileType::Source));
        root->addNestedNodes(std::move(nodes));
        setRootProjectNode(std::move(root));
    }
};

class LanguageClientManagerTest final : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void cleanup();

    void testFileMovesToProjectClientWhenProjectOpens();
    void testShutdownProjectClientIsNotUsedForFileOutsideProjects();
    void testRestartingProjectLessClientIsReused();
    void testFileMovesToExistingProjectClientWhenAddedToProject();
    void testFileReturnsToProjectLessClientWhenProjectCloses();

private:
    TextDocument *openDocument(const FilePath &filePath);
    TestProject *addProject(const FilePath &projectDirectory, const FilePaths &files);
    QList<Client *> testClients() const;

    std::unique_ptr<TemporaryDirectory> m_tempDir;
    std::unique_ptr<TestBuildConfigurationFactory> m_buildConfigurationFactory;
    std::unique_ptr<Kit> m_kit;
    TestSettings *m_settings = nullptr;
    QList<IDocument *> m_documents;
    QList<QPointer<Project>> m_projects;
};

void LanguageClientManagerTest::init()
{
    m_tempDir = std::make_unique<TemporaryDirectory>("LanguageClientManagerTest");
    m_buildConfigurationFactory = std::make_unique<TestBuildConfigurationFactory>();
    m_kit = std::make_unique<Kit>();
    FakeServerInterface::answerShutdown = true;

    // Bypass the settings page, which would write the setting to the user's settings.
    m_settings = new TestSettings;
    LanguageClientManager::instance()->m_currentSettings.append(m_settings);
}

void LanguageClientManagerTest::cleanup()
{
    FakeServerInterface::answerShutdown = true;
    EditorManager::closeDocuments(m_documents, false);
    m_documents.clear();
    for (const QPointer<Project> &project : std::as_const(m_projects)) {
        if (project)
            ProjectManager::removeProject(project);
    }
    m_projects.clear();

    LanguageClientManager::instance()->m_currentSettings.removeOne(m_settings);
    QList<QPointer<Client>> clients;
    for (Client *client : testClients()) {
        clients << client;
        // Nothing will answer a shutdown request that was sent while answerShutdown was off.
        if (client->state() == Client::ShutdownRequested)
            LanguageClientManager::deleteClient(client);
        else
            LanguageClientManager::shutdownClient(client);
    }
    QTRY_VERIFY(Utils::allOf(clients, [](const QPointer<Client> &client) { return !client; }));
    delete m_settings;
    m_settings = nullptr;

    m_kit.reset();
    m_buildConfigurationFactory.reset();
    m_tempDir.reset();
}

TextDocument *LanguageClientManagerTest::openDocument(const FilePath &filePath)
{
    if (!filePath.parentDir().ensureWritableDir() || !filePath.writeFileContents("content"))
        return nullptr;
    IEditor *editor = EditorManager::openEditor(filePath);
    if (!editor)
        return nullptr;
    m_documents << editor->document();
    return qobject_cast<TextDocument *>(editor->document());
}

TestProject *LanguageClientManagerTest::addProject(
    const FilePath &projectDirectory, const FilePaths &files)
{
    const FilePath projectFile = projectDirectory.pathAppended("test.lcmproject");
    if (!projectDirectory.ensureWritableDir() || !projectFile.writeFileContents({}))
        return nullptr;
    auto project = new TestProject(projectFile);
    ProjectManager::addProject(project);
    m_projects << project;
    // Adding the build configuration connects the language client manager to the file list.
    if (!project->addTargetForKit(m_kit.get()))
        return nullptr;
    project->setFiles(files);
    return project;
}

QList<Client *> LanguageClientManagerTest::testClients() const
{
    return LanguageClientManager::clientsForSetting(m_settings);
}

void LanguageClientManagerTest::testFileMovesToProjectClientWhenProjectOpens()
{
    const FilePath projectDirectory = m_tempDir->path().pathAppended("project");
    const FilePath file = projectDirectory.pathAppended("file.lcmtest");

    TextDocument *document = openDocument(file);
    QVERIFY(document);
    QPointer<Client> projectLessClient = LanguageClientManager::clientForDocument(document);
    QVERIFY(projectLessClient);
    QVERIFY(!projectLessClient->buildConfiguration());
    QTRY_COMPARE(projectLessClient->state(), Client::Initialized);

    TestProject *project = addProject(projectDirectory, {file});
    QVERIFY(project);
    BuildConfiguration *bc = project->activeBuildConfiguration();
    QVERIFY(bc);

    QTRY_VERIFY(LanguageClientManager::clientForDocument(document)
                && LanguageClientManager::clientForDocument(document)->buildConfiguration() == bc);
    QTRY_COMPARE(LanguageClientManager::clientForDocument(document)->state(), Client::Initialized);
}

void LanguageClientManagerTest::testShutdownProjectClientIsNotUsedForFileOutsideProjects()
{
    const FilePath projectDirectory = m_tempDir->path().pathAppended("project");
    const FilePath projectFile = projectDirectory.pathAppended("file.lcmtest");
    const FilePath looseFile = m_tempDir->path().pathAppended("loose/file.lcmtest");

    TestProject *project = addProject(projectDirectory, {projectFile});
    QVERIFY(project);
    TextDocument *projectDocument = openDocument(projectFile);
    QVERIFY(projectDocument);
    QPointer<Client> projectClient = LanguageClientManager::clientForDocument(projectDocument);
    QVERIFY(projectClient);
    QCOMPARE(projectClient->buildConfiguration(), project->activeBuildConfiguration());
    QTRY_COMPARE(projectClient->state(), Client::Initialized);

    // Keep the project client in ShutdownRequested, as for a server that takes time to exit.
    FakeServerInterface::answerShutdown = false;
    ProjectManager::removeProject(project);
    QVERIFY(projectClient);
    QCOMPARE(projectClient->state(), Client::ShutdownRequested);
    QVERIFY(!projectClient->buildConfiguration());

    TextDocument *looseDocument = openDocument(looseFile);
    QVERIFY(looseDocument);
    Client *looseClient = LanguageClientManager::clientForDocument(looseDocument);
    QVERIFY(looseClient);
    QVERIFY(looseClient != projectClient);
    QVERIFY(!looseClient->buildConfiguration());
    QTRY_COMPARE(looseClient->state(), Client::Initialized);
}

void LanguageClientManagerTest::testRestartingProjectLessClientIsReused()
{
    const FilePath firstFile = m_tempDir->path().pathAppended("loose/first.lcmtest");
    const FilePath secondFile = m_tempDir->path().pathAppended("loose/second.lcmtest");

    TextDocument *firstDocument = openDocument(firstFile);
    QVERIFY(firstDocument);
    QPointer<Client> projectLessClient = LanguageClientManager::clientForDocument(firstDocument);
    QVERIFY(projectLessClient);
    QTRY_COMPARE(projectLessClient->state(), Client::Initialized);

    // Keep the client in ShutdownRequested, as for a server that takes time to exit.
    FakeServerInterface::answerShutdown = false;
    LanguageClientManager::restartClient(projectLessClient);
    QCOMPARE(projectLessClient->state(), Client::ShutdownRequested);

    TextDocument *secondDocument = openDocument(secondFile);
    QVERIFY(secondDocument);
    QCOMPARE(testClients(), QList<Client *>{projectLessClient.data()});
    QCOMPARE(LanguageClientManager::clientForDocument(secondDocument), projectLessClient.data());
}

void LanguageClientManagerTest::testFileMovesToExistingProjectClientWhenAddedToProject()
{
    const FilePath projectDirectory = m_tempDir->path().pathAppended("project");
    const FilePath projectFile = projectDirectory.pathAppended("file.lcmtest");
    // Outside of the project directory, so it is not part of the project until it is listed.
    const FilePath sharedFile = m_tempDir->path().pathAppended("shared/file.lcmtest");

    TestProject *project = addProject(projectDirectory, {projectFile});
    QVERIFY(project);
    TextDocument *projectDocument = openDocument(projectFile);
    QVERIFY(projectDocument);
    QPointer<Client> projectClient = LanguageClientManager::clientForDocument(projectDocument);
    QVERIFY(projectClient);
    QCOMPARE(projectClient->buildConfiguration(), project->activeBuildConfiguration());
    QTRY_COMPARE(projectClient->state(), Client::Initialized);

    TextDocument *sharedDocument = openDocument(sharedFile);
    QVERIFY(sharedDocument);
    QPointer<Client> projectLessClient = LanguageClientManager::clientForDocument(sharedDocument);
    QVERIFY(projectLessClient);
    QVERIFY(!projectLessClient->buildConfiguration());
    QTRY_COMPARE(projectLessClient->state(), Client::Initialized);

    project->setFiles({projectFile, sharedFile});

    QTRY_COMPARE(LanguageClientManager::clientForDocument(sharedDocument), projectClient.data());
    QCOMPARE(LanguageClientManager::clientForDocument(projectDocument), projectClient.data());
}

void LanguageClientManagerTest::testFileReturnsToProjectLessClientWhenProjectCloses()
{
    const FilePath projectDirectory = m_tempDir->path().pathAppended("project");
    const FilePath file = projectDirectory.pathAppended("file.lcmtest");

    TextDocument *document = openDocument(file);
    QVERIFY(document);
    QPointer<Client> projectLessClient = LanguageClientManager::clientForDocument(document);
    QVERIFY(projectLessClient);
    QVERIFY(!projectLessClient->buildConfiguration());
    QTRY_COMPARE(projectLessClient->state(), Client::Initialized);

    TestProject *project = addProject(projectDirectory, {file});
    QVERIFY(project);
    QTRY_VERIFY(LanguageClientManager::clientForDocument(document)
                && LanguageClientManager::clientForDocument(document) != projectLessClient);
    QPointer<Client> projectClient = LanguageClientManager::clientForDocument(document);
    QTRY_COMPARE(projectClient->state(), Client::Initialized);

    ProjectManager::removeProject(project);

    QTRY_COMPARE(LanguageClientManager::clientForDocument(document), projectLessClient.data());
    QTRY_VERIFY(!projectClient);
    QCOMPARE(testClients(), QList<Client *>{projectLessClient.data()});
}

QObject *createLanguageClientManagerTest()
{
    return new LanguageClientManagerTest;
}

} // namespace LanguageClient::Internal

#include "languageclientmanager_test.moc"
