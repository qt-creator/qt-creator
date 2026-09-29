// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <devcontainer/devcontainer.h>
#include <devcontainer/devcontainerconfig.h>
#include <devcontainer/devcontainerfeature.h>

#include <utils/environment.h>
#include <utils/hostosinfo.h>
#include <utils/qtcprocess.h>

#include <QJsonDocument>
#include <QTemporaryDir>
#include <QTest>

using namespace Utils;

class tst_DevContainerFeatures : public QObject
{
    Q_OBJECT

    QTemporaryDir temporaryDir;
    const FilePath tempDir = FilePath::fromString(temporaryDir.path());

private slots:
    void initTestCase() { QVERIFY(temporaryDir.isValid()); }

    void readLocalFeature();
    void featureReference_data();
    void featureReference();
    void featureDependencyOptions();
    void featureOptionEnvName_data();
    void featureOptionEnvName();
    void featureEnvFile();
    void featureInstallOrder_data();
    void featureInstallOrder();
    void featuresDockerfile();
    void registryCredentialSource_data();
    void registryCredentialSource();
    void malformedDockerConfig();
    void isLocalRegistry_data();
    void isLocalRegistry();
    void registryApiHost_data();
    void registryApiHost();
    void isSameOrigin_data();
    void isSameOrigin();
    void mergeMounts();
    void parseCredentialHelperOutput();
    void mergedCustomizations();
};

void tst_DevContainerFeatures::readLocalFeature()
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

void tst_DevContainerFeatures::featureReference_data()
{
    using Type = DevContainer::FeatureReference::Type;
    QTest::addColumn<QString>("reference");
    QTest::addColumn<bool>("valid");
    QTest::addColumn<int>("type");
    QTest::addColumn<QString>("id");
    QTest::addColumn<QString>("resource");
    QTest::addColumn<QString>("tag");
    QTest::addColumn<QString>("digest");

    const QString digest = "sha256:" + QString(64, 'a');

    QTest::newRow("oci-tag") << "ghcr.io/devcontainers/features/node:1" << true << int(Type::Oci)
                             << "node" << "ghcr.io/devcontainers/features/node" << "1" << "";
    QTest::newRow("oci-latest") << "ghcr.io/devcontainers/features/node" << true << int(Type::Oci)
                                << "node" << "ghcr.io/devcontainers/features/node" << "latest"
                                << "";
    QTest::newRow("oci-digest") << "ghcr.io/devcontainers/features/node@" + digest << true
                                << int(Type::Oci) << "node"
                                << "ghcr.io/devcontainers/features/node" << "" << digest;
    QTest::newRow("oci-port") << "localhost:5000/my/feature:2.1" << true << int(Type::Oci)
                              << "feature" << "localhost:5000/my/feature" << "2.1" << "";
    QTest::newRow("oci-uppercase") << "ghcr.io/Owner/Features/Tool:1" << true << int(Type::Oci)
                                   << "tool" << "ghcr.io/owner/features/tool" << "1" << "";
    QTest::newRow("tarball") << "https://example.com/x/devcontainer-feature-go.tgz" << true
                             << int(Type::Tarball) << "go"
                             << "https://example.com/x/devcontainer-feature-go.tgz" << "latest"
                             << "";
    QTest::newRow("tarball-http-local")
        << "http://localhost:8000/devcontainer-feature-go.tgz" << true << int(Type::Tarball) << "go"
        << "http://localhost:8000/devcontainer-feature-go.tgz" << "latest" << "";
    QTest::newRow("tarball-http-remote")
        << "http://example.com/x/devcontainer-feature-go.tgz" << false << 0 << "" << "" << "" << "";
    QTest::newRow("local") << "./myfeature" << true << int(Type::Local) << "myfeature"
                           << "./myfeature" << "latest" << "";
    QTest::newRow("local-parent") << "../shared/other" << true << int(Type::Local) << "other"
                                  << "../shared/other" << "latest" << "";

    QTest::newRow("empty") << "" << false << 0 << "" << "" << "" << "";
    QTest::newRow("bare-id") << "node" << false << 0 << "" << "" << "" << "";
    QTest::newRow("github-shorthand") << "owner/repo/feature@1" << false << 0 << "" << "" << ""
                                      << "";
    QTest::newRow("absolute") << "/abs/feature" << false << 0 << "" << "" << "" << "";
    QTest::newRow("tarball-name") << "https://example.com/feature.tgz" << false << 0 << "" << ""
                                  << "" << "";
    QTest::newRow("bad-digest") << "ghcr.io/a/b@sha256:xyz" << false << 0 << "" << "" << "" << "";
}

void tst_DevContainerFeatures::featureReference()
{
    QFETCH(QString, reference);
    QFETCH(bool, valid);

    const FilePath configFolder = FilePath::fromString("/work/.devcontainer");
    const Result<DevContainer::FeatureReference> ref
        = DevContainer::FeatureReference::parse(reference, configFolder);
    QCOMPARE(bool(ref), valid);
    if (!valid)
        return;

    QFETCH(int, type);
    QFETCH(QString, id);
    QFETCH(QString, resource);
    QFETCH(QString, tag);
    QFETCH(QString, digest);
    QCOMPARE(int(ref->type), type);
    QCOMPARE(ref->id, id);
    QCOMPARE(ref->resource(), resource);
    QCOMPARE(ref->tag, tag);
    QCOMPARE(ref->digest, digest);
    if (ref->type == DevContainer::FeatureReference::Type::Local)
        QCOMPARE(ref->localPath, configFolder.resolvePath(reference).cleanPath());
}

void tst_DevContainerFeatures::featureDependencyOptions()
{
    static const QByteArray jsonData = R"json(
    {
        "image": "alpine:latest",
        "features": {
            "ghcr.io/devcontainers/features/node:1": "18",
            "ghcr.io/devcontainers/features/git:1": true,
            "ghcr.io/devcontainers/features/python:1": {
                "version": "3.12",
                "installTools": false,
                "folder": "${localWorkspaceFolderBasename}"
            }
        }
    })json";

    DevContainer::InstanceConfig instanceConfig{
        .workspaceFolder = FilePath::fromString("/some/project"),
        .configFilePath = FilePath::fromString("/some/project/.devcontainer/devcontainer.json")};

    const Result<DevContainer::Config> config
        = DevContainer::Config::fromJson(jsonData, [instanceConfig](const QJsonValue &value) {
              return instanceConfig.jsonToString(value);
          });
    QVERIFY_RESULT(config);
    QCOMPARE(config->common.features.size(), 3);

    QMap<QString, QJsonObject> options;
    for (const DevContainer::FeatureDependency &dep : config->common.features)
        options.insert(dep.id, dep.options);

    QCOMPARE(options.value("ghcr.io/devcontainers/features/node:1"), QJsonObject({{"version", "18"}}));
    QCOMPARE(options.value("ghcr.io/devcontainers/features/git:1"), QJsonObject());
    QCOMPARE(
        options.value("ghcr.io/devcontainers/features/python:1"),
        QJsonObject({{"version", "3.12"}, {"installTools", false}, {"folder", "project"}}));

    const Result<DevContainer::Config> invalid = DevContainer::Config::fromJson(
        R"({"image": "alpine", "features": {"ghcr.io/a/b:1": 42}})",
        [](const QJsonValue &value) { return value.toString(); });
    QVERIFY(!invalid);
}

void tst_DevContainerFeatures::featureOptionEnvName_data()
{
    QTest::addColumn<QString>("option");
    QTest::addColumn<QString>("envName");

    QTest::newRow("simple") << "version" << "VERSION";
    QTest::newRow("camel") << "installZsh" << "INSTALLZSH";
    QTest::newRow("dash") << "my-option" << "MY_OPTION";
    QTest::newRow("dot") << "a.b" << "A_B";
    QTest::newRow("leading-digits") << "12abc" << "_ABC";
    QTest::newRow("leading-underscores") << "__x" << "_X";
    QTest::newRow("umlaut") << "gr\u00fc\u00dfe" << "GR__E";
}

void tst_DevContainerFeatures::featureOptionEnvName()
{
    QFETCH(QString, option);
    QFETCH(QString, envName);
    QCOMPARE(DevContainer::featureOptionEnvName(option), envName);
}

static DevContainer::ResolvedFeature makeFeature(
    const QString &reference,
    const QStringList &installsAfter = {},
    const QStringList &dependsOn = {},
    const QJsonObject &userOptions = {},
    const QStringList &legacyIds = {})
{
    DevContainer::ResolvedFeature f;
    f.reference = *DevContainer::FeatureReference::parse(reference, FilePath::fromString("/cfg"));
    f.userOptions = userOptions;
    f.feature.id = f.reference.id;
    f.feature.installsAfter = installsAfter;
    f.feature.legacyIds = legacyIds;
    for (const QString &dep : dependsOn)
        f.feature.dependsOn.append(DevContainer::FeatureDependency{dep, {}});
    return f;
}

void tst_DevContainerFeatures::featureEnvFile()
{
    DevContainer::ResolvedFeature f = makeFeature(
        "ghcr.io/a/features/x:1",
        {},
        {},
        {{"greeting", "say \"hi\" to $USER"},
         {"count", 3},
         {"extra", true},
         {"big", 1234567},
         {"fraction", 0.1234567}});
    f.feature.options["greeting"].defaultValue = "hello";
    f.feature.options["flag"].defaultValue = false;
    f.feature.options["my-path"].defaultValue = "/usr/local";
    f.feature.options["nodefault"] = {};

    const QByteArray expected = "BIG=\"1234567\"\n"
                                "COUNT=\"3\"\n"
                                "EXTRA=\"true\"\n"
                                "FLAG=\"false\"\n"
                                "FRACTION=\"0.1234567\"\n"
                                "GREETING=\"say \\\"hi\\\" to \\$USER\"\n"
                                "MY_PATH=\"/usr/local\"\n";
    QCOMPARE(DevContainer::featureEnvFileContents(f), expected);

    if (HostOsInfo::isWindowsHost())
        QSKIP("Sourcing the file needs a POSIX shell.");

    // The file is sourced by the shell, so the value must come back unchanged.
    const FilePath envFile = tempDir / "features.env";
    QVERIFY_RESULT(envFile.writeFileContents(expected));
    Process sh;
    sh.setCommand({"/bin/sh", {"-c", ". \"$0\" && printf %s \"$GREETING\"", envFile.path()}});
    sh.runBlocking();
    QCOMPARE(sh.cleanedStdOut(), "say \"hi\" to $USER");
}

using FeatureList = std::vector<DevContainer::ResolvedFeature>;
Q_DECLARE_METATYPE(FeatureList)

void tst_DevContainerFeatures::featureInstallOrder_data()
{
    QTest::addColumn<FeatureList>("features");
    QTest::addColumn<QStringList>("overrideOrder");
    QTest::addColumn<QStringList>("expected");

    const QString r = "ghcr.io/o/f/";

    QTest::newRow("alphabetical")
        << FeatureList{makeFeature(r + "c"), makeFeature(r + "a"), makeFeature(r + "b")}
        << QStringList() << QStringList{"a", "b", "c"};

    QTest::newRow("installsAfter")
        << FeatureList{makeFeature(r + "a", {r + "c"}), makeFeature(r + "b"), makeFeature(r + "c")}
        << QStringList() << QStringList{"b", "c", "a"};

    QTest::newRow("installsAfter-with-version")
        << FeatureList{makeFeature(r + "a:1", {r + "b:2"}), makeFeature(r + "b:1")} << QStringList()
        << QStringList{"b", "a"};

    QTest::newRow("installsAfter-absent")
        << FeatureList{makeFeature(r + "a", {r + "zzz"}), makeFeature(r + "b")} << QStringList()
        << QStringList{"a", "b"};

    QTest::newRow("installsAfter-legacyId") << FeatureList{
        makeFeature(r + "a", {r + "oldname"}),
        makeFeature(
            r + "b",
            {},
            {},
            {},
            {"oldname"})} << QStringList() << QStringList{"b", "a"};

    QTest::newRow("dependsOn-chain") << FeatureList{
        makeFeature(r + "a", {}, {r + "b:1"}),
        makeFeature(r + "b", {}, {r + "c"}),
        makeFeature(
            r
            + "c")} << QStringList() << QStringList{"c", "b", "a"};

    QTest::newRow("override")
        << FeatureList{makeFeature(r + "a"), makeFeature(r + "m"), makeFeature(r + "z")}
        << QStringList{r + "z", r + "a"} << QStringList{"z", "a", "m"};

    // The priority only picks among the features that are ready in a round, it does not pull
    // the dependencies of a feature forward.
    QTest::newRow("override-keeps-dependsOn")
        << FeatureList{makeFeature(r + "a", {}, {r + "b"}), makeFeature(r + "b"), makeFeature(r + "c")}
        << QStringList{r + "a"} << QStringList{"b", "c", "a"};

    QTest::newRow("duplicates")
        << FeatureList{makeFeature(r + "a"), makeFeature(r + "a"), makeFeature(r + "b")}
        << QStringList() << QStringList{"a", "b"};

    QTest::newRow("same-feature-other-options") << FeatureList{
        makeFeature(r + "a", {}, {}, {{"v", "1"}}),
        makeFeature(
            r + "a",
            {},
            {},
            {{"v", "2"}})} << QStringList() << QStringList{"a", "a"};

    QTest::newRow("cycle")
        << FeatureList{makeFeature(r + "a", {r + "b"}), makeFeature(r + "b", {r + "a"}), makeFeature(r + "c")}
        << QStringList() << QStringList();

    QTest::newRow("missing-dependency")
        << FeatureList{makeFeature(r + "a", {}, {r + "b"})} << QStringList() << QStringList();
}

void tst_DevContainerFeatures::featureInstallOrder()
{
    QFETCH(FeatureList, features);
    QFETCH(QStringList, overrideOrder);
    QFETCH(QStringList, expected);

    const Result<FeatureList> ordered = DevContainer::featureInstallOrder(
        features, overrideOrder.isEmpty() ? std::nullopt : std::make_optional(overrideOrder));

    if (expected.isEmpty()) {
        QVERIFY(!ordered);
        return;
    }
    QVERIFY_RESULT(ordered);
    QStringList ids;
    for (const DevContainer::ResolvedFeature &f : *ordered)
        ids << f.reference.id;
    QCOMPARE(ids, expected);
}

void tst_DevContainerFeatures::featuresDockerfile()
{
    DevContainer::ResolvedFeature a = makeFeature("ghcr.io/o/f/a:1");
    a.folder = FilePath::fromString("/ctx/dev-container-features/0_a");
    a.feature.containerEnv = {{"PATH", "/opt/a/bin:${PATH}"}, {"QUOTED", "say \"x\""}};
    DevContainer::ResolvedFeature b = makeFeature("./b");
    b.folder = FilePath::fromString("/ctx/dev-container-features/1_b");

    const QString dockerfile = DevContainer::featuresDockerfile(
        {a, b}, {.containerUser = "root", .remoteUser = "dev", .imageUser = "dev"});

    const QStringList lines = dockerfile.split('\n');
    QCOMPARE(lines.value(0), "ARG _DEV_CONTAINERS_BASE_IMAGE=placeholder");
    QCOMPARE(lines.value(1), "FROM $_DEV_CONTAINERS_BASE_IMAGE");
    QCOMPARE(lines.value(2), "USER root");
    QVERIFY(lines.contains("ENV PATH=\"/opt/a/bin:${PATH}\""));
    QVERIFY(lines.contains("ENV QUOTED=\"say \\\"x\\\"\""));

    const qsizetype envA = dockerfile.indexOf("ENV PATH=");
    const qsizetype runA = dockerfile.indexOf("RUN cd /tmp/dev-container-features/0_a ");
    const qsizetype runB = dockerfile.indexOf("RUN cd /tmp/dev-container-features/1_b ");
    QVERIFY(envA >= 0 && runA > envA && runB > runA);
    QVERIFY(dockerfile.endsWith("\nUSER dev\n"));
}

void tst_DevContainerFeatures::registryCredentialSource_data()
{
    QTest::addColumn<QByteArray>("config");
    QTest::addColumn<QString>("registry");
    QTest::addColumn<QString>("username");
    QTest::addColumn<QString>("secret");
    QTest::addColumn<QString>("helper");
    QTest::addColumn<QString>("serverUrl");

    const QByteArray auth = QByteArray("me:p:w").toBase64();

    QTest::newRow("auths") << QByteArray(
        R"({"auths": {"registry.example.com": {"auth": ")" + auth + R"("}}})")
                           << "registry.example.com" << "me" << "p:w" << ""
                           << "registry.example.com";
    QTest::newRow("auths-url-key")
        << QByteArray(
               R"({"auths": {"https://Registry.Example.com/v1/": {"auth": ")" + auth + R"("}}})")
        << "registry.example.com" << "me" << "p:w" << "" << "https://Registry.Example.com/v1/";
    QTest::newRow("identitytoken")
        << QByteArray(R"({"auths": {"registry.example.com": {"identitytoken": "tok"}}})")
        << "registry.example.com" << "<token>" << "tok" << "" << "registry.example.com";
    QTest::newRow("credHelpers-win")
        << QByteArray(
               R"({"auths": {"registry.example.com": {"auth": ")" + auth
               + R"("}}, "credHelpers": {"registry.example.com": "gcr"}})")
        << "registry.example.com" << "" << "" << "gcr" << "registry.example.com";
    QTest::newRow("credsStore") << QByteArray(
        R"({"auths": {"registry.example.com": {}}, "credsStore": "desktop"})")
                                << "registry.example.com" << "" << "" << "desktop"
                                << "registry.example.com";
    QTest::newRow("auths-beat-credsStore")
        << QByteArray(
               R"({"auths": {"registry.example.com": {"auth": ")" + auth
               + R"("}}, "credsStore": "desktop"})")
        << "registry.example.com" << "me" << "p:w" << "" << "registry.example.com";
    QTest::newRow("other-registry")
        << QByteArray(R"({"auths": {"other.example.com": {"auth": ")" + auth + R"("}}})")
        << "registry.example.com" << "" << "" << "" << "registry.example.com";
    QTest::newRow("port") << QByteArray(
        R"({"auths": {"localhost:5000": {"auth": ")" + auth + R"("}}})")
                          << "localhost:5000" << "me" << "p:w" << "" << "localhost:5000";
    QTest::newRow("docker-hub-auths")
        << QByteArray(R"({"auths": {"https://index.docker.io/v1/": {"auth": ")" + auth + R"("}}})")
        << "docker.io" << "me" << "p:w" << "" << "https://index.docker.io/v1/";
    QTest::newRow("docker-hub-credsStore")
        << QByteArray(R"({"auths": {"https://index.docker.io/v1/": {}}, "credsStore": "desktop"})")
        << "docker.io" << "" << "" << "desktop" << "https://index.docker.io/v1/";
    QTest::newRow("docker-hub-no-entry")
        << QByteArray(R"({"credsStore": "desktop"})") << "docker.io"
        << "" << "" << "desktop" << "https://index.docker.io/v1/";
    QTest::newRow("no-config") << QByteArray() << "registry.example.com" << "" << "" << ""
                               << "registry.example.com";
}

void tst_DevContainerFeatures::registryCredentialSource()
{
    QFETCH(QByteArray, config);
    QFETCH(QString, registry);
    QFETCH(QString, username);
    QFETCH(QString, secret);
    QFETCH(QString, helper);
    QFETCH(QString, serverUrl);

    const FilePath configDir = tempDir / "dockerconfig" / QTest::currentDataTag();
    QVERIFY_RESULT(configDir.ensureWritableDir());
    (configDir / "config.json").removeFile();
    if (!config.isEmpty())
        QVERIFY_RESULT((configDir / "config.json").writeFileContents(config));

    Environment env;
    env.set("DOCKER_CONFIG", configDir.path());
    const DevContainer::RegistryCredentialSource source
        = DevContainer::registryCredentialSource(registry, env);

    QCOMPARE(bool(source.credentials), !username.isEmpty());
    if (source.credentials) {
        QCOMPARE(source.credentials->username, username);
        QCOMPARE(source.credentials->secret, secret);
    }
    QCOMPARE(source.helper, helper);
    QCOMPARE(source.serverUrl, serverUrl);
}

void tst_DevContainerFeatures::parseCredentialHelperOutput()
{
    const Result<DevContainer::RegistryCredentials> ok = DevContainer::parseCredentialHelperOutput(
        R"({"ServerURL": "ghcr.io", "Username": "me", "Secret": "s3cr3t"})");
    QVERIFY_RESULT(ok);
    QCOMPARE(ok->username, "me");
    QCOMPARE(ok->secret, "s3cr3t");

    QVERIFY(!DevContainer::parseCredentialHelperOutput("credentials not found in native keychain"));
}

void tst_DevContainerFeatures::mergedCustomizations()
{
    DevContainer::ResolvedFeature a = makeFeature("ghcr.io/o/f/a");
    a.feature.customizations
        = QJsonDocument::fromJson(
              R"({"qt-creator": {"kits": [{"name": "A"}], "auto-detect-kits": false, "x": 1}})")
              .object();
    DevContainer::ResolvedFeature b = makeFeature("ghcr.io/o/f/b");
    b.feature.customizations
        = QJsonDocument::fromJson(
              R"({"qt-creator": {"kits": [{"name": "B"}], "x": 2}, "vscode": {"extensions": ["e"]}})")
              .object();
    const QJsonObject config
        = QJsonDocument::fromJson(
              R"({"qt-creator": {"kits": [{"name": "C"}], "auto-detect-kits": true}})")
              .object();

    const QJsonObject merged = DevContainer::mergedCustomizations({a, b}, config);
    const QJsonObject expected = QJsonDocument::fromJson(R"({
        "qt-creator": {
            "kits": [{"name": "A"}, {"name": "B"}, {"name": "C"}],
            "auto-detect-kits": true,
            "x": 2
        },
        "vscode": {"extensions": ["e"]}
    })")
                                     .object();
    QCOMPARE(merged, expected);
}

void tst_DevContainerFeatures::malformedDockerConfig()
{
    const FilePath configDir = tempDir / "dockerconfig" / "malformed";
    QVERIFY_RESULT(configDir.ensureWritableDir());
    QVERIFY_RESULT((configDir / "config.json").writeFileContents(R"({"auths": {)"));

    Environment env;
    env.set("DOCKER_CONFIG", configDir.path());
    const DevContainer::RegistryCredentialSource source
        = DevContainer::registryCredentialSource("registry.example.com", env);
    QVERIFY(!source.credentials);
    QVERIFY(source.helper.isEmpty());
    QVERIFY2(source.error.contains("config.json"), qPrintable(source.error));
}

void tst_DevContainerFeatures::isLocalRegistry_data()
{
    QTest::addColumn<QString>("registry");
    QTest::addColumn<bool>("local");

    QTest::newRow("localhost") << "localhost" << true;
    QTest::newRow("localhost-port") << "localhost:5000" << true;
    QTest::newRow("loopback") << "127.0.0.1" << true;
    QTest::newRow("loopback-port") << "127.0.0.1:5000" << true;
    QTest::newRow("ipv6-loopback-port") << "[::1]:5000" << true;
    QTest::newRow("loopback-prefix") << "127.0.0.1.example.com" << false;
    QTest::newRow("localhost-prefix") << "localhost.example.com:5000" << false;
    QTest::newRow("remote") << "ghcr.io" << false;
}

void tst_DevContainerFeatures::isLocalRegistry()
{
    QFETCH(QString, registry);
    QFETCH(bool, local);
    QCOMPARE(DevContainer::isLocalRegistry(registry), local);
}

void tst_DevContainerFeatures::registryApiHost_data()
{
    QTest::addColumn<QString>("registry");
    QTest::addColumn<QString>("apiHost");

    QTest::newRow("docker.io") << "docker.io" << "registry-1.docker.io";
    QTest::newRow("index.docker.io") << "index.docker.io" << "registry-1.docker.io";
    QTest::newRow("ghcr.io") << "ghcr.io" << "ghcr.io";
    QTest::newRow("local") << "localhost:5000" << "localhost:5000";
}

void tst_DevContainerFeatures::registryApiHost()
{
    QFETCH(QString, registry);
    QFETCH(QString, apiHost);
    QCOMPARE(DevContainer::registryApiHost(registry), apiHost);
}

void tst_DevContainerFeatures::isSameOrigin_data()
{
    QTest::addColumn<QUrl>("first");
    QTest::addColumn<QUrl>("second");
    QTest::addColumn<bool>("same");

    QTest::newRow("same") << QUrl("https://r.example/v2/a") << QUrl("https://r.example/b?x=1")
                          << true;
    QTest::newRow("default-port") << QUrl("https://r.example/v2/a")
                                  << QUrl("https://r.example:443/b") << true;
    QTest::newRow("downgrade") << QUrl("https://r.example/v2/a") << QUrl("http://r.example/b")
                               << false;
    QTest::newRow("downgrade-same-port")
        << QUrl("https://r.example/v2/a") << QUrl("http://r.example:443/b") << false;
    QTest::newRow("other-port") << QUrl("http://127.0.0.1:5000/a")
                                << QUrl("http://127.0.0.1:5001/a") << false;
    QTest::newRow("other-host") << QUrl("https://r.example/a") << QUrl("https://s.example/a")
                                << false;
}

void tst_DevContainerFeatures::isSameOrigin()
{
    QFETCH(QUrl, first);
    QFETCH(QUrl, second);
    QFETCH(bool, same);
    QCOMPARE(DevContainer::isSameOrigin(first, second), same);
}

void tst_DevContainerFeatures::mergeMounts()
{
    using DevContainer::Mount;
    using DevContainer::MountType;
    const std::vector<std::variant<Mount, QString>> mounts{
        Mount{MountType::Volume, QString("from-feature"), "/x"},
        Mount{MountType::Bind, QString("/host"), "/y"},
        QString("type=volume,source=from-config,dst=/x"),
        QString("type=tmpfs"),
        QString("type=tmpfs")};

    const std::vector<std::variant<Mount, QString>> merged = DevContainer::mergeMounts(mounts);
    QStringList result;
    for (const auto &mount : merged) {
        result << std::visit(
            [](const auto &m) -> QString {
                if constexpr (std::is_same_v<std::decay_t<decltype(m)>, QString>)
                    return m;
                else
                    return m.source.value_or(QString()) + "->" + m.target;
            },
            mount);
    }
    // The last mount at /x wins, mounts without a target are all kept.
    QCOMPARE(
        result,
        QStringList(
            {"/host->/y", "type=volume,source=from-config,dst=/x", "type=tmpfs", "type=tmpfs"}));
}

QTEST_GUILESS_MAIN(tst_DevContainerFeatures)

#include "tst_devcontainerfeatures.moc"
