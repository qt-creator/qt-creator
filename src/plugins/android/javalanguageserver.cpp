/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "javalanguageserver.h"

#include "androidconfigurations.h"
#include "androidconstants.h"
#include "androidmanager.h"

#include <languageclient/client.h>
#include <languageclient/languageclientinterface.h>
#include <languageclient/languageclientutils.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectnodes.h>
#include <projectexplorer/target.h>
#include <qtsupport/qtkitinformation.h>
#include <utils/environment.h>
#include <utils/pathchooser.h>
#include <utils/temporarydirectory.h>
#include <utils/variablechooser.h>

#include <QGridLayout>
#include <QLineEdit>

using namespace Utils;

constexpr char languageServerKey[] = "languageServer";

namespace Android {
namespace Internal {

class JLSSettingsWidget : public QWidget
{
    Q_DECLARE_TR_FUNCTIONS(JLSSettingsWidget)
public:
    JLSSettingsWidget(const JLSSettings *settings, QWidget *parent);

    QString name() const { return m_name->text(); }
    QString java() const { return m_java->filePath().toString(); }
    QString languageServer() const { return m_ls->filePath().toString(); }
    QString workspace() const { return m_workspace->filePath().toString(); }

private:
    QLineEdit *m_name = nullptr;
    PathChooser *m_java = nullptr;
    PathChooser *m_ls = nullptr;
    PathChooser *m_workspace = nullptr;
};

JLSSettingsWidget::JLSSettingsWidget(const JLSSettings *settings, QWidget *parent)
    : QWidget(parent)
    , m_name(new QLineEdit(settings->m_name, this))
    , m_java(new PathChooser(this))
    , m_ls(new PathChooser(this))
{
    int row = 0;
    auto *mainLayout = new QGridLayout;
    mainLayout->addWidget(new QLabel(tr("Name:")), row, 0);
    mainLayout->addWidget(m_name, row, 1);
    auto chooser = new VariableChooser(this);
    chooser->addSupportedWidget(m_name);

    mainLayout->addWidget(new QLabel(tr("Java:")), ++row, 0);
    m_java->setExpectedKind(Utils::PathChooser::ExistingCommand);
    m_java->setPath(QDir::toNativeSeparators(settings->m_executable));
    mainLayout->addWidget(m_java, row, 1);

    mainLayout->addWidget(new QLabel(tr("Java Language Server:")), ++row, 0);
    m_ls->setExpectedKind(Utils::PathChooser::File);
    m_ls->lineEdit()->setPlaceholderText(tr("Path to equinox launcher jar"));
    m_ls->setPromptDialogFilter("org.eclipse.equinox.launcher_*.jar");
    m_ls->setPath(QDir::toNativeSeparators(settings->m_languageServer));
    mainLayout->addWidget(m_ls, row, 1);

    setLayout(mainLayout);
}

JLSSettings::JLSSettings()
{
    m_settingsTypeId = Constants::JLS_SETTINGS_ID;
    m_name = "Java Language Server";
    m_startBehavior = RequiresProject;
    m_languageFilter.mimeTypes = QStringList(Constants::JAVA_MIMETYPE);
    const FilePath &javaPath = Utils::Environment::systemEnvironment().searchInPath("java");
    if (javaPath.exists())
        m_executable = javaPath.toString();
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

    QFileInfo languageServerFileInfo(m_languageServer);
    QDir configDir = languageServerFileInfo.absoluteDir();
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
        arguments = arguments.arg(m_languageServer, configDir.absolutePath());
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

QVariantMap JLSSettings::toMap() const
{
    QVariantMap map = StdIOSettings::toMap();
    map.insert(languageServerKey, m_languageServer);
    return map;
}

void JLSSettings::fromMap(const QVariantMap &map)
{
    StdIOSettings::fromMap(map);
    m_languageServer = map[languageServerKey].toString();
}

LanguageClient::BaseSettings *JLSSettings::copy() const
{
    return new JLSSettings(*this);
}

class JLSInterface : public LanguageClient::StdIOClientInterface
{
public:
    JLSInterface() = default;

    QString workspaceDir() const { return m_workspaceDir.path(); }

private:
    TemporaryDirectory m_workspaceDir = TemporaryDirectory("QtCreator-jls-XXXXXX");
};

LanguageClient::BaseClientInterface *JLSSettings::createInterface() const
{
    auto interface = new JLSInterface();
    interface->setExecutable(m_executable);
    QString arguments = this->arguments();
    arguments += QString(" -data \"%1\"").arg(interface->workspaceDir());
    interface->setArguments(arguments);
    return interface;
}

class JLSClient : public LanguageClient::Client
{
public:
    using Client::Client;

    void executeCommand(const LanguageServerProtocol::Command &command) override;
    void setCurrentProject(ProjectExplorer::Project *project) override;
    void updateProjectFiles();
    void updateTarget(ProjectExplorer::Target *target);

private:
    ProjectExplorer::Target *m_currentTarget = nullptr;
};

void JLSClient::executeCommand(const LanguageServerProtocol::Command &command)
{
    if (command.command() == "java.apply.workspaceEdit") {
        const QJsonArray arguments = command.arguments().value_or(QJsonArray());
        for (const QJsonValue &argument : arguments) {
            if (!argument.isObject())
                continue;
            LanguageServerProtocol::WorkspaceEdit edit(argument.toObject());
            if (edit.isValid(nullptr))
                LanguageClient::applyWorkspaceEdit(edit);
        }
    } else {
        Client::executeCommand(command);
    }
}

void JLSClient::setCurrentProject(ProjectExplorer::Project *project)
{
    Client::setCurrentProject(project);
    QTC_ASSERT(project, return);
    updateTarget(project->activeTarget());
    updateProjectFiles();
    connect(project, &ProjectExplorer::Project::activeTargetChanged,
            this, &JLSClient::updateTarget);
}

static void generateProjectFile(const FilePath &projectDir,
                                const QString &qtSrc,
                                const QString &projectName)
{
    const FilePath projectFilePath = projectDir.pathAppended(".project");
    QFile projectFile(projectFilePath.toString());
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
                                  const QString &sourceDir,
                                  const QStringList &libs)
{
    const FilePath classPathFilePath = projectDir.pathAppended(".classpath");
    QFile classPathFile(classPathFilePath.toString());
    if (classPathFile.open(QFile::Truncate | QFile::WriteOnly)) {
        QXmlStreamWriter writer(&classPathFile);
        writer.setAutoFormatting(true);
        writer.writeStartDocument();
        writer.writeComment("Autogenerated by Qt Creator. "
                            "Changes to this file will not be taken into account.");
        writer.writeStartElement("classpath");
        writer.writeEmptyElement("classpathentry");
        writer.writeAttribute("kind", "src");
        writer.writeAttribute("path", sourceDir);
        writer.writeEmptyElement("classpathentry");
        writer.writeAttribute("kind", "src");
        writer.writeAttribute("path", "qtSrc");
        for (const QString &lib : libs) {
            writer.writeEmptyElement("classpathentry");
            writer.writeAttribute("kind", "lib");
            writer.writeAttribute("path", lib);
        }
        writer.writeEndElement(); // classpath
        writer.writeEndDocument();
        classPathFile.close();
    }
}

void JLSClient::updateProjectFiles()
{
    using namespace ProjectExplorer;
    if (!m_currentTarget)
        return;
    if (Target *target = m_currentTarget) {
        Kit *kit = m_currentTarget->kit();
        if (DeviceTypeKitAspect::deviceTypeId(kit) != Android::Constants::ANDROID_DEVICE_TYPE)
            return;
        if (ProjectNode *node = project()->findNodeForBuildKey(target->activeBuildKey())) {
            QtSupport::BaseQtVersion *version = QtSupport::QtKitAspect::qtVersion(kit);
            if (!version)
                return;
            const QString qtSrc = version->prefix().toString() + "/src/android/java/src";
            const FilePath &projectDir = project()->rootProjectDirectory();
            if (!projectDir.exists())
                return;
            const FilePath packageSourceDir = FilePath::fromVariant(
                node->data(Constants::AndroidPackageSourceDir));
            FilePath sourceDir = packageSourceDir.pathAppended("src");
            if (!sourceDir.exists())
                return;
            sourceDir = sourceDir.relativeChildPath(projectDir);
            const FilePath &sdkLocation = AndroidConfigurations::currentConfig().sdkLocation();
            const QString &targetSDK = AndroidManager::buildTargetSDK(m_currentTarget);
            const QString androidJar = QString("%1/platforms/%2/android.jar")
                                           .arg(sdkLocation.toString(), targetSDK);
            QStringList libs(androidJar);
            QDir libDir(packageSourceDir.pathAppended("libs").toString());
            libs << Utils::transform(libDir.entryInfoList({"*.jar"}, QDir::Files),
                                     &QFileInfo::absoluteFilePath);
            generateProjectFile(projectDir, qtSrc, project()->displayName());
            generateClassPathFile(projectDir, sourceDir.toString(), libs);
        }
    }
}

void JLSClient::updateTarget(ProjectExplorer::Target *target)
{
    if (m_currentTarget) {
        disconnect(m_currentTarget, &ProjectExplorer::Target::parsingFinished,
                   this, &JLSClient::updateProjectFiles);
    }
    m_currentTarget = target;
    if (m_currentTarget) {
        connect(m_currentTarget, &ProjectExplorer::Target::parsingFinished,
                this, &JLSClient::updateProjectFiles);
    }
    updateProjectFiles();
}

LanguageClient::Client *JLSSettings::createClient(LanguageClient::BaseClientInterface *interface) const
{
    return new JLSClient(interface);
}

} // namespace Internal
} // namespace Android
