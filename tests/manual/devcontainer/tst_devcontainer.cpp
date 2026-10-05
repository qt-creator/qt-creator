// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "devcontainer/devcontainerfeature.h"
#include <devcontainer/devcontainer.h>
#include <devcontainer/devcontainerconfig.h>

#include <utils/hostosinfo.h>
#include <utils/qtcprocess.h>
#include <utils/stringutils.h>

#include <QCryptographicHash>
#include <QJsonArray>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QRegularExpression>
#include <QScopeGuard>
#include <QSignalSpy>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTemporaryFile>
#include <QTest>
#include <QUrlQuery>

#ifdef __GNUC__
// We are making use of named initializers a lot here, and GCC complains if we do not initialize all fields.
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#endif

using namespace Utils;
using namespace QtTaskTree;

constexpr auto recipeTimeout = std::chrono::minutes(60); // std::chrono::seconds(5);

static bool testDocker(const FilePath &executable)
{
    Process p;
    p.setCommand({executable, {"info", "--format", "{{.OSType}}"}});
    p.runBlocking();
    const QString platform = p.cleanedStdOut().trimmed();
    return p.result() == ProcessResult::FinishedWithSuccess && platform == "linux";
}

static bool testDockerMount(const FilePath &executable, const FilePath &testDir)
{
    Process p;
    p.setCommand(
        {executable,
         {"run",
          "--rm",
          "--mount",
          "type=bind,source=" + testDir.path() + ",target=/mnt/test",
          "alpine:latest",
          "ls",
          "/mnt/test"}});
    p.runBlocking();
    if (p.result() != ProcessResult::FinishedWithSuccess) {
        qWarning() << "Docker mount test failed:" << p.verboseExitMessage();
        return false;
    }
    return p.result() == ProcessResult::FinishedWithSuccess;
}

class tst_DevContainer : public QObject
{
    Q_OBJECT

    const FilePath tempDir = FilePath::fromString(QDir::tempPath()) / "tst_DevContainer";

    QString logMessages;

    std::function<void(const QString &)> logFunction = [this](const QString &msg) {
        logMessages += msg + '\n';
    };

private slots:
    void initTestCase()
    {
        QTC_ASSERT_RESULT(
            tempDir.ensureWritableDir(), QSKIP("Failed to create temp directory for tests."));

        (tempDir / "main.cpp").writeFileContents(R"(
#include <iostream>
int main() {
    std::cout << "Hello, DevContainer!" << std::endl;
    return 0;
})");

        if (!testDocker("docker"))
            QSKIP("Docker is not set up correctly, skipping tests.");

        if (!testDockerMount("docker", tempDir))
            QSKIP("Docker mount test failed, skipping tests.");
    }

    void init() { logMessages.clear(); }

    void cleanup()
    {
        if (QTest::currentTestFailed())
            qWarning().noquote() << "Log:\n\n" << logMessages;
    }

    void parseUserFromPasswd_data();
    void parseUserFromPasswd();
    void dockerCompose();
    void processInterface();
    void instanceConfigToString_data();
    void instanceConfigToString();
    void readConfig();
    void testCommands();
    void upWithHooks();
    void upImage();
    void upDockerfile();
    void containerWorkspaceReplacers();
    void upImageWithLocalFeatures();
    void upDockerfileWithFeature();
    void upComposeWithFeature();
    void upWithOciFeature();
    void upWithMissingFeature();
    void upCustomizationsFromFeature();
    void lifecycleHooksOnce();
    void updateRemoteUserUid_data();
    void updateRemoteUserUid();
    void privateRegistryFeature();
    void downRemovesFeaturesImage();
    void composeRebuildsDependencies();
    void signalAsRemoteUser();
    void emptyRemoteUser();
    void verifyManifest_data();
    void verifyManifest();
    void blobRedirectsWithoutCredentials();
    void composeServiceEntrypoint();
    void mountConflict();
    void composeRecreatedRunsCreateHooks();
    void tokenRealmWithoutHttps();
    void identityTokenRefresh();
    void composeKeepsDollarSigns();
    void dockerHubFeature();

private:
    struct UpResult
    {
        std::unique_ptr<DevContainer::Instance> instance;
        DevContainer::RunningInstance runningInstance;
        DoneWith doneWith = DoneWith::Error;
    };
    using AdjustConfig = std::function<void(DevContainer::InstanceConfig &)>;
    UpResult up(const FilePath &configFile, const AdjustConfig &adjust = {});
    void down(const UpResult &upResult);
    QString runInContainer(const UpResult &upResult, const CommandLine &cmdLine);
};

static void writeFeature(
    const FilePath &folder, const QByteArray &featureJson, const QByteArray &installSh)
{
    QVERIFY_RESULT(folder.ensureWritableDir());
    QVERIFY_RESULT((folder / "devcontainer-feature.json").writeFileContents(featureJson));
    QVERIFY_RESULT((folder / "install.sh").writeFileContents(installSh));
}

tst_DevContainer::UpResult tst_DevContainer::up(
    const FilePath &configFile, const AdjustConfig &adjust)
{
    UpResult result;
    DevContainer::InstanceConfig instanceConfig{
        .workspaceFolder = tempDir,
        .configFilePath = configFile,
        .mounts = {},
        .logFunction = logFunction};
    if (adjust)
        adjust(instanceConfig);

    Result<std::unique_ptr<DevContainer::Instance>> instance = DevContainer::Instance::fromFile(
        instanceConfig);
    if (!instance) {
        logFunction(instance.error());
        return result;
    }
    result.instance = std::move(*instance);
    result.runningInstance = std::make_shared<DevContainer::RunningInstanceData>();

    const Result<Group> recipe = result.instance->upRecipe(result.runningInstance);
    if (!recipe) {
        logFunction(recipe.error());
        return result;
    }
    result.doneWith = QTaskTree::runBlocking((*recipe).withTimeout(recipeTimeout));
    return result;
}

void tst_DevContainer::down(const UpResult &upResult)
{
    if (!upResult.instance)
        return;
    const Result<Group> downRecipe = upResult.instance->downRecipe(true);
    if (!downRecipe) {
        qWarning() << "Cannot create the down recipe:" << downRecipe.error();
        return;
    }
    if (QTaskTree::runBlocking((*downRecipe).withTimeout(recipeTimeout)) != DoneWith::Success)
        qWarning().noquote() << "Shutting down the container failed:\n" << logMessages;

    Process rmi;
    rmi.setCommand({"docker", QStringList{"rmi", "-f"} + upResult.instance->imageNames()});
    rmi.runBlocking();
}

QString tst_DevContainer::runInContainer(const UpResult &upResult, const CommandLine &cmdLine)
{
    Process process;
    process.setProcessInterfaceCreator(
        [&] { return upResult.instance->createProcessInterface(upResult.runningInstance); });
    process.setCommand(cmdLine);
    process.runBlocking(std::chrono::seconds(30));
    logFunction(QString("%1 -> %2 %3")
                    .arg(
                        cmdLine.toUserOutput(),
                        process.cleanedStdOut().trimmed(),
                        process.cleanedStdErr().trimmed()));
    return process.cleanedStdOut().trimmed();
}

void tst_DevContainer::parseUserFromPasswd_data()
{
    QTest::addColumn<QString>("passwdLine");
    QTest::addColumn<DevContainer::UserFromPasswd>("expectedUser");

    QTest::newRow("root") << "root:x:0:0:root:/root:/bin/sh"
                          << DevContainer::UserFromPasswd{"root", "0", "0", "/root", "/bin/sh"};

    QTest::newRow("macuser")
        << R"(_swtransparencyd:*:303:303:Software Transparency Services:/var/db/swtransparencyd:/usr/bin/false)"
        << DevContainer::UserFromPasswd{
               "_swtransparencyd", "303", "303", "/var/db/swtransparencyd", "/usr/bin/false"};

    QTest::newRow("macroot") << R"(root:*:0:0:System Administrator:/var/root:/bin/sh)"
                             << DevContainer::UserFromPasswd{"root", "0", "0", "/var/root", "/bin/sh"};

    QTest::newRow("rtkit") << R"(rtkit:x:120:125:RealtimeKit,,,:/proc:/usr/sbin/nologin)"
                           << DevContainer::UserFromPasswd{
                                  "rtkit", "120", "125", "/proc", "/usr/sbin/nologin"};

    QTest::newRow("umlautuser")
        << R"(mürta:x:1002:1002:Müggelmann,443,+49172423222,,Ööööhhh:/home/mürta:/bin/bash)"
        << DevContainer::UserFromPasswd{"mürta", "1002", "1002", "/home/mürta", "/bin/bash"};

    QTest::newRow("wsl")
        << R"(systemd-timesync:x:103:106:systemd Time Synchronization,,,:/run/systemd:/usr/sbin/nologin)"
        << DevContainer::UserFromPasswd{
               "systemd-timesync", "103", "106", "/run/systemd", "/usr/sbin/nologin"};
}

void tst_DevContainer::parseUserFromPasswd()
{
    QFETCH(QString, passwdLine);
    QFETCH(DevContainer::UserFromPasswd, expectedUser);

    const auto res = DevContainer::parseUserFromPasswd(passwdLine);
    QVERIFY(res);
    QCOMPARE(res->name, expectedUser.name);
    QCOMPARE(res->uid, expectedUser.uid);
    QCOMPARE(res->gid, expectedUser.gid);
    QCOMPARE(res->home, expectedUser.home);
    QCOMPARE(res->shell, expectedUser.shell);
}

void tst_DevContainer::instanceConfigToString_data()
{
    QTest::addColumn<DevContainer::InstanceConfig>("instanceConfig");
    QTest::addColumn<QString>("input");
    QTest::addColumn<QString>("expectedOutput");

    DevContainer::InstanceConfig instanceConfig{
        .workspaceFolder = tempDir,
        .configFilePath = tempDir / "devcontainer.json",
        .mounts = {},
        .logFunction = logFunction};

    QTest::newRow("default") << instanceConfig << "Hello ${localWorkspaceFolder}"
                             << QString("Hello %1")
                                    .arg(instanceConfig.workspaceFolder.toUrlishString());
    QTest::newRow("workspaceFolderBasename")
        << instanceConfig << "Hello ${localWorkspaceFolderBasename}"
        << QString("Hello %1").arg(instanceConfig.workspaceFolder.fileName());
    QTest::newRow("devcontainerId") << instanceConfig << "Hello ${devcontainerId}"
                                    << QString("Hello %1").arg(instanceConfig.devContainerId());
    QTest::newRow("localEnvPath") << instanceConfig << "Hello ${localEnv:PATH}"
                                  << QString("Hello %1")
                                         .arg(instanceConfig.localEnvironment.value_or("PATH", ""));
    QTest::newRow("localEnvPathDefault")
        << instanceConfig << "Hello ${localEnv:PATH:default}"
        << QString("Hello %1").arg(instanceConfig.localEnvironment.value_or("PATH", "default"));
    QTest::newRow("localEnvNonExistent")
        << instanceConfig << "Hello ${localEnv:NON_EXISTENT_ENV_VAR}"
        << QString("Hello %1")
               .arg(instanceConfig.localEnvironment.value_or("NON_EXISTENT_ENV_VAR", ""));
    QTest::newRow("localEnvNonExistentDefault")
        << instanceConfig << "Hello ${localEnv:NON_EXISTENT_ENV_VAR:default}"
        << QString("Hello %1")
               .arg(instanceConfig.localEnvironment.value_or("NON_EXISTENT_ENV_VAR", "default"));
    QTest::newRow("localEnvNonExistentDefaultExtra")
        << instanceConfig << "Hello ${localEnv:NON_EXISTENT_ENV_VAR:default:extra}"
        << QString("Hello %1")
               .arg(instanceConfig.localEnvironment
                        .value_or("NON_EXISTENT_ENV_VAR", "default:extra"));
    QTest::newRow("invalid-variable")
        << instanceConfig << "Hello ${invalidVariable}"
        << QString("Hello ${invalidVariable}"); // Should not change, as the variable is invalid
}

void tst_DevContainer::instanceConfigToString()
{
    QFETCH(DevContainer::InstanceConfig, instanceConfig);
    QFETCH(QString, input);
    QFETCH(QString, expectedOutput);

    QString output = instanceConfig.jsonToString(QJsonValue::fromVariant(input));
    QCOMPARE(output, expectedOutput);
}

void tst_DevContainer::readConfig()
{
    static const QByteArray jsonData
        = R"json(// For format details, see https://aka.ms/devcontainer.json. For config options, see the
// README at: https://github.com/devcontainers/templates/tree/main/src/alpine
{
    "name": "Minimum spec container (x86_64)",
    // Or use a Dockerfile or Docker Compose file. More info: https://containers.dev/guide/dockerfile
    "build": {
        "dockerfile": "Dockerfile",
        "options": [
            "--platform=linux/amd64"
        ]
    },
    "customizations": {
        "vscode": {
            "extensions": [
                "ms-vscode.cmake-tools",
                "theqtcompany.qt"
            ],
            "settings": {
                "qt-core.additionalQtPaths": [
                    "/6.7.0/gcc_64/bin/qtpaths"
                ],
                "qt-core.qtInstallationRoot": ""
            }
        }
    },
    "shutdownAction": "none",
    // Features to add to the dev container. More info: https://containers.dev/features.
    // "features": {},
    // Use 'forwardPorts' to make a list of ports inside the container available locally.
    // "forwardPorts": [],
    // Use 'postCreateCommand' to run commands after the container is created.
    // "postCreateCommand": "uname -a",
    // Configure tool-specific properties.
    // "customizations": {},
    // Uncomment to connect as root instead. More info: https://aka.ms/dev-containers-non-root.
    // "remoteUser": "root"
    "initializeCommand": "echo 'Local Workspace Folder: ${localWorkspaceFolder}'",
    "onCreateCommand": "echo 'My container id is: ${devcontainerId}'",
    "postCreateCommand": "echo 'Your PATH is: ${localEnv:PATH}'"
}
    )json";

    DevContainer::InstanceConfig instanceConfig{
        .workspaceFolder = tempDir,
        .configFilePath = tempDir / "devcontainer.json",
        .mounts = {},
        .logFunction = logFunction};

    const Result<DevContainer::Config> devContainer
        = DevContainer::Config::fromJson(jsonData, [instanceConfig](const QJsonValue &value) {
              return instanceConfig.jsonToString(value);
          });

    QVERIFY_RESULT(devContainer);
    QVERIFY(devContainer->common.name);
    QCOMPARE(*devContainer->common.name, "Minimum spec container (x86_64)");
    QVERIFY(devContainer->containerConfig);
    QCOMPARE(devContainer->containerConfig->index(), 0);
}

void tst_DevContainer::testCommands()
{
    static const QByteArray jsonData = R"(
    {
        "initializeCommand": "echo hello",
        "onCreateCommand": ["echo", "world"],
        "updateContentCommand": {
            "echo": "echo test",
            "ls": ["ls", "-lach"]
        }
    })";

    const Result<DevContainer::Config> devContainer
        = DevContainer::Config::fromJson(jsonData, [](const QJsonValue &value) {
              return value.toString();
          });
    QVERIFY_RESULT(devContainer);
    QCOMPARE(devContainer->common.initializeCommand->index(), 0);
    QCOMPARE(std::get<QString>(*devContainer->common.initializeCommand), "echo hello");
    QCOMPARE(devContainer->common.onCreateCommand->index(), 1);
    QCOMPARE(
        std::get<QStringList>(*devContainer->common.onCreateCommand),
        QStringList() << "echo" << "world");
    QCOMPARE(devContainer->common.updateContentCommand->index(), 2);
    auto commandMap = std::get<std::map<QString, std::variant<QString, QStringList>>>(
        *devContainer->common.updateContentCommand);
    QCOMPARE(commandMap.size(), 2);

    QCOMPARE(commandMap["echo"].index(), 0);
    QCOMPARE(std::get<QString>(commandMap["echo"]), "echo test");
    QCOMPARE(commandMap["ls"].index(), 1);
    QCOMPARE(std::get<QStringList>(commandMap["ls"]), QStringList() << "ls" << "-lach");
}

void tst_DevContainer::upDockerfile()
{
    QTemporaryFile dockerFile;
    dockerFile.setFileTemplate(QDir::tempPath() + "/DockerfileXXXXXX");
    QVERIFY(dockerFile.open());
    dockerFile.write(R"(
FROM alpine:latest AS test
    )");
    dockerFile.flush();

    DevContainer::Config config;
    DevContainer::DockerfileContainer dockerFileConfig {
        //.appPort = 10,
        .dockerfile = dockerFile.fileName(),
        .buildOptions = DevContainer::BuildOptions{
            .target = "test",
            .args = {{"arg1", "value1"}, {"arg2", "value2"}},
            .cacheFrom = QStringList{"cache1", "cache2"}
        },
    };
    config.containerConfig = dockerFileConfig;
    config.common.name = "Test Dockerfile";

    DevContainer::InstanceConfig instanceConfig{
        .workspaceFolder = tempDir,
        .configFilePath = tempDir / "devcontainer.json",
        .mounts = {},
        .logFunction = logFunction};

    std::unique_ptr<DevContainer::Instance> instance
        = DevContainer::Instance::fromConfig(config, instanceConfig);

    DevContainer::RunningInstance runningInstance
        = std::make_shared<DevContainer::RunningInstanceData>();
    const Result<Group> recipe = instance->upRecipe(runningInstance);
    QVERIFY_RESULT(recipe);
    QCOMPARE(QTaskTree::runBlocking((*recipe).withTimeout(recipeTimeout)), DoneWith::Success);

    const Result<Group> downRecipe = instance->downRecipe(false);
    QVERIFY_RESULT(downRecipe);
    QCOMPARE(QTaskTree::runBlocking(*downRecipe), DoneWith::Success);
}

void tst_DevContainer::upImage()
{
    DevContainer::Config config;
    DevContainer::ImageContainer imageConfig{
        .image = "alpine:latest",
    };
    config.containerConfig = imageConfig;
    config.common.name = "Test Image";

    DevContainer::InstanceConfig instanceConfig{
        .workspaceFolder = tempDir,
        .configFilePath = tempDir / "devcontainer.json",
        .mounts = {},
        .logFunction = logFunction};

    std::unique_ptr<DevContainer::Instance> instance
        = DevContainer::Instance::fromConfig(config, instanceConfig);

    DevContainer::RunningInstance runningInstance
        = std::make_shared<DevContainer::RunningInstanceData>();
    const Result<Group> recipe = instance->upRecipe(runningInstance);
    QVERIFY_RESULT(recipe);
    QCOMPARE(QTaskTree::runBlocking((*recipe).withTimeout(recipeTimeout)), DoneWith::Success);

    const Result<Group> downRecipe = instance->downRecipe(false);
    QVERIFY_RESULT(downRecipe);
    QCOMPARE(QTaskTree::runBlocking(*downRecipe), DoneWith::Success);
}

void tst_DevContainer::upWithHooks()
{
    DevContainer::Config config;
    DevContainer::ImageContainer imageConfig{
        .image = "alpine:latest",
    };
    config.containerConfig = imageConfig;
    config.common.name = "Test Image";
    if (HostOsInfo::isWindowsHost())
        config.common.initializeCommand = "ver";
    else
        config.common.initializeCommand = "uname -a";

    config.common.onCreateCommand = QStringList{"ls", "-lach"};
    config.common.postCreateCommand = "uname -a";
    config.common.updateContentCommand = DevContainer::CommandMap{
        std::make_pair(
            "parallel echo 1", "echo First echo \\(waiting 1\\) && sleep 1 && echo Done sleeping"),
        std::make_pair(
            "parallel echo 2 ", "echo Second echo \\(waiting 2\\) && sleep 2 && echo Done sleeping"),
        std::make_pair("run ls", QStringList{"ls", "-l", "/tmp"}),
    };

    DevContainer::InstanceConfig instanceConfig{
        .workspaceFolder = tempDir,
        .configFilePath = tempDir / "devcontainer.json",
        .mounts = {},
        .logFunction = logFunction};

    std::unique_ptr<DevContainer::Instance> instance
        = DevContainer::Instance::fromConfig(config, instanceConfig);

    DevContainer::RunningInstance runningInstance
        = std::make_shared<DevContainer::RunningInstanceData>();
    const Result<Group> recipe = instance->upRecipe(runningInstance);
    QVERIFY_RESULT(recipe);
    QCOMPARE(QTaskTree::runBlocking((*recipe).withTimeout(recipeTimeout)), DoneWith::Success);

    const Result<Group> downRecipe = instance->downRecipe(false);
    QVERIFY_RESULT(downRecipe);
    QCOMPARE(QTaskTree::runBlocking(*downRecipe), DoneWith::Success);
}

void tst_DevContainer::processInterface()
{
    DevContainer::ImageContainer imageConfig{
        .image = "alpine:latest",
    };

    DevContainer::Config config;
    config.containerConfig = imageConfig;
    config.common.name = "Test Image";

    config.common.containerEnv = {
        {"CONTAINER_TEST", "test_value_container"},
        {"CONTAINER_VAR", "container_value"},
        {"CONTAINER_UNSET_ME", "Not unset yet!"},
        {"CONTAINER_CHANGE_ME", "container_value_to_change"},
    };

    config.common.remoteEnv
        = {{"TEST_VAR", "test_value"},
           {"ANOTHER_VAR", "another_value"},
           {"CONTAINER_UNSET_ME", std::nullopt},
           {"CONTAINER_CHANGE_ME", "changed_container_value"},
           {"REMOTEENV_FROM_CONTAINER", "${containerEnv:CONTAINER_TEST}"}};

    DevContainer::InstanceConfig instanceConfig{
        .workspaceFolder = tempDir,
        .configFilePath = tempDir / "devcontainer.json",
        .mounts = {},
        .logFunction = logFunction};

    std::unique_ptr<DevContainer::Instance> instance
        = DevContainer::Instance::fromConfig(config, instanceConfig);

    DevContainer::RunningInstance runningInstance
        = std::make_shared<DevContainer::RunningInstanceData>();
    const Result<Group> recipe = instance->upRecipe(runningInstance);
    QVERIFY_RESULT(recipe);
    QCOMPARE(QTaskTree::runBlocking((*recipe).withTimeout(recipeTimeout)), DoneWith::Success);

    Process process;

    Environment testEnv;
    testEnv.set("CONTAINER_VAR", "changed_container_value");
    testEnv.set("CONTAINER_TEST", "", false);

    process.setEnvironment(testEnv);
    process.setProcessInterfaceCreator(
        [&]() { return instance->createProcessInterface(runningInstance); });
    process.setCommand({"printenv", {}});
    process.runBlocking(std::chrono::seconds(10));
    const QString output = process.cleanedStdOut().trimmed();

    logFunction("Process output:" + output);
    logFunction("Process error:" + process.cleanedStdErr().trimmed());
    logFunction(process.verboseExitMessage());

    QVERIFY(process.result() == ProcessResult::FinishedWithSuccess);

    Environment firstEnv(output.split('\n', Qt::SkipEmptyParts));
    QVERIFY(!firstEnv.hasKey("CONTAINER_TEST"));
    QVERIFY(!firstEnv.hasKey("CONTAINER_UNSET_ME"));
    QCOMPARE(firstEnv.value("CONTAINER_VAR"), "changed_container_value");
    QCOMPARE(firstEnv.value("TEST_VAR"), "test_value");
    QCOMPARE(firstEnv.value("ANOTHER_VAR"), "another_value");
    QCOMPARE(firstEnv.value("CONTAINER_CHANGE_ME"), "changed_container_value");
    QCOMPARE(firstEnv.value("REMOTEENV_FROM_CONTAINER"), "test_value_container");

    Process sleepProc;
    sleepProc.setProcessInterfaceCreator(
        [&]() { return instance->createProcessInterface(runningInstance); });
    sleepProc.setCommand({"sleep", {"100000"}});
    sleepProc.start();
    QVERIFY(sleepProc.waitForStarted());
    sleepProc.kill();
    QVERIFY(sleepProc.waitForFinished());

    const Result<Group> downRecipe = instance->downRecipe(false);
    QVERIFY_RESULT(downRecipe);
    QCOMPARE(QTaskTree::runBlocking(*downRecipe), DoneWith::Success);
}

void tst_DevContainer::containerWorkspaceReplacers()
{
    static const QByteArray jsonData = R"json(
{
    "build": {
        "dockerfile": "Dockerfile"
    },
    "workspaceFolder": "/custom/workspace/folder",
    "containerEnv": {
        "folder": "${containerWorkspaceFolder}",
        "basename": "${containerWorkspaceFolderBasename}"
    }
}
    )json";

    DevContainer::InstanceConfig instanceConfig{
        .workspaceFolder = tempDir,
        .configFilePath = tempDir / "devcontainer.json",
        .mounts = {},
        .logFunction = logFunction};

    const Result<DevContainer::Config> config
        = DevContainer::Config::fromJson(jsonData, [instanceConfig](const QJsonValue &value) {
              return instanceConfig.jsonToString(value);
          });

    QVERIFY_RESULT(config);
    QCOMPARE(config->containerConfig->index(), 0);
    const auto containerConfig = std::get<DevContainer::DockerfileContainer>(
        *config->containerConfig);
    QCOMPARE(containerConfig.workspaceFolder, "/custom/workspace/folder");
    QCOMPARE((*config).common.containerEnv.at("folder"), "/custom/workspace/folder");
    QCOMPARE((*config).common.containerEnv.at("basename"), "folder");
}

void tst_DevContainer::dockerCompose()
{
    if (HostOsInfo::isLinuxHost())
        QSKIP("docker-compose has been having spurious failures. Skipping on Linux for now.");

    static const QByteArray composeFile = R"yaml(
services:
  devcontainer:
    image: alpine:latest
    volumes:
      - ../..:/workspaces:cached
    network_mode: service:db
    command: sleep infinity

  db:
    image: postgres:latest
    restart: unless-stopped
    environment:
      POSTGRES_PASSWORD: postgres
      POSTGRES_USER: postgres
      POSTGRES_DB: postgres

volumes:
  postgres-data:
)yaml";

    static const QByteArray devcontainerJson = R"json(
{
    "name": "Test Compose",
    "dockerComposeFile": "docker-compose.yml",
    "service": "devcontainer",
    "workspaceFolder": "/workspaces/${localWorkspaceFolderBasename}"
}
)json";

    const FilePath dotDevContainerDir = tempDir / ".devcontainer";
    QVERIFY_RESULT(dotDevContainerDir.ensureWritableDir());

    const FilePath composePath = dotDevContainerDir / "docker-compose.yml";
    QVERIFY_RESULT(composePath.writeFileContents(composeFile));

    DevContainer::InstanceConfig instanceConfig{
        .workspaceFolder = tempDir,
        .configFilePath = dotDevContainerDir / "devcontainer.json",
        .mounts = {},
        .logFunction = logFunction};

    const Result<DevContainer::Config> config
        = DevContainer::Config::fromJson(devcontainerJson, [instanceConfig](const QJsonValue &value) {
              return instanceConfig.jsonToString(value);
          });

    QVERIFY_RESULT(config);

    std::unique_ptr<DevContainer::Instance> instance
        = DevContainer::Instance::fromConfig(*config, instanceConfig);

    DevContainer::RunningInstance runningInstance
        = std::make_shared<DevContainer::RunningInstanceData>();
    const Result<Group> recipe = instance->upRecipe(runningInstance);
    QVERIFY_RESULT(recipe);
    QCOMPARE(QTaskTree::runBlocking((*recipe).withTimeout(recipeTimeout)), DoneWith::Success);

    Process process;
    process.setProcessInterfaceCreator(
        [&]() { return instance->createProcessInterface(runningInstance); });
    process.setCommand({"ls", {"-lach"}});
    process.runBlocking(std::chrono::seconds(10));

    logFunction("Process output: " + process.cleanedStdOut().trimmed());
    logFunction("Process error: " + process.cleanedStdErr().trimmed());
    logFunction(process.verboseExitMessage());

    QVERIFY(process.exitCode() == 0);

    // Shutdown
    const Result<Group> downRecipe = instance->downRecipe(false);
    QVERIFY_RESULT(downRecipe);
    QCOMPARE(QTaskTree::runBlocking((*downRecipe).withTimeout(recipeTimeout)), DoneWith::Success);
}

void tst_DevContainer::upImageWithLocalFeatures()
{
    const FilePath configFolder = tempDir / ".devcontainer" / "features-image";
    QVERIFY_RESULT(configFolder.ensureWritableDir());

    // "asecond" sorts first, so only its installsAfter puts it behind "zfirst".
    writeFeature(
        configFolder / "zfirst",
        R"json({
        "id": "zfirst",
        "version": "1.0.0",
        "options": {
            "greeting": { "type": "string", "default": "hello" },
            "flag": { "type": "boolean", "default": false }
        },
        "containerEnv": { "ZFIRST_ENV": "from-zfirst" },
        "capAdd": [ "SYS_PTRACE" ],
        "entrypoint": "/usr/local/share/zfirst-entrypoint.sh",
        "postCreateCommand": "echo zfirst >> /tmp/hooks.txt"
    })json",
        R"(#!/bin/sh
set -e
echo "$GREETING $FLAG" > /usr/local/share/zfirst.txt
printf '#!/bin/sh\ntouch /tmp/zfirst-entrypoint-ran\n' > /usr/local/share/zfirst-entrypoint.sh
chmod +x /usr/local/share/zfirst-entrypoint.sh
)");
    writeFeature(
        configFolder / "asecond",
        R"json({
        "id": "asecond",
        "version": "1.0.0",
        "installsAfter": [ "./zfirst" ]
    })json",
        R"(#!/bin/sh
set -e
cat /usr/local/share/zfirst.txt > /usr/local/share/asecond.txt
echo "$_REMOTE_USER:$_REMOTE_USER_HOME:$_CONTAINER_USER" >> /usr/local/share/asecond.txt
)");

    const FilePath configFile = configFolder / "devcontainer.json";
    QVERIFY_RESULT(configFile.writeFileContents(R"json({
        "image": "alpine:latest",
        "features": {
            "./asecond": {},
            "./zfirst": { "greeting": "hi there", "flag": true }
        },
        "postCreateCommand": "echo user >> /tmp/hooks.txt"
    })json"));

    const UpResult result = up(configFile);
    const auto guard = qScopeGuard([&] { down(result); });
    QCOMPARE(result.doneWith, DoneWith::Success);

    QCOMPARE(
        runInContainer(result, {"cat", {"/usr/local/share/asecond.txt"}}),
        "hi there true\nroot:/root:root");
    QCOMPARE(runInContainer(result, {"printenv", {"ZFIRST_ENV"}}), "from-zfirst");
    QCOMPARE(runInContainer(result, {"cat", {"/tmp/hooks.txt"}}), "zfirst\nuser");
    QCOMPARE(
        runInContainer(result, {"/bin/sh", {"-c", "test -f /tmp/zfirst-entrypoint-ran && echo yes"}}),
        "yes");

    Process inspect;
    inspect.setCommand(
        {"docker",
         {"inspect",
          "--format",
          "{{json .HostConfig.CapAdd}}",
          result.runningInstance->containerId}});
    inspect.runBlocking();
    QVERIFY2(inspect.cleanedStdOut().contains("SYS_PTRACE"), qPrintable(inspect.cleanedStdOut()));
}

void tst_DevContainer::upDockerfileWithFeature()
{
    const FilePath configFolder = tempDir / ".devcontainer" / "features-dockerfile";
    QVERIFY_RESULT(configFolder.ensureWritableDir());

    QVERIFY_RESULT((configFolder / "Dockerfile").writeFileContents(R"(FROM alpine:latest
RUN adduser -D builder
USER builder
)"));
    writeFeature(configFolder / "tool", R"json({ "id": "tool", "version": "1.0.0" })json", R"(#!/bin/sh
set -e
id -un > /usr/local/share/tool-installed-by
echo "$_CONTAINER_USER:$_CONTAINER_USER_HOME" >> /usr/local/share/tool-installed-by
)");

    const FilePath configFile = configFolder / "devcontainer.json";
    QVERIFY_RESULT(configFile.writeFileContents(R"json({
        "build": { "dockerfile": "Dockerfile" },
        "features": { "./tool": {} }
    })json"));

    const UpResult result = up(configFile);
    const auto guard = qScopeGuard([&] { down(result); });
    QCOMPARE(result.doneWith, DoneWith::Success);

    // Features install as root, the container runs as the user of the Dockerfile again.
    QCOMPARE(
        runInContainer(result, {"cat", {"/usr/local/share/tool-installed-by"}}),
        "root\nbuilder:/home/builder");
    QCOMPARE(runInContainer(result, {"id", {"-un"}}), "builder");
}

void tst_DevContainer::upComposeWithFeature()
{
    if (HostOsInfo::isLinuxHost())
        QSKIP("docker-compose has been having spurious failures. Skipping on Linux for now.");

    const FilePath configFolder = tempDir / ".devcontainer" / "features-compose";
    QVERIFY_RESULT(configFolder.ensureWritableDir());

    QVERIFY_RESULT((configFolder / "docker-compose.yml").writeFileContents(R"yaml(
services:
  app:
    image: alpine:latest
    command: ["sleep", "infinity"]
)yaml"));
    writeFeature(
        configFolder / "compfeat",
        R"json({
        "id": "compfeat",
        "version": "1.0.0",
        "containerEnv": { "COMPFEAT": "yes" },
        "entrypoint": "/usr/local/share/compfeat-entrypoint.sh",
        "postCreateCommand": "echo compfeat >> /tmp/hooks.txt"
    })json",
        R"(#!/bin/sh
set -e
printf '#!/bin/sh\ntouch /tmp/compfeat-entrypoint-ran\n' > /usr/local/share/compfeat-entrypoint.sh
chmod +x /usr/local/share/compfeat-entrypoint.sh
)");

    const QString volumeName = "qtc-devcontainer-test-"
                               + QString::number(QCoreApplication::applicationPid());
    const FilePath configFile = configFolder / "devcontainer.json";
    QVERIFY_RESULT(configFile.writeFileContents(QString(R"json({
        "dockerComposeFile": "docker-compose.yml",
        "service": "app",
        "workspaceFolder": "/tmp",
        "features": { "./compfeat": {} },
        "mounts": [ { "type": "volume", "source": "%1", "target": "/data" } ],
        "postCreateCommand": "echo user >> /tmp/hooks.txt"
    })json")
                                                    .arg(volumeName)
                                                    .toUtf8()));

    const UpResult result = up(configFile);
    const auto guard = qScopeGuard([&] {
        down(result);
        Process rm;
        rm.setCommand({"docker", {"volume", "rm", volumeName}});
        rm.runBlocking();
    });
    QCOMPARE(result.doneWith, DoneWith::Success);

    QCOMPARE(runInContainer(result, {"printenv", {"COMPFEAT"}}), "yes");
    QCOMPARE(runInContainer(result, {"cat", {"/tmp/hooks.txt"}}), "compfeat\nuser");
    QCOMPARE(
        runInContainer(
            result,
            {"/bin/sh",
             {"-c", "test -f /tmp/compfeat-entrypoint-ran && test -d /data && echo yes"}}),
        "yes");
    // Starting again finds the container running, so the creation hooks do not run twice.
    const UpResult again = up(configFile);
    QCOMPARE(again.doneWith, DoneWith::Success);
    QCOMPARE(runInContainer(again, {"cat", {"/tmp/hooks.txt"}}), "compfeat\nuser");

    // The command of the service still runs behind the entrypoint of the feature.
    QCOMPARE(
        runInContainer(
            result, {"/bin/sh", {"-c", "pgrep -f 'sleep infinity' >/dev/null && echo yes"}}),
        "yes");
}

void tst_DevContainer::upWithOciFeature()
{
    const FilePath configFolder = tempDir / ".devcontainer" / "features-oci";
    QVERIFY_RESULT(configFolder.ensureWritableDir());

    // common-utils creates the remote user, which it learns from _REMOTE_USER.
    const FilePath configFile = configFolder / "devcontainer.json";
    QVERIFY_RESULT(configFile.writeFileContents(R"json({
        "image": "alpine:latest",
        "features": {
            "ghcr.io/devcontainers/features/common-utils:2": {
                "installZsh": false,
                "installOhMyZsh": false,
                "installOhMyZshConfig": false,
                "upgradePackages": false,
                "username": "automatic"
            }
        },
        "remoteUser": "tester"
    })json"));

    const UpResult result = up(configFile);
    const auto guard = qScopeGuard([&] { down(result); });
    QCOMPARE(result.doneWith, DoneWith::Success);

    QCOMPARE(runInContainer(result, {"id", {"-un"}}), "tester");
    QCOMPARE(
        runInContainer(result, {"/bin/sh", {"-c", "test -x /usr/local/bin/code && echo yes"}}),
        "yes");
}

void tst_DevContainer::upWithMissingFeature()
{
    const FilePath configFolder = tempDir / ".devcontainer" / "features-missing";
    QVERIFY_RESULT(configFolder.ensureWritableDir());

    const FilePath configFile = configFolder / "devcontainer.json";
    QVERIFY_RESULT(configFile.writeFileContents(R"json({
        "image": "alpine:latest",
        "features": { "ghcr.io/devcontainers/features/qtc-does-not-exist:1": {} }
    })json"));

    const UpResult result = up(configFile);
    const auto guard = qScopeGuard([&] { down(result); });
    QCOMPARE(result.doneWith, DoneWith::Error);
    QVERIFY2(logMessages.contains("qtc-does-not-exist"), qPrintable(logMessages));
    QVERIFY2(!logMessages.contains("Creating container"), qPrintable(logMessages));
}

void tst_DevContainer::upCustomizationsFromFeature()
{
    const FilePath configFolder = tempDir / ".devcontainer" / "features-customizations";
    QVERIFY_RESULT(configFolder.ensureWritableDir());
    writeFeature(
        configFolder / "kitfeat",
        R"json({
        "id": "kitfeat",
        "version": "1.0.0",
        "customizations": { "qt-creator": { "kits": [ { "name": "FromFeature" } ] } }
    })json",
        "#!/bin/sh\n");

    const FilePath configFile = configFolder / "devcontainer.json";
    QVERIFY_RESULT(configFile.writeFileContents(R"json({
        "image": "alpine:latest",
        "features": { "./kitfeat": {} },
        "customizations": { "qt-creator": { "kits": [ { "name": "FromConfig" } ] } }
    })json"));

    const UpResult result = up(configFile);
    const auto guard = qScopeGuard([&] { down(result); });
    QCOMPARE(result.doneWith, DoneWith::Success);

    const QJsonArray kits
        = DevContainer::customization(result.runningInstance->customizations, "qt-creator/kits")
              .toArray();
    QCOMPARE(kits.size(), 2);
    QCOMPARE(kits.at(0).toObject().value("name").toString(), "FromFeature");
    QCOMPARE(kits.at(1).toObject().value("name").toString(), "FromConfig");
}

void tst_DevContainer::lifecycleHooksOnce()
{
    const FilePath configFolder = tempDir / ".devcontainer" / "lifecycle";
    QVERIFY_RESULT(configFolder.ensureWritableDir());
    const FilePath configFile = configFolder / "devcontainer.json";
    QVERIFY_RESULT(configFile.writeFileContents(R"json({
        "image": "alpine:latest",
        "shutdownAction": "none",
        "onCreateCommand": "echo create >> /tmp/lifecycle.txt",
        "updateContentCommand": "echo update >> /tmp/lifecycle.txt",
        "postCreateCommand": "echo postcreate >> /tmp/lifecycle.txt",
        "postStartCommand": "echo start >> /tmp/lifecycle.txt",
        "postAttachCommand": "echo attach >> /tmp/lifecycle.txt"
    })json"));

    const UpResult first = up(configFile);
    const auto guard = qScopeGuard([&] { down(first); });
    QCOMPARE(first.doneWith, DoneWith::Success);
    QCOMPARE(
        runInContainer(first, {"cat", {"/tmp/lifecycle.txt"}}),
        "create\nupdate\npostcreate\nstart\nattach");

    // The container keeps running, so it is neither created nor started again.
    const UpResult second = up(configFile);
    QCOMPARE(second.doneWith, DoneWith::Success);
    QVERIFY2(
        logMessages.contains("Skipping onCreateCommand, as the container existed already."),
        qPrintable(logMessages));
    QVERIFY2(
        logMessages.contains("Skipping postStartCommand, as the container was running already."),
        qPrintable(logMessages));
    QCOMPARE(
        runInContainer(second, {"cat", {"/tmp/lifecycle.txt"}}),
        "create\nupdate\npostcreate\nstart\nattach\nattach");

    Process stop;
    stop.setCommand({"docker", {"stop", "-t", "0", second.runningInstance->containerId}});
    stop.runBlocking();
    QCOMPARE(stop.result(), ProcessResult::FinishedWithSuccess);

    const UpResult third = up(configFile);
    QCOMPARE(third.doneWith, DoneWith::Success);
    QCOMPARE(
        runInContainer(third, {"cat", {"/tmp/lifecycle.txt"}}),
        "create\nupdate\npostcreate\nstart\nattach\nattach\nstart\nattach");
}

void tst_DevContainer::updateRemoteUserUid_data()
{
    QTest::addColumn<QString>("updateSetting");
    QTest::addColumn<QString>("expectedIds");

    QTest::newRow("default") << "" << "4242:4343:4242:4343";
    QTest::newRow("enabled") << R"("updateRemoteUserUID": true,)" << "4242:4343:4242:4343";
    QTest::newRow("disabled") << R"("updateRemoteUserUID": false,)" << "1000:1000:1000:1000";
}

void tst_DevContainer::updateRemoteUserUid()
{
    QFETCH(QString, updateSetting);
    QFETCH(QString, expectedIds);

    const FilePath configFolder = tempDir / ".devcontainer"
                                  / ("uid-" + QString::fromLatin1(QTest::currentDataTag()));
    QVERIFY_RESULT(configFolder.ensureWritableDir());
    QVERIFY_RESULT((configFolder / "Dockerfile").writeFileContents(R"(FROM alpine:latest
RUN adduser -D -u 1000 builder
USER builder
)"));
    const FilePath configFile = configFolder / "devcontainer.json";
    QVERIFY_RESULT(configFile.writeFileContents(QString(R"json({
        "build": { "dockerfile": "Dockerfile" },
        %1
        "remoteUser": "builder"
    })json")
                                                    .arg(updateSetting)
                                                    .toUtf8()));

    const UpResult result = up(configFile, [](DevContainer::InstanceConfig &config) {
        config.localUser = DevContainer::LocalUser{4242, 4343};
    });
    const auto guard = qScopeGuard([&] { down(result); });
    QCOMPARE(result.doneWith, DoneWith::Success);

    QCOMPARE(
        runInContainer(
            result, {"/bin/sh", {"-c", "echo $(id -u):$(id -g):$(stat -c %u:%g /home/builder)"}}),
        expectedIds);
    QCOMPARE(runInContainer(result, {"id", {"-un"}}), "builder");
}

// A Docker config of our own, which still talks to the same Docker daemon.
static FilePath writeDockerConfig(const FilePath &dir, QJsonObject config)
{
    if (!dir.ensureWritableDir())
        return {};
    const Result<QByteArray> userConfig
        = (FilePath::fromUserInput(QDir::homePath()) / ".docker" / "config.json").fileContents();
    const QString currentContext
        = userConfig
              ? QJsonDocument::fromJson(*userConfig).object().value("currentContext").toString()
              : QString();
    if (!currentContext.isEmpty()) {
        config.insert("currentContext", currentContext);
        const FilePath contexts = FilePath::fromUserInput(QDir::homePath()) / ".docker"
                                  / "contexts";
        const FilePath ownContexts = dir / "contexts";
        if (!ownContexts.removeRecursively() || !contexts.copyRecursively(ownContexts))
            return {};
    }
    if (!(dir / "config.json").writeFileContents(QJsonDocument(config).toJson()))
        return {};
    return dir;
}

struct HttpResult
{
    int status = 0;
    QByteArray location;
};

static HttpResult sendRequest(
    QNetworkAccessManager &nam,
    const QByteArray &verb,
    const QUrl &url,
    const QByteArray &contentType,
    const QByteArray &body)
{
    QNetworkRequest request(url);
    request.setRawHeader("Authorization", "Basic " + QByteArray("tester:secret").toBase64());
    if (!contentType.isEmpty())
        request.setHeader(QNetworkRequest::ContentTypeHeader, contentType);
    std::unique_ptr<QNetworkReply> reply(nam.sendCustomRequest(request, verb, body));
    QSignalSpy finished(reply.get(), &QNetworkReply::finished);
    if (!finished.wait(30000))
        return {};
    return {
        reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(),
        reply->rawHeader("Location")};
}

static QByteArray sha256Digest(const QByteArray &data)
{
    return "sha256:" + QCryptographicHash::hash(data, QCryptographicHash::Sha256).toHex();
}

// Pushes a blob the way "docker push" does: open an upload, then complete it with the data.
static bool pushBlob(
    QNetworkAccessManager &nam, const QUrl &base, const QString &repository, const QByteArray &data)
{
    const HttpResult upload = sendRequest(
        nam, "POST", base.resolved(QUrl("/v2/" + repository + "/blobs/uploads/")), {}, {});
    if (upload.status != 202)
        return false;
    QUrl target = base.resolved(QUrl::fromEncoded(upload.location));
    QUrlQuery query(target);
    query.addQueryItem("digest", QString::fromLatin1(sha256Digest(data)));
    target.setQuery(query);
    return sendRequest(nam, "PUT", target, "application/octet-stream", data).status == 201;
}

void tst_DevContainer::privateRegistryFeature()
{
    const FilePath registryDir = tempDir / "registry";
    QVERIFY_RESULT((registryDir / "auth").ensureWritableDir());
    // htpasswd -Bbn tester secret
    QVERIFY_RESULT(
        (registryDir / "auth" / "htpasswd")
            .writeFileContents(
                "tester:$2y$05$0Km8UjZ9mbV39zDeXgx/JOgH5UPFwsRVur.IIjmL43eVR58ELUPPG\n"));

    const QString registryName = "qtc-devcontainer-test-registry-"
                                 + QString::number(QCoreApplication::applicationPid());
    Process run;
    run.setCommand(
        {"docker",
         {"run",
          "-d",
          "--rm",
          "--name",
          registryName,
          "-p",
          "127.0.0.1::5000",
          "-e",
          "REGISTRY_AUTH=htpasswd",
          "-e",
          "REGISTRY_AUTH_HTPASSWD_REALM=test",
          "-e",
          "REGISTRY_AUTH_HTPASSWD_PATH=/auth/htpasswd",
          "--mount",
          "type=bind,source=" + (registryDir / "auth").path() + ",target=/auth",
          "registry:2"}});
    run.runBlocking(std::chrono::minutes(5));
    QVERIFY2(run.result() == ProcessResult::FinishedWithSuccess, qPrintable(run.verboseExitMessage()));
    const auto removeRegistry = qScopeGuard([&] {
        Process rm;
        rm.setCommand({"docker", {"rm", "-f", registryName}});
        rm.runBlocking();
    });

    const auto registryLogs = [&]() -> QString {
        Process logs;
        logs.setCommand({"docker", {"logs", registryName}});
        logs.runBlocking();
        return logs.cleanedStdOut() + logs.cleanedStdErr();
    };
    QTRY_VERIFY_WITH_TIMEOUT(registryLogs().contains("listening on"), 30000);

    Process port;
    port.setCommand({"docker", {"port", registryName, "5000"}});
    port.runBlocking();
    const QString hostPort = port.cleanedStdOut().trimmed().section('\n', 0, 0).section(':', -1);
    QVERIFY(!hostPort.isEmpty());
    const QString registry = "localhost:" + hostPort;

    // Publish a feature.
    const FilePath featureDir = registryDir / "regfeat";
    writeFeature(featureDir, R"json({ "id": "regfeat", "version": "1.0.0" })json", R"(#!/bin/sh
echo from-registry > /usr/local/share/regfeat.txt
)");
    Process tar;
    tar.setCommand({"tar", {"-cf", "-", "-C", featureDir.path(), "."}});
    tar.runBlocking();
    const QByteArray layer = tar.rawStdOut();
    QVERIFY(!layer.isEmpty());
    const QByteArray configBlob = "{}";

    QNetworkAccessManager nam;
    const QUrl base("http://" + registry);
    const QString repository = "qtc/features/regfeat";
    QVERIFY(pushBlob(nam, base, repository, configBlob));
    QVERIFY(pushBlob(nam, base, repository, layer));
    const QByteArray manifest
        = QJsonDocument(
              QJsonObject{
                  {"schemaVersion", 2},
                  {"mediaType", "application/vnd.oci.image.manifest.v1+json"},
                  {"config",
                   QJsonObject{
                       {"mediaType", "application/vnd.devcontainers"},
                       {"digest", QString::fromLatin1(sha256Digest(configBlob))},
                       {"size", configBlob.size()}}},
                  {"layers",
                   QJsonArray{QJsonObject{
                       {"mediaType", "application/vnd.devcontainers.layer.v1+tar"},
                       {"digest", QString::fromLatin1(sha256Digest(layer))},
                       {"size", layer.size()}}}}})
              .toJson();
    QCOMPARE(
        sendRequest(
            nam,
            "PUT",
            base.resolved(QUrl("/v2/" + repository + "/manifests/1")),
            "application/vnd.oci.image.manifest.v1+json",
            manifest)
            .status,
        201);

    const FilePath configFolder = tempDir / ".devcontainer" / "features-registry";
    QVERIFY_RESULT(configFolder.ensureWritableDir());
    const FilePath configFile = configFolder / "devcontainer.json";
    QVERIFY_RESULT(configFile.writeFileContents(QString(R"json({
        "image": "alpine:latest",
        "features": { "%1/qtc/features/regfeat:1": {} }
    })json")
                                                    .arg(registry)
                                                    .toUtf8()));

    const auto dockerConfig = [&](const QString &name, const QJsonObject &config) {
        return writeDockerConfig(registryDir / name, config);
    };

    const FilePath cacheFolder = registryDir / "cache";

    { // Without credentials, the registry refuses.
        logMessages.clear();
        const FilePath dir = dockerConfig("config-none", {});
        const UpResult result = up(configFile, [&](DevContainer::InstanceConfig &config) {
            config.localEnvironment.set("DOCKER_CONFIG", dir.path());
        });
        const auto guard = qScopeGuard([&] { down(result); });
        QCOMPARE(result.doneWith, DoneWith::Error);
        QVERIFY2(logMessages.contains("requires credentials"), qPrintable(logMessages));
    }

    { // A credential helper provides them.
        logMessages.clear();
        const FilePath binDir = registryDir / "bin";
        QVERIFY_RESULT(binDir.ensureWritableDir());
        const FilePath helperInput = registryDir / "helper-input";
        const FilePath helper = binDir
                                / (HostOsInfo::isWindowsHost() ? "docker-credential-qtctest.bat"
                                                             : "docker-credential-qtctest");
        QVERIFY_RESULT(helper.writeFileContents(
            (HostOsInfo::isWindowsHost() ? QString(R"(@echo off
findstr "^" > "%1"
echo {"Username": "tester", "Secret": "secret"}
)")
                                             .arg(helperInput.nativePath())
                                       : QString(R"(#!/bin/sh
cat > "%1"
echo '{"Username": "tester", "Secret": "secret"}'
)")
                                             .arg(helperInput.path()))
                .toUtf8()));
        QVERIFY_RESULT(
            helper.setPermissions(QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner));

        const FilePath dir = dockerConfig(
            "config-helper", QJsonObject{{"credHelpers", QJsonObject{{registry, "qtctest"}}}});
        const UpResult result = up(configFile, [&](DevContainer::InstanceConfig &config) {
            config.localEnvironment.set("DOCKER_CONFIG", dir.path());
            config.localEnvironment.prependOrSetPath(binDir);
        });
        const auto guard = qScopeGuard([&] { down(result); });
        QCOMPARE(result.doneWith, DoneWith::Success);
        QCOMPARE(runInContainer(result, {"cat", {"/usr/local/share/regfeat.txt"}}), "from-registry");
        QCOMPARE(
            helperInput.fileContents().value_or(QByteArray()).trimmed(),
            registry.toUtf8());
    }

    const FilePath authsDir = dockerConfig(
        "config-auths",
        QJsonObject{
            {"auths",
             QJsonObject{
                 {registry,
                  QJsonObject{
                      {"auth", QString::fromLatin1(QByteArray("tester:secret").toBase64())}}}}}});
    const auto withAuths = [&](DevContainer::InstanceConfig &config) {
        config.localEnvironment.set("DOCKER_CONFIG", authsDir.path());
        config.featureCacheFolder = cacheFolder;
    };

    { // Credentials from the config file, and the download fills the cache.
        logMessages.clear();
        const UpResult result = up(configFile, withAuths);
        const auto guard = qScopeGuard([&] { down(result); });
        QCOMPARE(result.doneWith, DoneWith::Success);
        QCOMPARE(runInContainer(result, {"cat", {"/usr/local/share/regfeat.txt"}}), "from-registry");
        QVERIFY2(!logMessages.contains("cached"), qPrintable(logMessages));
        QVERIFY((cacheFolder / "blobs" / "sha256" / QString::fromLatin1(sha256Digest(layer).mid(7)))
                    .isFile());
    }

    { // The second start does not download the blob again.
        logMessages.clear();
        const UpResult result = up(configFile, withAuths);
        const auto guard = qScopeGuard([&] { down(result); });
        QCOMPARE(result.doneWith, DoneWith::Success);
        QVERIFY2(logMessages.contains("is cached already"), qPrintable(logMessages));
    }

    Process stop;
    stop.setCommand({"docker", {"rm", "-f", registryName}});
    stop.runBlocking();
    QCOMPARE(stop.result(), ProcessResult::FinishedWithSuccess);

    { // Without the registry, the cached copy is used.
        logMessages.clear();
        const UpResult result = up(configFile, withAuths);
        const auto guard = qScopeGuard([&] { down(result); });
        QCOMPARE(result.doneWith, DoneWith::Success);
        QVERIFY2(logMessages.contains("Cannot reach the registry"), qPrintable(logMessages));
        QCOMPARE(runInContainer(result, {"cat", {"/usr/local/share/regfeat.txt"}}), "from-registry");
    }
}

void tst_DevContainer::downRemovesFeaturesImage()
{
    const FilePath configFolder = tempDir / ".devcontainer" / "features-down";
    QVERIFY_RESULT(configFolder.ensureWritableDir());
    writeFeature(
        configFolder / "adduser",
        R"json({ "id": "adduser", "version": "1.0.0" })json",
        "#!/bin/sh\nadduser -D -u 1000 dev\n");
    const FilePath configFile = configFolder / "devcontainer.json";
    QVERIFY_RESULT(configFile.writeFileContents(R"json({
        "image": "alpine:latest",
        "features": { "./adduser": {} },
        "remoteUser": "dev"
    })json"));

    // Features and the user ID update together build the intermediate features image.
    const UpResult result = up(configFile, [](DevContainer::InstanceConfig &config) {
        config.localUser = DevContainer::LocalUser{4242, 4242};
    });
    const auto guard = qScopeGuard([&] { down(result); });
    QCOMPARE(result.doneWith, DoneWith::Success);

    const QString featuresImage = result.instance->imageNames().at(2);
    QVERIFY(featuresImage.endsWith("-features"));
    const auto imageExists = [&featuresImage] {
        Process inspect;
        inspect.setCommand({"docker", {"image", "inspect", featuresImage}});
        inspect.runBlocking();
        return inspect.result() == ProcessResult::FinishedWithSuccess;
    };
    QVERIFY(imageExists());

    const Result<Group> downRecipe = result.instance->downRecipe(true);
    QVERIFY_RESULT(downRecipe);
    QCOMPARE(QTaskTree::runBlocking((*downRecipe).withTimeout(recipeTimeout)), DoneWith::Success);
    QVERIFY(!imageExists());
}

static QString composeServiceContainer(const QString &project, const QString &service)
{
    Process ps;
    ps.setCommand(
        {"docker",
         {"ps",
          "-q",
          "--filter",
          "label=com.docker.compose.project=" + project,
          "--filter",
          "label=com.docker.compose.service=" + service}});
    ps.runBlocking();
    return ps.cleanedStdOut().trimmed();
}

void tst_DevContainer::composeRebuildsDependencies()
{
    if (HostOsInfo::isLinuxHost())
        QSKIP("docker-compose has been having spurious failures. Skipping on Linux for now.");

    const FilePath configFolder = tempDir / ".devcontainer" / "compose-dependencies";
    QVERIFY_RESULT((configFolder / "helper").ensureWritableDir());
    QVERIFY_RESULT((configFolder / "docker-compose.yml").writeFileContents(R"yaml(
services:
  app:
    image: alpine:latest
    command: ["sleep", "infinity"]
    depends_on: [helper]
  helper:
    build: ./helper
    command: ["sleep", "infinity"]
)yaml"));
    const auto writeHelper = [&](const QByteArray &version) {
        return (configFolder / "helper" / "Dockerfile")
            .writeFileContents("FROM alpine:latest\nRUN echo " + version + " > /version\n");
    };
    QVERIFY_RESULT(writeHelper("v1"));
    writeFeature(
        configFolder / "nothing",
        R"json({ "id": "nothing", "version": "1.0.0" })json",
        "#!/bin/sh\n");
    const FilePath configFile = configFolder / "devcontainer.json";
    QVERIFY_RESULT(configFile.writeFileContents(R"json({
        "dockerComposeFile": "docker-compose.yml",
        "service": "app",
        "workspaceFolder": "/tmp",
        "features": { "./nothing": {} }
    })json"));

    QString project;
    const UpResult first = up(configFile);
    const auto guard = qScopeGuard([&] {
        down(first);
        Process rmi;
        rmi.setCommand({"docker", {"rmi", "-f", project + "-helper"}});
        rmi.runBlocking();
    });
    QCOMPARE(first.doneWith, DoneWith::Success);

    Process label;
    label.setCommand(
        {"docker",
         {"inspect",
          "--format",
          "{{index .Config.Labels \"com.docker.compose.project\"}}",
          first.runningInstance->containerId}});
    label.runBlocking();
    project = label.cleanedStdOut().trimmed();
    QVERIFY(!project.isEmpty());

    const auto helperVersion = [&] {
        Process cat;
        cat.setCommand(
            {"docker", {"exec", composeServiceContainer(project, "helper"), "cat", "/version"}});
        cat.runBlocking();
        return cat.cleanedStdOut().trimmed();
    };
    QCOMPARE(helperVersion(), "v1");

    // The dependency is rebuilt although the dev container runs a custom image.
    QVERIFY_RESULT(writeHelper("v2"));
    const UpResult second = up(configFile);
    QCOMPARE(second.doneWith, DoneWith::Success);
    QCOMPARE(helperVersion(), "v2");
}

void tst_DevContainer::signalAsRemoteUser()
{
    const FilePath configFolder = tempDir / ".devcontainer" / "signal-user";
    QVERIFY_RESULT(configFolder.ensureWritableDir());
    QVERIFY_RESULT((configFolder / "Dockerfile").writeFileContents(R"(FROM alpine:latest
RUN adduser -D imageuser && adduser -D remote
USER imageuser
)"));
    const FilePath configFile = configFolder / "devcontainer.json";
    QVERIFY_RESULT(configFile.writeFileContents(R"json({
        "build": { "dockerfile": "Dockerfile" },
        "remoteUser": "remote",
        "updateRemoteUserUID": false
    })json"));

    const UpResult result = up(configFile);
    const auto guard = qScopeGuard([&] { down(result); });
    QCOMPARE(result.doneWith, DoneWith::Success);

    Process sleeper;
    sleeper.setProcessInterfaceCreator(
        [&] { return result.instance->createProcessInterface(result.runningInstance); });
    sleeper.setCommand({"sleep", {"4242"}});
    sleeper.start();
    QVERIFY(sleeper.waitForStarted());
    const auto sleeperOwner = [&] {
        return runInContainer(
            result, {"/bin/sh", {"-c", "ps -o user,args | awk '/[s]leep 4242/ { print $1 }'"}});
    };
    QTRY_COMPARE_WITH_TIMEOUT(sleeperOwner(), QString("remote"), 20000);

    // The signal must come from the user who owns the process, not from the image user.
    sleeper.kill();
    QTRY_COMPARE_WITH_TIMEOUT(sleeperOwner(), QString(), 20000);
}

void tst_DevContainer::emptyRemoteUser()
{
    const FilePath configFolder = tempDir / ".devcontainer" / "empty-remote-user";
    QVERIFY_RESULT(configFolder.ensureWritableDir());
    QVERIFY_RESULT((configFolder / "Dockerfile").writeFileContents(R"(FROM alpine:latest
RUN adduser -D -s /bin/ash builder
USER builder
)"));
    const FilePath configFile = configFolder / "devcontainer.json";
    QVERIFY_RESULT(configFile.writeFileContents(R"json({
        "build": { "dockerfile": "Dockerfile" },
        "remoteUser": ""
    })json"));

    const UpResult result = up(configFile);
    const auto guard = qScopeGuard([&] { down(result); });
    QCOMPARE(result.doneWith, DoneWith::Success);

    // An empty remote user is the container user, for the shell lookup as everywhere else.
    QVERIFY2(logMessages.contains("Running container user: builder "), qPrintable(logMessages));
    QCOMPARE(runInContainer(result, {"id", {"-un"}}), "builder");
}

// Answers HTTP requests from a handler, one request per connection.
class FakeHttpServer : public QTcpServer
{
public:
    struct Request
    {
        QByteArray method;
        QByteArray path;
        QByteArray headers;
        QByteArray body;
        quint16 port = 0;
    };
    struct Response
    {
        QByteArray status = "200 OK";
        QByteArray headers;
        QByteArray body;
    };
    using Handler = std::function<Response(const Request &)>;

    explicit FakeHttpServer(const Handler &handler)
    {
        connect(this, &QTcpServer::pendingConnectionAvailable, this, [this, handler] {
            while (QTcpSocket *socket = nextPendingConnection()) {
                auto buffer = std::make_shared<QByteArray>();
                connect(socket, &QTcpSocket::readyRead, socket, [this, socket, buffer, handler] {
                    *buffer += socket->readAll();
                    const qsizetype end = buffer->indexOf("\r\n\r\n");
                    if (end < 0)
                        return;
                    const QByteArray head = buffer->left(end);
                    static const QRegularExpression lengthRegex(
                        "content-length: *(\\d+)", QRegularExpression::CaseInsensitiveOption);
                    const QRegularExpressionMatch length = lengthRegex.match(
                        QString::fromLatin1(head));
                    const qsizetype bodySize = length.hasMatch() ? length.captured(1).toLongLong()
                                                                 : 0;
                    if (buffer->size() < end + 4 + bodySize)
                        return;
                    const qsizetype lineEnd = head.indexOf('\n');
                    const QList<QByteArray> requestLine = head.left(lineEnd).trimmed().split(' ');
                    const Request request{
                        requestLine.value(0),
                        requestLine.value(1),
                        head.mid(lineEnd + 1),
                        buffer->mid(end + 4, bodySize),
                        socket->localPort()};
                    requests.append(request);
                    const Response response = handler(request);
                    socket->write(
                        "HTTP/1.1 " + response.status + "\r\n" + response.headers
                        + "Content-Length: " + QByteArray::number(response.body.size())
                        + "\r\nConnection: close\r\n\r\n" + response.body);
                    socket->disconnectFromHost();
                });
                connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
            }
        });
    }

    QList<Request> requests;
};

void tst_DevContainer::verifyManifest_data()
{
    QTest::addColumn<QByteArray>("body");
    QTest::addColumn<QString>("pin");
    QTest::addColumn<QString>("expectedError");

    const QByteArray manifest = R"({"schemaVersion": 2, "layers": [{"mediaType":
        "application/vnd.devcontainers.layer.v1+tar", "digest": "sha256:)"
                                + QByteArray(64, 'b') + R"(", "size": 1}]})";

    QTest::newRow("wrong-digest") << manifest << "@sha256:" + QString(64, 'a')
                                  << "does not match its digest";
    QTest::newRow("not-json") << QByteArray("<html>Proxy error</html>") << ":1"
                              << "Cannot read the manifest";
}

void tst_DevContainer::verifyManifest()
{
    QFETCH(QByteArray, body);
    QFETCH(QString, pin);
    QFETCH(QString, expectedError);

    // Answers every request with the same body, like a registry that serves the wrong content.
    FakeHttpServer registry([body](const FakeHttpServer::Request &) {
        return FakeHttpServer::Response{
            "200 OK", "Content-Type: application/vnd.oci.image.manifest.v1+json\r\n", body};
    });
    QVERIFY(registry.listen(QHostAddress::LocalHost));

    const FilePath configFolder = tempDir / ".devcontainer"
                                  / ("manifest-" + QString::fromLatin1(QTest::currentDataTag()));
    QVERIFY_RESULT(configFolder.ensureWritableDir());
    const FilePath configFile = configFolder / "devcontainer.json";
    QVERIFY_RESULT(configFile.writeFileContents(
        QString(
            R"json({ "image": "alpine:latest", "features": { "127.0.0.1:%1/qtc/feat%2": {} } })json")
            .arg(registry.serverPort())
            .arg(pin)
            .toUtf8()));

    const UpResult result = up(configFile);
    const auto guard = qScopeGuard([&] { down(result); });
    QCOMPARE(result.doneWith, DoneWith::Error);
    QVERIFY2(logMessages.contains(expectedError), qPrintable(logMessages));
}

void tst_DevContainer::blobRedirectsWithoutCredentials()
{
    const FilePath featureDir = tempDir / "redirect-feature";
    writeFeature(
        featureDir,
        R"json({ "id": "redirected", "version": "1.0.0" })json",
        "#!/bin/sh\necho redirected > /usr/local/share/redirected.txt\n");
    Process tar;
    tar.setCommand({"tar", {"-cf", "-", "-C", featureDir.path(), "."}});
    tar.runBlocking();
    const QByteArray layer = tar.rawStdOut();
    QVERIFY(!layer.isEmpty());
    const QByteArray layerDigest
        = "sha256:" + QCryptographicHash::hash(layer, QCryptographicHash::Sha256).toHex();

    // The storage redirects once more on its own host, as signed URLs do.
    FakeHttpServer storage([layer](const FakeHttpServer::Request &request) {
        if (request.path == "/blob")
            return FakeHttpServer::Response{"302 Found", "Location: /blob?sig=1\r\n", {}};
        return FakeHttpServer::Response{"200 OK", {}, layer};
    });
    QVERIFY(storage.listen(QHostAddress::Any));

    FakeHttpServer registry([&](const FakeHttpServer::Request &request) {
        const bool authorized = request.headers.contains("Bearer registry-token");
        if (request.path.startsWith("/token"))
            return FakeHttpServer::Response{"200 OK", {}, R"({"token": "registry-token"})"};
        if (!authorized) {
            return FakeHttpServer::Response{
                "401 Unauthorized",
                "WWW-Authenticate: Bearer realm=\"http://127.0.0.1:"
                    + QByteArray::number(request.port) + "/token\",service=\"fake\"\r\n",
                {}};
        }
        if (request.path.contains("/manifests/")) {
            const QByteArray manifest
                = QJsonDocument(
                      QJsonObject{
                          {"schemaVersion", 2},
                          {"layers",
                           QJsonArray{QJsonObject{
                               {"mediaType", "application/vnd.devcontainers.layer.v1+tar"},
                               {"digest", QString::fromLatin1(layerDigest)},
                               {"size", layer.size()}}}}})
                      .toJson();
            return FakeHttpServer::Response{
                "200 OK", "Content-Type: application/vnd.oci.image.manifest.v1+json\r\n", manifest};
        }
        return FakeHttpServer::Response{
            "307 Temporary Redirect",
            "Location: http://localhost:" + QByteArray::number(storage.serverPort()) + "/blob\r\n",
            {}};
    });
    QVERIFY(registry.listen(QHostAddress::LocalHost));

    const FilePath configFolder = tempDir / ".devcontainer" / "blob-redirects";
    QVERIFY_RESULT(configFolder.ensureWritableDir());
    const FilePath configFile = configFolder / "devcontainer.json";
    QVERIFY_RESULT(configFile.writeFileContents(
        QString(
            R"json({ "image": "alpine:latest", "features": { "127.0.0.1:%1/qtc/redirected:1": {} } })json")
            .arg(registry.serverPort())
            .toUtf8()));

    const UpResult result = up(configFile);
    const auto guard = qScopeGuard([&] { down(result); });
    QCOMPARE(result.doneWith, DoneWith::Success);
    QCOMPARE(runInContainer(result, {"cat", {"/usr/local/share/redirected.txt"}}), "redirected");

    QCOMPARE(storage.requests.size(), 2);
    for (const FakeHttpServer::Request &request : std::as_const(storage.requests))
        QVERIFY2(!request.headers.toLower().contains("authorization:"), request.path.constData());
}

void tst_DevContainer::composeServiceEntrypoint()
{
    if (HostOsInfo::isLinuxHost())
        QSKIP("docker-compose has been having spurious failures. Skipping on Linux for now.");

    const FilePath configFolder = tempDir / ".devcontainer" / "compose-entrypoint";
    QVERIFY_RESULT(configFolder.ensureWritableDir());
    // The image has a CMD, which the entrypoint of the service replaces.
    QVERIFY_RESULT((configFolder / "docker-compose.yml").writeFileContents(R"yaml(
services:
  app:
    image: alpine:latest
    entrypoint: ["sleep", "4343"]
)yaml"));
    writeFeature(
        configFolder / "wrapper",
        R"json({
        "id": "wrapper",
        "version": "1.0.0",
        "entrypoint": "/usr/local/share/wrapper-entrypoint.sh"
    })json",
        R"(#!/bin/sh
printf '#!/bin/sh\ntrue\n' > /usr/local/share/wrapper-entrypoint.sh
chmod +x /usr/local/share/wrapper-entrypoint.sh
)");
    const FilePath configFile = configFolder / "devcontainer.json";
    QVERIFY_RESULT(configFile.writeFileContents(R"json({
        "dockerComposeFile": "docker-compose.yml",
        "service": "app",
        "workspaceFolder": "/tmp",
        "features": { "./wrapper": {} }
    })json"));

    const UpResult result = up(configFile);
    const auto guard = qScopeGuard([&] { down(result); });
    QCOMPARE(result.doneWith, DoneWith::Success);
    QCOMPARE(
        runInContainer(result, {"/bin/sh", {"-c", "tr '\\0' ' ' < /proc/1/cmdline"}}), "sleep 4343");
}

void tst_DevContainer::mountConflict()
{
    const QString pid = QString::number(QCoreApplication::applicationPid());
    const QString featureVolume = "qtc-devcontainer-test-feature-" + pid;
    const QString configVolume = "qtc-devcontainer-test-config-" + pid;

    const FilePath configFolder = tempDir / ".devcontainer" / "mount-conflict";
    QVERIFY_RESULT(configFolder.ensureWritableDir());
    writeFeature(
        configFolder / "withmount",
        QString(R"json({
        "id": "withmount",
        "version": "1.0.0",
        "mounts": [ { "type": "volume", "source": "%1", "target": "/data" } ]
    })json")
            .arg(featureVolume)
            .toUtf8(),
        "#!/bin/sh\n");
    const FilePath configFile = configFolder / "devcontainer.json";
    QVERIFY_RESULT(configFile.writeFileContents(QString(R"json({
        "image": "alpine:latest",
        "features": { "./withmount": {} },
        "mounts": [ "type=volume,source=%1,target=/data" ]
    })json")
                                                    .arg(configVolume)
                                                    .toUtf8()));

    const UpResult result = up(configFile);
    const auto guard = qScopeGuard([&] {
        down(result);
        Process rm;
        rm.setCommand({"docker", {"volume", "rm", featureVolume, configVolume}});
        rm.runBlocking();
    });
    QCOMPARE(result.doneWith, DoneWith::Success);

    // The mount of the configuration comes later, so it wins.
    Process inspect;
    inspect.setCommand(
        {"docker",
         {"inspect",
          "--format",
          R"({{range .Mounts}}{{if eq .Destination "/data"}}{{.Name}} {{end}}{{end}})",
          result.runningInstance->containerId}});
    inspect.runBlocking();
    QCOMPARE(inspect.cleanedStdOut().trimmed(), configVolume);
}

void tst_DevContainer::composeRecreatedRunsCreateHooks()
{
    if (HostOsInfo::isLinuxHost())
        QSKIP("docker-compose has been having spurious failures. Skipping on Linux for now.");

    const FilePath configFolder = tempDir / ".devcontainer" / "compose-recreated";
    QVERIFY_RESULT(configFolder.ensureWritableDir());
    QVERIFY_RESULT((configFolder / "docker-compose.yml").writeFileContents(R"yaml(
services:
  app:
    image: alpine:latest
    command: ["sleep", "infinity"]
)yaml"));
    const auto writeConfig = [&](const QString &value) {
        return (configFolder / "devcontainer.json")
            .writeFileContents(QString(R"json({
            "dockerComposeFile": "docker-compose.yml",
            "service": "app",
            "workspaceFolder": "/tmp",
            "shutdownAction": "none",
            "containerEnv": { "GENERATION": "%1" },
            "onCreateCommand": "touch /tmp/created"
        })json")
                                   .arg(value)
                                   .toUtf8());
    };
    QVERIFY_RESULT(writeConfig("1"));
    const FilePath configFile = configFolder / "devcontainer.json";

    const UpResult first = up(configFile);
    const auto guard = qScopeGuard([&] { down(first); });
    QCOMPARE(first.doneWith, DoneWith::Success);
    QCOMPARE(runInContainer(first, {"/bin/sh", {"-c", "test -f /tmp/created && echo yes"}}), "yes");

    // A changed environment makes compose replace the running container with a new one.
    QVERIFY_RESULT(writeConfig("2"));
    const UpResult second = up(configFile);
    QCOMPARE(second.doneWith, DoneWith::Success);
    QVERIFY(second.runningInstance->containerId != first.runningInstance->containerId);
    QCOMPARE(runInContainer(second, {"printenv", {"GENERATION"}}), "2");
    QCOMPARE(runInContainer(second, {"/bin/sh", {"-c", "test -f /tmp/created && echo yes"}}), "yes");
}

void tst_DevContainer::tokenRealmWithoutHttps()
{
    FakeHttpServer registry([](const FakeHttpServer::Request &) {
        return FakeHttpServer::Response{
            "401 Unauthorized",
            "WWW-Authenticate: Bearer realm=\"http://example.invalid/token\",service=\"fake\"\r\n",
            {}};
    });
    QVERIFY(registry.listen(QHostAddress::LocalHost));

    const FilePath configFolder = tempDir / ".devcontainer" / "realm-http";
    QVERIFY_RESULT(configFolder.ensureWritableDir());
    const FilePath configFile = configFolder / "devcontainer.json";
    QVERIFY_RESULT(configFile.writeFileContents(
        QString(
            R"json({ "image": "alpine:latest", "features": { "127.0.0.1:%1/qtc/feat:1": {} } })json")
            .arg(registry.serverPort())
            .toUtf8()));

    const UpResult result = up(configFile);
    const auto guard = qScopeGuard([&] { down(result); });
    QCOMPARE(result.doneWith, DoneWith::Error);
    QVERIFY2(logMessages.contains("which does not use HTTPS"), qPrintable(logMessages));
}

void tst_DevContainer::identityTokenRefresh()
{
    const FilePath featureDir = tempDir / "token-feature";
    writeFeature(
        featureDir,
        R"json({ "id": "tokenfeat", "version": "1.0.0" })json",
        "#!/bin/sh\necho token > /usr/local/share/tokenfeat.txt\n");
    Process tar;
    tar.setCommand({"tar", {"-cf", "-", "-C", featureDir.path(), "."}});
    tar.runBlocking();
    const QByteArray layer = tar.rawStdOut();
    QVERIFY(!layer.isEmpty());
    const QString layerDigest = QString::fromLatin1(
        "sha256:" + QCryptographicHash::hash(layer, QCryptographicHash::Sha256).toHex());

    // A token with the characters a form encodes: "+" is a space and "%" starts an escape.
    const QString identityToken = "a+b/c%2Bd=";
    QString receivedToken;

    FakeHttpServer registry([&](const FakeHttpServer::Request &request) {
        if (request.method == "POST" && request.path == "/token") {
            // Decode the form the way a token server does.
            QByteArray value;
            for (const QByteArray &pair : request.body.split('&')) {
                if (pair.startsWith("refresh_token="))
                    value = pair.mid(14);
            }
            receivedToken = QString::fromUtf8(
                QByteArray::fromPercentEncoding(value.replace('+', ' ')));
            return FakeHttpServer::Response{"200 OK", {}, R"({"access_token": "short-lived"})"};
        }
        if (!request.headers.contains("Bearer short-lived")) {
            return FakeHttpServer::Response{
                "401 Unauthorized",
                "WWW-Authenticate: Bearer realm=\"http://127.0.0.1:"
                    + QByteArray::number(request.port) + "/token\",service=\"fake\"\r\n",
                {}};
        }
        if (request.path.contains("/manifests/")) {
            const QByteArray manifest
                = QJsonDocument(
                      QJsonObject{
                          {"schemaVersion", 2},
                          {"layers",
                           QJsonArray{QJsonObject{
                               {"mediaType", "application/vnd.devcontainers.layer.v1+tar"},
                               {"digest", layerDigest},
                               {"size", layer.size()}}}}})
                      .toJson();
            return FakeHttpServer::Response{
                "200 OK", "Content-Type: application/vnd.oci.image.manifest.v1+json\r\n", manifest};
        }
        return FakeHttpServer::Response{"200 OK", {}, layer};
    });
    QVERIFY(registry.listen(QHostAddress::LocalHost));
    const QString registryName = "127.0.0.1:" + QString::number(registry.serverPort());

    const FilePath dockerConfigDir = writeDockerConfig(
        tempDir / "dockerconfig-identitytoken",
        QJsonObject{
            {"auths", QJsonObject{{registryName, QJsonObject{{"identitytoken", identityToken}}}}}});
    QVERIFY(!dockerConfigDir.isEmpty());

    const FilePath configFolder = tempDir / ".devcontainer" / "identity-token";
    QVERIFY_RESULT(configFolder.ensureWritableDir());
    const FilePath configFile = configFolder / "devcontainer.json";
    QVERIFY_RESULT(configFile.writeFileContents(
        QString(R"json({ "image": "alpine:latest", "features": { "%1/qtc/tokenfeat:1": {} } })json")
            .arg(registryName)
            .toUtf8()));

    const UpResult result = up(configFile, [&](DevContainer::InstanceConfig &config) {
        config.localEnvironment.set("DOCKER_CONFIG", dockerConfigDir.path());
    });
    const auto guard = qScopeGuard([&] { down(result); });
    QCOMPARE(receivedToken, identityToken);
    QCOMPARE(result.doneWith, DoneWith::Success);
    QCOMPARE(runInContainer(result, {"cat", {"/usr/local/share/tokenfeat.txt"}}), "token");
}

void tst_DevContainer::composeKeepsDollarSigns()
{
    if (HostOsInfo::isLinuxHost())
        QSKIP("docker-compose has been having spurious failures. Skipping on Linux for now.");

    const FilePath configFolder = tempDir / ".devcontainer" / "compose-dollar";
    QVERIFY_RESULT(configFolder.ensureWritableDir());
    QVERIFY_RESULT((configFolder / "docker-compose.yml").writeFileContents(R"yaml(
services:
  app:
    image: alpine:latest
    command: ["sleep", "infinity"]
)yaml"));
    const FilePath configFile = configFolder / "devcontainer.json";
    QVERIFY_RESULT(configFile.writeFileContents(R"json({
        "dockerComposeFile": "docker-compose.yml",
        "service": "app",
        "workspaceFolder": "/tmp",
        "containerEnv": { "PRICE": "a$HOME", "BRACED": "${NOT_SET}" }
    })json"));

    const UpResult result = up(configFile);
    const auto guard = qScopeGuard([&] { down(result); });
    QCOMPARE(result.doneWith, DoneWith::Success);

    // As with "docker run -e", the values reach the container as they are written.
    QCOMPARE(runInContainer(result, {"printenv", {"PRICE"}}), "a$HOME");
    QCOMPARE(runInContainer(result, {"printenv", {"BRACED"}}), "${NOT_SET}");
}

void tst_DevContainer::dockerHubFeature()
{
    const FilePath configFolder = tempDir / ".devcontainer" / "docker-hub";
    QVERIFY_RESULT(configFolder.ensureWritableDir());
    const FilePath configFile = configFolder / "devcontainer.json";
    QVERIFY_RESULT(configFile.writeFileContents(R"json({
        "image": "alpine:latest",
        "features": { "docker.io/library/hello-world:latest": {} }
    })json"));

    // hello-world is an image, not a feature. Getting as far as its manifest shows that the
    // registry API of Docker Hub was reached and access was granted.
    const UpResult result = up(configFile);
    const auto guard = qScopeGuard([&] { down(result); });
    QCOMPARE(result.doneWith, DoneWith::Error);
    QVERIFY2(logMessages.contains("does not contain a feature layer"), qPrintable(logMessages));
}

QTEST_GUILESS_MAIN(tst_DevContainer)

#include "tst_devcontainer.moc"
