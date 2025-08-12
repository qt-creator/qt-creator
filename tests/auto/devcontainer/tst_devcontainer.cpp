// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "devcontainer/devcontainerfeature.h"
#include <devcontainer/devcontainer.h>
#include <devcontainer/devcontainerconfig.h>

#include <utils/qtcprocess.h>
#include <utils/stringutils.h>

#include <QTemporaryFile>
#include <QTest>

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
    void readLocalFeature();
};

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
        .dockerCli = "docker",
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
        .dockerCli = "docker",
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
        .dockerCli = "docker",
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
        .dockerCli = "docker",
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
        .dockerCli = "docker",
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
        .dockerCli = "docker",
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
        .dockerCli = "docker",
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
version: '3.8'
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
    volumes:
      - postgres-data:/var/lib/postgresql/data
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
        .dockerCli = "docker",
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

void tst_DevContainer::readLocalFeature()
{
    static const QByteArray devcontainer_feature_json = R"json(
{
    "id": "test-feature",
    "version": "1.0.0",
    "name": "Test Feature",
    "options": {
        "optionA": {
            "type": "string",
            "default": "Hello World",
            "description": "An example option for the test feature."
        },
        "optionB": {
            "type": "boolean",
            "default": true,
            "description": "A boolean option."
        },
        "optionC": {
            "type": "string",
            "description": "An enum option.",
            "enum": ["value1", "value2", "value3"],
            "default": "value2"
        },
        "optionD": {
            "type": "string",
            "description": "An option with proposals",
            "proposals": ["proposal1", "proposal2"],
            "default": "proposal1"
        }
    },
    "init": true,
    "containerEnv": {
        "feature-container-env": "Hello Feature"
    },
}
)json";

    const auto toString = [](const QJsonValue &value) { return value.toString(); };

    Result<DevContainer::Feature> feature
        = DevContainer::Feature::fromJson(devcontainer_feature_json, toString);

    QVERIFY_RESULT(feature);
    QCOMPARE(feature->id, "test-feature");
    QCOMPARE(feature->version, "1.0.0");
    QCOMPARE(feature->name, "Test Feature");
    QCOMPARE(feature->options.size(), 4);
    QCOMPARE(feature->options.value("optionA").type, "string");
    QCOMPARE(feature->options.value("optionA").defaultValue, "Hello World");
    QCOMPARE(feature->options.value("optionA").description, "An example option for the test feature.");
    QCOMPARE(feature->options.value("optionB").type, "boolean");
    QCOMPARE(feature->options.value("optionB").defaultValue, true);
    QCOMPARE(feature->options.value("optionB").description, "A boolean option.");
    QCOMPARE(feature->options.value("optionC").type, "string");
    QCOMPARE(feature->options.value("optionC").defaultValue, "value2");
    QCOMPARE(feature->options.value("optionC").description, "An enum option.");
    QCOMPARE(
        feature->options.value("optionC").enumValues,
        QStringList() << "value1"
                      << "value2"
                      << "value3");
    QCOMPARE(feature->options.value("optionD").type, "string");
    QCOMPARE(feature->options.value("optionD").defaultValue, "proposal1");
    QCOMPARE(feature->options.value("optionD").description, "An option with proposals");
    QCOMPARE(
        feature->options.value("optionD").proposals,
        QStringList() << "proposal1"
                      << "proposal2");

    QCOMPARE(feature->init, true);
    QCOMPARE(feature->containerEnv.size(), 1);
    QCOMPARE(feature->containerEnv.at("feature-container-env"), "Hello Feature");
}

QTEST_GUILESS_MAIN(tst_DevContainer)

#include "tst_devcontainer.moc"
