// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "androidconfigurations.h"
#include "androidconstants.h"
#include "androidtr.h"
#include "androidutils.h"
#include "javalanguageserver.h"

#include <languageclient/client.h>
#include <languageclient/languageclientinterface.h>
#include <languageclient/languageclientsettings.h>
#include <languageclient/languageclientutils.h>

#include <projectexplorer/devicesupport/devicekitaspects.h>
#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/buildsystem.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectnodes.h>

#include <qtsupport/qtkitaspect.h>

#include <utils/environment.h>
#include <utils/layoutbuilder.h>
#include <utils/mimeconstants.h>
#include <utils/pathchooser.h>
#include <utils/temporarydirectory.h>
#include <utils/variablechooser.h>

#include <QLineEdit>
#include <QXmlStreamWriter>

using namespace LanguageClient;
using namespace ProjectExplorer;
using namespace Utils;

constexpr char languageServerKey[] = "languageServer";

namespace Android::Internal {

class JLSSettings final : public StdIOSettings
{
public:
    JLSSettings();

    bool applyFromSettingsWidget(QWidget *widget) final;
    QWidget *createSettingsWidget(QWidget *parent) const final;
    bool isValid() const final;
    void toMap(Store &map) const final;
    void fromMap(const Store &map) final;
    BaseSettings *copy() const final;
    Client *createClient(BaseClientInterface *interface) const final;
    BaseClientInterface *createInterface(BuildConfiguration *) const final;

    FilePath m_languageServer;

private:
    JLSSettings(const JLSSettings &other) = default;
};

class JLSSettingsWidget : public QWidget
{
public:
    JLSSettingsWidget(const JLSSettings *settings, QWidget *parent);

    QString name() const { return m_name->text(); }
    FilePath java() const { return m_java->filePath(); }
    FilePath languageServer() const { return m_ls->filePath(); }

private:
    QLineEdit *m_name = nullptr;
    PathChooser *m_java = nullptr;
    PathChooser *m_ls = nullptr;
};

JLSSettingsWidget::JLSSettingsWidget(const JLSSettings *settings, QWidget *parent)
    : QWidget(parent)
    , m_name(new QLineEdit(settings->m_name, this))
    , m_java(new PathChooser(this))
    , m_ls(new PathChooser(this))
{
    auto chooser = new VariableChooser(this);
    chooser->addSupportedWidget(m_name);

    m_java->setExpectedKind(PathChooser::ExistingCommand);
    m_java->setFilePath(settings->m_executable);

    m_ls->setExpectedKind(PathChooser::File);
    m_ls->lineEdit()->setPlaceholderText(Tr::tr("Path to equinox launcher jar"));
    m_ls->setPromptDialogFilter("org.eclipse.equinox.launcher_*.jar");
    m_ls->setFilePath(settings->m_languageServer);

    using namespace Layouting;
    Form {
        Tr::tr("Name:"), m_name, br,
        Tr::tr("Java:"), m_java, br,
        Tr::tr("Java Language Server:"), m_ls, br,
    }.attachTo(this);
}

JLSSettings::JLSSettings()
{
    m_settingsTypeId = Constants::JLS_SETTINGS_ID;
    m_name = "Java Language Server";
    m_startBehavior = RequiresProject;
    m_languageFilter.mimeTypes = QStringList(Utils::Constants::JAVA_MIMETYPE);
    const FilePath &javaPath = Environment::systemEnvironment().searchInPath("java");
    if (javaPath.exists())
        m_executable = javaPath;
}

bool JLSSettings::applyFromSettingsWidget(QWidget *widget)
{
    bool changed = false;
    auto jlswidget = static_cast<JLSSettingsWidget *>(widget);
    changed |= m_name != jlswidget->name();
    m_name = jlswidget->name();

    changed |= m_languageServer != jlswidget->languageServer();
    m_languageServer = jlswidget->languageServer();

    changed |= m_executable != jlswidget->java();
    m_executable = jlswidget->java();

    QString arguments = "-Declipse.application=org.eclipse.jdt.ls.core.id1 "
                        "-Dosgi.bundles.defaultStartLevel=4 "
                        "-Declipse.product=org.eclipse.jdt.ls.core.product "
                        "-Dlog.level=WARNING "
                        "-noverify "
                        "-Xmx1G "
                        "-jar \"%1\" "
                        "-configuration \"%2\"";

    QDir configDir = m_languageServer.toFileInfo().absoluteDir();
    if (configDir.exists()) {
        configDir.cdUp();
        if constexpr (HostOsInfo::hostOs() == OsTypeWindows)
            configDir.cd("config_win");
        else if constexpr (HostOsInfo::hostOs() == OsTypeLinux)
            configDir.cd("config_linux");
        else if constexpr (HostOsInfo::hostOs() == OsTypeMac)
            configDir.cd("config_mac");
    }
    if (configDir.exists()) {
        arguments = arguments.arg(m_languageServer.path(), configDir.absolutePath());
        changed |= m_arguments != arguments;
        m_arguments = arguments;
    }
    return changed;
}

QWidget *JLSSettings::createSettingsWidget(QWidget *parent) const
{
    return new JLSSettingsWidget(this, parent);
}

bool JLSSettings::isValid() const
{
    return StdIOSettings::isValid() && !m_languageServer.isEmpty();
}

void JLSSettings::toMap(Store &map) const
{
    StdIOSettings::toMap(map);
    map.insert(languageServerKey, m_languageServer.toSettings());
}

void JLSSettings::fromMap(const Store &map)
{
    StdIOSettings::fromMap(map);
    m_languageServer = FilePath::fromSettings(map[languageServerKey]);
}

BaseSettings *JLSSettings::copy() const
{
    return new JLSSettings(*this);
}

class JLSInterface : public StdIOClientInterface
{
public:
    QString workspaceDir() const { return m_workspaceDir.path().path(); }

private:
    TemporaryDirectory m_workspaceDir = TemporaryDirectory("QtCreator-jls-XXXXXX");
};

BaseClientInterface *JLSSettings::createInterface(BuildConfiguration *) const
{
    auto interface = new JLSInterface();
    CommandLine cmd{m_executable, arguments(), CommandLine::Raw};
    cmd.addArgs({"-data", interface->workspaceDir()});
    interface->setCommandLine(cmd);
    return interface;
}

class JLSClient : public Client
{
public:
    using Client::Client;

    void executeCommand(const LanguageServerProtocol::Command &command) override;
    void setCurrentBuildConfiguration(BuildConfiguration *bc) override;
    void updateProjectFiles();
};

void JLSClient::executeCommand(const LanguageServerProtocol::Command &command)
{
    if (command.command() == "java.apply.workspaceEdit") {
        const QJsonArray arguments = command.arguments().value_or(QJsonArray());
        for (const QJsonValue &argument : arguments) {
            if (!argument.isObject())
                continue;
            LanguageServerProtocol::WorkspaceEdit edit(argument.toObject());
            if (edit.isValid())
                LanguageClient::applyWorkspaceEdit(this, edit);
        }
    } else {
        Client::executeCommand(command);
    }
}

void JLSClient::setCurrentBuildConfiguration(BuildConfiguration *bc)
{
    Client::setCurrentBuildConfiguration(bc);
    QTC_ASSERT(bc, return);
    updateProjectFiles();

    connect(bc->buildSystem(), &BuildSystem::parsingStarted,
            this, &JLSClient::updateProjectFiles);
    connect(bc->project(), &Project::activeBuildConfigurationChanged, this,
            [this](BuildConfiguration *active) {
                if (active == buildConfiguration())
                    updateProjectFiles();
            });
}

static void generateProjectFile(const FilePath &projectDir,
                                const QString &qtSrc,
                                const QString &projectName)
{
    const FilePath projectFilePath = projectDir.pathAppended(".project");
    QFile projectFile(projectFilePath.toFSPathString());
    if (projectFile.open(QFile::Truncate | QFile::WriteOnly)) {
        QXmlStreamWriter writer(&projectFile);
        writer.setAutoFormatting(true);
        writer.writeStartDocument();
        writer.writeComment("Autogenerated by Qt Creator. "
                            "Changes to this file will not be taken into account.");
        writer.writeStartElement("projectDescription");
        writer.writeTextElement("name", projectName);
        writer.writeStartElement("natures");
        writer.writeTextElement("nature", "org.eclipse.jdt.core.javanature");
        writer.writeEndElement(); // natures
        writer.writeStartElement("linkedResources");
        writer.writeStartElement("link");
        writer.writeTextElement("name", "qtSrc");
        writer.writeTextElement("type", "2");
        writer.writeTextElement("location", qtSrc);
        writer.writeEndElement(); // link
        writer.writeEndElement(); // linkedResources
        writer.writeEndElement(); // projectDescription
        writer.writeEndDocument();
        projectFile.close();
    }
}

static void generateClassPathFile(const FilePath &projectDir,
                                  const FilePath &sourceDir,
                                  const FilePaths &libs)
{
    const FilePath classPathFilePath = projectDir.pathAppended(".classpath");
    QFile classPathFile(classPathFilePath.toFSPathString());
    if (classPathFile.open(QFile::Truncate | QFile::WriteOnly)) {
        QXmlStreamWriter writer(&classPathFile);
        writer.setAutoFormatting(true);
        writer.writeStartDocument();
        writer.writeComment("Autogenerated by Qt Creator. "
                            "Changes to this file will not be taken into account.");
        writer.writeStartElement("classpath");
        writer.writeEmptyElement("classpathentry");
        writer.writeAttribute("kind", "src");
        writer.writeAttribute("path", sourceDir.toUserOutput());
        writer.writeEmptyElement("classpathentry");
        writer.writeAttribute("kind", "src");
        writer.writeAttribute("path", "qtSrc");
        for (const FilePath &lib : libs) {
            writer.writeEmptyElement("classpathentry");
            writer.writeAttribute("kind", "lib");
            writer.writeAttribute("path", lib.toUserOutput());
        }
        writer.writeEndElement(); // classpath
        writer.writeEndDocument();
        classPathFile.close();
    }
}

void JLSClient::updateProjectFiles()
{
    QTC_ASSERT(buildConfiguration(), return);

    Kit *kit = buildConfiguration()->kit();
    if (RunDeviceTypeKitAspect::deviceTypeId(kit) != Android::Constants::ANDROID_DEVICE_TYPE)
        return;

    if (ProjectNode *node = project()->findNodeForBuildKey(buildConfiguration()->activeBuildKey())) {
        QtSupport::QtVersion *version = QtSupport::QtKitAspect::qtVersion(kit);
        if (!version)
            return;
        const FilePath qtSrc = version->prefix().pathAppended("src/android/java/src");
        const FilePath projectDir = project()->rootProjectDirectory();
        if (!projectDir.exists())
            return;
        const FilePath packageSourceDir = FilePath::fromVariant(
            node->data(Constants::AndroidPackageSourceDir));

        FilePath sourceDir = packageSourceDir.pathAppended("src/main/java");
        if (!sourceDir.exists()) {
            sourceDir = packageSourceDir.pathAppended("src");
            if (!sourceDir.exists()) {
                return;
            }
        }

        sourceDir = sourceDir.relativeChildPath(projectDir);

        const QStringList classPaths = node->data(Constants::AndroidClassPaths).toStringList();

        const FilePath &sdkLocation = AndroidConfig::sdkLocation();
        const QString &targetSDK = buildTargetSDK(buildConfiguration());
        const FilePath androidJar = sdkLocation / QString("platforms/%2/android.jar")
                                       .arg(targetSDK);
        FilePaths libs = {androidJar};
        libs << packageSourceDir.pathAppended("libs").dirEntries({{"*.jar"}, QDir::Files});

        for (const QString &path : classPaths)
            libs << FilePath::fromString(path);

        generateProjectFile(projectDir, qtSrc.path(), project()->displayName());
        generateClassPathFile(projectDir, sourceDir, libs);
    }
}

Client *JLSSettings::createClient(BaseClientInterface *interface) const
{
    return new JLSClient(interface);
}

void setupJavaLanguageServer()
{
    LanguageClientSettings::registerClientType(
        {Android::Constants::JLS_SETTINGS_ID, Tr::tr("Java Language Server"),
         [] { return new JLSSettings; }});
}

} // namespace Android::Internal
