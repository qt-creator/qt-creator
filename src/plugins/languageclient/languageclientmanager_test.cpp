// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "languageclientmanager_test.h"

#ifdef WITH_TESTS

#include "client.h"
#include "languageclientinterface.h"
#include "languageclientmanager.h"
#include "languageclientsettings.h"

#include <coreplugin/editormanager/editormanager.h>

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/buildinfo.h>
#include <projectexplorer/buildsystem.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/projectnodes.h>
#include <projectexplorer/target.h>

#include <texteditor/textdocument.h>

#include <utils/algorithm.h>
#include <utils/filepath.h>
#include <utils/temporarydirectory.h>

#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QTest>
#include <QTimer>

#include <atomic>

using namespace ProjectExplorer;
using namespace LanguageServerProtocol;

namespace LanguageClient {

static const char fakeProjectType[] = "LanguageClientManagerTest.Project";
static const char fakeBuildConfigurationType[] = "LanguageClientManagerTest.BuildConfiguration";

class FakeServerInterface final : public BaseClientInterface
{
public:
    FakeServerInterface() { instances << this; }

    ~FakeServerInterface() override { instances.removeOne(this); }

    Utils::FilePath serverDeviceTemplate() const final { return {}; }

    void releaseShutdown()
    {
        if (!m_pendingShutdownId.isUndefined())
            replyToShutdown();
    }

    static inline QList<FakeServerInterface *> instances;
    static inline std::atomic_bool holdShutdown = false;

protected:
    void sendData(const QByteArray &data) final
    {
        m_received += data;
        static const QRegularExpression contentLength("Content-Length: (\\d+)");
        while (true) {
            const qsizetype headerEnd = m_received.indexOf("\r\n\r\n");
            if (headerEnd < 0)
                return;
            const QRegularExpressionMatch match = contentLength.match(
                QString::fromLatin1(m_received.left(headerEnd)));
            const qsizetype bodyStart = headerEnd + 4;
            const qsizetype bodyEnd = bodyStart + match.captured(1).toInt();
            if (!match.hasMatch() || m_received.size() < bodyEnd)
                return;
            const QJsonObject message
                = QJsonDocument::fromJson(m_received.mid(bodyStart, bodyEnd - bodyStart)).object();
            m_received.remove(0, bodyEnd);
            handleMessage(message);
        }
    }

private:
    void handleMessage(const QJsonObject &message)
    {
        const QString method = message.value("method").toString();
        if (method == "initialize") {
            reply(message.value("id"), QJsonObject{{"capabilities", QJsonObject()}});
        } else if (method == "shutdown") {
            m_pendingShutdownId = message.value("id");
            if (!holdShutdown)
                replyToShutdown();
        } else if (method == "exit") {
            QTimer::singleShot(0, this, [this] { emit finished(); });
        }
    }

    void replyToShutdown()
    {
        reply(m_pendingShutdownId, QJsonValue::Null);
        m_pendingShutdownId = QJsonValue::Undefined;
    }

    void reply(const QJsonValue &id, const QJsonValue &result)
    {
        const QJsonObject response{{"jsonrpc", "2.0"}, {"id", id}, {"result", result}};
        QTimer::singleShot(0, this, [this, response] {
            emit messageReceived(JsonRpcMessage(response));
        });
    }

    QByteArray m_received;
    QJsonValue m_pendingShutdownId = QJsonValue::Undefined;
};

class FakeServerSettings final : public BaseSettings
{
public:
    FakeServerSettings()
    {
        id.setValue("LanguageClientManagerTest.Server");
        name.setValue("LanguageClientManagerTest server");
        mimeTypes.setValue({"text/plain"});
        startBehavior.setValue(RequiresProject);
        enabled.setValue(true);
    }

    BaseSettings *create() const final { return new FakeServerSettings; }

protected:
    BaseClientInterface *createInterface(BuildConfiguration *) const final
    {
        return new FakeServerInterface;
    }
};

class FakeBuildSystem final : public BuildSystem
{
public:
    using BuildSystem::BuildSystem;

    static QString name() { return "fake"; }
    void triggerParsing() final {}
};

class FakeBuildConfigurationFactory final : public BuildConfigurationFactory
{
public:
    FakeBuildConfigurationFactory()
    {
        setSupportedProjectType(fakeProjectType);
        setBuildGenerator([](const Kit *, const Utils::FilePath &projectFilePath, bool) {
            QList<BuildInfo> infos;
            for (const QString &displayName : {QString("first"), QString("second")}) {
                BuildInfo info;
                info.buildSystemName = FakeBuildSystem::name();
                info.buildDirectory = projectFilePath.parentDir();
                info.displayName = displayName;
                infos << info;
            }
            return infos;
        });
        registerBuildConfiguration<BuildConfiguration>(fakeBuildConfigurationType);
    }
};

class FakeProject final : public Project
{
public:
    FakeProject(const Utils::FilePath &projectFilePath, Kit *kit)
        : Project("application/vnd.test.languageclientmanager", projectFilePath)
    {
        setType(fakeProjectType);
        setDisplayName("languageClientManagerTestProject");
        setBuildSystemCreator<FakeBuildSystem>();
        target = addTargetForKit(kit);
    }

    bool needsConfiguration() const final { return false; }

    Target *target = nullptr;
};

static bool isLive(const Client *client)
{
    const Client::State state = client->state();
    return state == Client::Uninitialized || state == Client::InitializeRequested
           || state == Client::Initialized;
}

class LanguageClientManagerTest final : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    void testSwitchingBuildConfigurationMovesClient();
    void testSwitchingBackBeforeOldServerExits();
    void testDocumentOpenedWhileOldServerIsShuttingDown();
    void testApplySettingsWhileClientIsRestarting();
    void testDocumentOpenedWhileClientIsRestartedInPlace();
    void testFileListChangeDoesNotStartClientForInactiveBuildConfiguration();
    void testSwitchingTargetMovesClient();

private:
    QList<Client *> clients() const
    {
        return LanguageClientManager::clientsForSettingId(m_settingId);
    }
    QList<Client *> liveClientsFor(const BuildConfiguration *bc) const
    {
        return Utils::filtered(clients(), [bc](const Client *client) {
            return client->buildConfiguration() == bc && isLive(client);
        });
    }
    void switchTo(BuildConfiguration *bc)
    {
        m_project->setActiveBuildConfiguration(bc, SetActive::Cascade);
    }
    TextEditor::TextDocument *openFile(const QString &fileName)
    {
        const Utils::FilePath file = m_directory->path() / fileName;
        if (!file.writeFileContents("int main() {}\n"))
            return nullptr;
        Core::IEditor *editor = Core::EditorManager::openEditor(file);
        return editor ? qobject_cast<TextEditor::TextDocument *>(editor->document()) : nullptr;
    }
    void announceFile(const QString &fileName)
    {
        auto root = std::make_unique<ProjectNode>(m_directory->path());
        root->addNode(std::make_unique<FileNode>(m_directory->path() / fileName, FileType::Source));
        m_project->setRootProjectNode(std::move(root));
    }
    void releaseAllServers()
    {
        FakeServerInterface::holdShutdown = false;
        const QList<FakeServerInterface *> servers = FakeServerInterface::instances;
        for (FakeServerInterface *server : servers)
            QMetaObject::invokeMethod(server, &FakeServerInterface::releaseShutdown);
    }

    std::unique_ptr<FakeBuildConfigurationFactory> m_factory;
    std::unique_ptr<Kit> m_kit;
    std::unique_ptr<Kit> m_otherKit;
    std::unique_ptr<Utils::TemporaryDirectory> m_directory;
    QString m_settingId;
    FakeProject *m_project = nullptr;
    BuildConfiguration *m_firstBc = nullptr;
    BuildConfiguration *m_secondBc = nullptr;
    TextEditor::TextDocument *m_document = nullptr;
};

void LanguageClientManagerTest::initTestCase()
{
    m_factory = std::make_unique<FakeBuildConfigurationFactory>();
    m_kit = std::make_unique<Kit>();
    m_otherKit = std::make_unique<Kit>();

    auto *settings = new FakeServerSettings;
    m_settingId = settings->id();
    LanguageClientManager::registerClientSettings(settings);
}

void LanguageClientManagerTest::cleanupTestCase()
{
    LanguageClientManager::removeClientSettings(m_settingId);
    QVERIFY(
        !LanguageClientSettings::settingById(LanguageClientSettings::pageSettings(), m_settingId));
    m_otherKit.reset();
    m_kit.reset();
    m_factory.reset();
}

void LanguageClientManagerTest::init()
{
    FakeServerInterface::holdShutdown = false;
    m_directory = std::make_unique<Utils::TemporaryDirectory>("languageclientmanagertest-XXXXXX");

    m_project = new FakeProject(m_directory->path() / "test.project", m_kit.get());
    QVERIFY(m_project->target);
    const QList<BuildConfiguration *> buildConfigurations = m_project->target->buildConfigurations();
    QCOMPARE(buildConfigurations.size(), 2);
    m_firstBc = buildConfigurations.at(0);
    m_secondBc = buildConfigurations.at(1);
    QCOMPARE(m_project->activeBuildConfiguration(), m_firstBc);
    ProjectManager::addProject(m_project);

    m_document = openFile("main.txt");
    QVERIFY(m_document);
}

void LanguageClientManagerTest::cleanup()
{
    releaseAllServers();
    Core::EditorManager::closeAllDocuments();
    m_document = nullptr;

    for (Client *client : clients()) {
        if (client->state() != Client::ShutdownRequested && client->state() != Client::Shutdown)
            LanguageClientManager::shutdownClient(client);
    }
    QTRY_VERIFY(clients().isEmpty());

    ProjectManager::removeProject(m_project);
    m_project = nullptr;
    m_firstBc = nullptr;
    m_secondBc = nullptr;
    m_directory.reset();
}

void LanguageClientManagerTest::testSwitchingBuildConfigurationMovesClient()
{
    QTRY_COMPARE(liveClientsFor(m_firstBc).size(), 1);
    QPointer<Client> firstClient = liveClientsFor(m_firstBc).first();
    QCOMPARE(clients().size(), 1);
    QTRY_COMPARE(firstClient->state(), Client::Initialized);
    QCOMPARE(LanguageClientManager::clientForDocument(m_document), firstClient.data());

    switchTo(m_secondBc);

    QTRY_COMPARE(clients().size(), 1);
    QPointer<Client> secondClient = clients().first();
    QCOMPARE(secondClient->buildConfiguration(), m_secondBc);
    QVERIFY(secondClient != firstClient);
    QTRY_VERIFY(firstClient.isNull());
    QTRY_COMPARE(secondClient->state(), Client::Initialized);
    QCOMPARE(LanguageClientManager::clientForDocument(m_document), secondClient.data());
}

void LanguageClientManagerTest::testSwitchingBackBeforeOldServerExits()
{
    QTRY_COMPARE(liveClientsFor(m_firstBc).size(), 1);
    QPointer<Client> oldClient = liveClientsFor(m_firstBc).first();
    QTRY_COMPARE(oldClient->state(), Client::Initialized);

    FakeServerInterface::holdShutdown = true;

    switchTo(m_secondBc);
    QTRY_COMPARE(oldClient->state(), Client::ShutdownRequested);
    QTRY_COMPARE(liveClientsFor(m_secondBc).size(), 1);
    QPointer<Client> interimClient = liveClientsFor(m_secondBc).first();
    QTRY_COMPARE(interimClient->state(), Client::Initialized);

    switchTo(m_firstBc);

    QTRY_COMPARE(liveClientsFor(m_firstBc).size(), 1);
    QPointer<Client> newClient = liveClientsFor(m_firstBc).first();
    QVERIFY(newClient != oldClient);
    QCOMPARE(oldClient->state(), Client::ShutdownRequested);

    releaseAllServers();

    QTRY_VERIFY(oldClient.isNull());
    QTRY_VERIFY(interimClient.isNull());
    QCOMPARE(clients().size(), 1);
    QCOMPARE(clients().first(), newClient.data());
    QTRY_COMPARE(newClient->state(), Client::Initialized);
    QCOMPARE(LanguageClientManager::clientForDocument(m_document), newClient.data());
}

void LanguageClientManagerTest::testDocumentOpenedWhileOldServerIsShuttingDown()
{
    QTRY_COMPARE(liveClientsFor(m_firstBc).size(), 1);
    QPointer<Client> oldClient = liveClientsFor(m_firstBc).first();
    QTRY_COMPARE(oldClient->state(), Client::Initialized);

    FakeServerInterface::holdShutdown = true;
    switchTo(m_secondBc);
    QTRY_COMPARE(oldClient->state(), Client::ShutdownRequested);
    QTRY_COMPARE(liveClientsFor(m_secondBc).size(), 1);
    switchTo(m_firstBc);
    QTRY_COMPARE(liveClientsFor(m_firstBc).size(), 1);
    QPointer<Client> newClient = liveClientsFor(m_firstBc).first();

    TextEditor::TextDocument *secondDocument = openFile("second.txt");
    QVERIFY(secondDocument);

    QTRY_COMPARE(LanguageClientManager::clientForDocument(secondDocument), newClient.data());
    QCOMPARE(oldClient->state(), Client::ShutdownRequested);

    releaseAllServers();

    QTRY_VERIFY(oldClient.isNull());
    QTRY_COMPARE(newClient->state(), Client::Initialized);
    QCOMPARE(LanguageClientManager::clientForDocument(secondDocument), newClient.data());
    QCOMPARE(LanguageClientManager::clientForDocument(m_document), newClient.data());
}

void LanguageClientManagerTest::testApplySettingsWhileClientIsRestarting()
{
    QTRY_COMPARE(liveClientsFor(m_firstBc).size(), 1);
    QPointer<Client> oldClient = liveClientsFor(m_firstBc).first();
    QTRY_COMPARE(oldClient->state(), Client::Initialized);

    FakeServerInterface::holdShutdown = true;
    LanguageClientManager::applySettings(m_settingId);
    QTRY_COMPARE(oldClient->state(), Client::ShutdownRequested);
    QCOMPARE(liveClientsFor(m_firstBc).size(), 1);
    QPointer<Client> restartedClient = liveClientsFor(m_firstBc).first();
    QVERIFY(restartedClient != oldClient);
    QTRY_COMPARE(restartedClient->state(), Client::Initialized);

    LanguageClientManager::applySettings(m_settingId);

    QTRY_COMPARE(restartedClient->state(), Client::ShutdownRequested);
    QCOMPARE(liveClientsFor(m_firstBc).size(), 1);
    QPointer<Client> latestClient = liveClientsFor(m_firstBc).first();
    QVERIFY(latestClient != restartedClient);

    releaseAllServers();

    QTRY_VERIFY(oldClient.isNull());
    QTRY_VERIFY(restartedClient.isNull());
    QCOMPARE(clients().size(), 1);
    QCOMPARE(clients().first(), latestClient.data());
    QTRY_COMPARE(latestClient->state(), Client::Initialized);
    QCOMPARE(LanguageClientManager::clientForDocument(m_document), latestClient.data());
}

void LanguageClientManagerTest::testDocumentOpenedWhileClientIsRestartedInPlace()
{
    QTRY_COMPARE(liveClientsFor(m_firstBc).size(), 1);
    QPointer<Client> client = liveClientsFor(m_firstBc).first();
    QTRY_COMPARE(client->state(), Client::Initialized);

    FakeServerInterface::holdShutdown = true;
    LanguageClientManager::restartClient(client);
    QTRY_COMPARE(client->state(), Client::ShutdownRequested);

    TextEditor::TextDocument *secondDocument = openFile("second.txt");
    QVERIFY(secondDocument);

    QCOMPARE(clients().size(), 1);
    QCOMPARE(clients().first(), client.data());

    releaseAllServers();

    QTRY_COMPARE(client->state(), Client::Initialized);
    QCOMPARE(clients().size(), 1);
    QCOMPARE(clients().first(), client.data());
    QCOMPARE(LanguageClientManager::clientForDocument(m_document), client.data());
    QTRY_COMPARE(LanguageClientManager::clientForDocument(secondDocument), client.data());
}

void LanguageClientManagerTest::testFileListChangeDoesNotStartClientForInactiveBuildConfiguration()
{
    QTRY_COMPARE(liveClientsFor(m_firstBc).size(), 1);
    QPointer<Client> firstClient = liveClientsFor(m_firstBc).first();
    QTRY_COMPARE(firstClient->state(), Client::Initialized);

    announceFile("main.txt");

    QCOMPARE(liveClientsFor(m_secondBc).size(), 0);
    QCOMPARE(clients().size(), 1);
    QCOMPARE(clients().first(), firstClient.data());
}

void LanguageClientManagerTest::testSwitchingTargetMovesClient()
{
    QTRY_COMPARE(liveClientsFor(m_firstBc).size(), 1);
    QPointer<Client> firstClient = liveClientsFor(m_firstBc).first();
    QTRY_COMPARE(firstClient->state(), Client::Initialized);

    Target *otherTarget = m_project->addTargetForKit(m_otherKit.get());
    QVERIFY(otherTarget);
    BuildConfiguration *otherBc = otherTarget->buildConfigurations().first();
    QVERIFY(otherBc);
    QCOMPARE(m_project->activeBuildConfiguration(), m_firstBc);
    QCOMPARE(liveClientsFor(m_firstBc).size(), 1);

    m_project->setActiveTarget(otherTarget, SetActive::NoCascade);
    QCOMPARE(m_project->activeBuildConfiguration(), otherBc);

    QTRY_COMPARE(clients().size(), 1);
    QPointer<Client> otherClient = clients().first();
    QCOMPARE(otherClient->buildConfiguration(), otherBc);
    QTRY_VERIFY(firstClient.isNull());
    QTRY_COMPARE(otherClient->state(), Client::Initialized);
    QCOMPARE(LanguageClientManager::clientForDocument(m_document), otherClient.data());

    announceFile("main.txt");

    QCOMPARE(liveClientsFor(m_firstBc).size(), 0);
    QCOMPARE(liveClientsFor(m_secondBc).size(), 0);
    QCOMPARE(clients().size(), 1);
}

QObject *createLanguageClientManagerTest()
{
    return new LanguageClientManagerTest;
}

} // namespace LanguageClient

#include "languageclientmanager_test.moc"

#endif // WITH_TESTS
