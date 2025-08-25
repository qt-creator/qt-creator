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

#include <QXmlStreamWriter>

using namespace LanguageClient;
using namespace ProjectExplorer;
using namespace Utils;

namespace Android::Internal {

class JLSSettings final : public StdIOSettings
{
public:
    JLSSettings();

    bool applyFromSettingsWidget(QWidget *widget) final;
    QWidget *createSettingsWidget(QWidget *parent) const final;
    bool isValid() const final;
    Client *createClient(BaseClientInterface *interface) const final;
    BaseClientInterface *createInterface(BuildConfiguration *) const final;

    FilePathAspect languageServer{this};
};

class JLSSettingsWidget : public QWidget
{
public:
    JLSSettingsWidget(const JLSSettings *settings, QWidget *parent)
        : QWidget(parent)
    {
        using namespace Layouting;
        Form {
            settings->name, br,
            settings->executable, br,
            settings->languageServer
        }.attachTo(this);
    }
};

JLSSettings::JLSSettings()
{
    m_settingsTypeId = Constants::JLS_SETTINGS_ID;
    name.setValue("Java Language Server");

    executable.setLabelText(Tr::tr("Java:"));

    languageServer.setSettingsKey("languageServer");
    languageServer.setExpectedKind(PathChooser::File);
    languageServer.setLabelText(Tr::tr("Java Language Server:"));
    languageServer.setPlaceHolderText(Tr::tr("Path to equinox launcher jar"));
    languageServer.setPromptDialogFilter("org.eclipse.equinox.launcher_*.jar");

    m_startBehavior = RequiresProject;
    m_languageFilter.mimeTypes = QStringList(Utils::Constants::JAVA_MIMETYPE);
    const FilePath &javaPath = Environment::systemEnvironment().searchInPath("java");
    if (javaPath.exists())
        executable.setValue(javaPath);
}

bool JLSSettings::applyFromSettingsWidget(QWidget *widget)
{
    bool changed = StdIOSettings::applyFromSettingsWidget(widget);

    QString args = "-Declipse.application=org.eclipse.jdt.ls.core.id1 "
                   "-Dosgi.bundles.defaultStartLevel=4 "
                   "-Declipse.product=org.eclipse.jdt.ls.core.product "
                   "-Dlog.level=WARNING "
                   "-noverify "
                   "-Xmx1G "
                   "-jar \"%1\" "
                   "-configuration \"%2\"";

    QDir configDir = languageServer().toFileInfo().absoluteDir();
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
        args = args.arg(languageServer().path(), configDir.absolutePath());
        changed |= arguments() != args;
        arguments.setValue(args);
    }

    return changed;
}

QWidget *JLSSettings::createSettingsWidget(QWidget *parent) const
{
    return new JLSSettingsWidget(this, parent);
}

bool JLSSettings::isValid() const
{
    return StdIOSettings::isValid() && !languageServer().isEmpty();
}

class JLSInterface : public StdIOClientInterface
{
public:
    FilePath workspaceDir() const { return m_workspaceDir.path(); }

private:
    TemporaryDirectory m_workspaceDir = TemporaryDirectory("QtCreator-jls-XXXXXX");
};

BaseClientInterface *JLSSettings::createInterface(BuildConfiguration *) const
{
    auto interface = new JLSInterface();
    CommandLine cmd{executable(), arguments(), CommandLine::Raw};
    cmd.addArgs({"-data", interface->workspaceDir().path()});
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
                                  const FilePaths &sourceDirs,
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
        for (const FilePath &sourceDir : sourceDirs) {
            writer.writeEmptyElement("classpathentry");
            writer.writeAttribute("kind", "src");
            writer.writeAttribute("path", sourceDir.toUserOutput());
        }
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

    if (ProjectNode *projectNode = project()->findNodeForBuildKey(buildConfiguration()->activeBuildKey())) {
        QtSupport::QtVersion *version = QtSupport::QtKitAspect::qtVersion(kit);
        if (!version)
            return;
        const FilePath qtSrc = version->prefix().pathAppended("src/android/java/src");
        const FilePath projectDir = project()->rootProjectDirectory();
        if (!projectDir.exists())
            return;

        const FilePath &sdkLocation = AndroidConfig::sdkLocation();
        const QString &targetSDK = buildTargetSDK(buildConfiguration());
        const FilePath androidJar = sdkLocation / QString("platforms/%2/android.jar")
                                       .arg(targetSDK);
        FilePaths libs = {androidJar};
        FilePaths sources;

        projectNode->managingProject()->forEachProjectNode([&projectDir, &sources, &libs](const ProjectNode *node) {
            if (!node->isProduct())
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

            sources << sourceDir.relativeChildPath(projectDir);

            libs << packageSourceDir.pathAppended("libs").dirEntries({{"*.jar"}, QDir::Files});

            const QStringList classPaths = node->data(Constants::AndroidClassPaths).toStringList();
            for (const QString &path : classPaths)
                libs << FilePath::fromString(path);
        });

        // Sort and remove duplicate entries.
        std::sort(libs.begin(), libs.end());
        auto last = std::unique(libs.begin(), libs.end());
        libs.erase(last, libs.constEnd());

        generateProjectFile(projectDir, qtSrc.path(), project()->displayName());
        generateClassPathFile(projectDir, sources, libs);
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
