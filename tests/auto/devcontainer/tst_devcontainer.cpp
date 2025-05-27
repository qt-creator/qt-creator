// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <devcontainer/devcontainer.h>
#include <devcontainer/devcontainerconfig.h>

#include <utils/stringutils.h>

#include <QtTest>

class tst_DevContainer : public QObject
{
    Q_OBJECT

private slots:
    void readConfig();
    void testCommands();
    void upDockerfile();
};

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
    "shutdownAction": "none"
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
}
    )json";

    Utils::Result<DevContainer::Config> devContainer = DevContainer::Config::fromJson(jsonData);
    QVERIFY_RESULT(devContainer);
    QVERIFY(devContainer->common.name);
    QCOMPARE(*devContainer->common.name, "Minimum spec container (x86_64)");
    QVERIFY(devContainer->containerConfig);
    QCOMPARE(devContainer->containerConfig->index(), 0);
    qDebug() << "Parsed DevContainer:" << *devContainer;
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

    Utils::Result<DevContainer::Config> devContainer = DevContainer::Config::fromJson(jsonData);
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

    qDebug() << "Parsed DevContainer:" << *devContainer;
}

void tst_DevContainer::upDockerfile()
{
    QTemporaryFile dockerFile;
    dockerFile.setFileTemplate(QDir::tempPath() + "/DockerfileXXXXXX");
    QVERIFY(dockerFile.open());
    dockerFile.write(R"(
FROM alpine:latest
    )");
    dockerFile.flush();

    DevContainer::Config config;
    DevContainer::DockerfileContainer dockerFileConfig {
        //.appPort = 10,
        .dockerfile = dockerFile.fileName(),
        .context = std::nullopt,
        .buildOptions = DevContainer::BuildOptions{
            .target = "test",
            .args = {{"arg1", "value1"}, {"arg2", "value2"}},
            .cacheFrom = QStringList{"cache1", "cache2"},
            .options = QStringList{"--option1", "--option2"},
        },
    };
    dockerFileConfig.appPort = QList<std::variant<int, QString>>{8080, "80:9090"}; // Example port
    config.containerConfig = dockerFileConfig;
    config.common.name = "Test Dockerfile";
    config.common.forwardPorts = {8080, "127.0.0.1:9090"};

    qDebug() << "DevContainer Config:" << config;

    std::unique_ptr<DevContainer::Instance> instance = DevContainer::Instance::fromConfig(config);

    DevContainer::InstanceConfig instanceConfig{
        .dockerCli = "docker",
        .dockerComposeCli = "docker-compose",
        .workspaceFolder = Utils::FilePath::fromUserInput(QDir::tempPath()),
        .configFilePath = Utils::FilePath::fromUserInput(QDir::tempPath()) / "devcontainer.json",
    };

    Utils::Result<Tasking::Group> recipe = instance->upRecipe(instanceConfig);
    QVERIFY_RESULT(recipe);
    Tasking::TaskTree::runBlocking(*recipe);
}

QTEST_GUILESS_MAIN(tst_DevContainer)

#include "tst_devcontainer.moc"
