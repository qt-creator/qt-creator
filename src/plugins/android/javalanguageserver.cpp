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

#include "androidconstants.h"

#include <languageclient/client.h>
#include <languageclient/languageclientutils.h>
#include <utils/environment.h>
#include <utils/pathchooser.h>
#include <utils/variablechooser.h>

#include <QGridLayout>
#include <QLineEdit>

using namespace Utils;

constexpr char languageServerKey[] = "languageServer";
constexpr char workspaceKey[] = "workspace";

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
    , m_workspace(new PathChooser(this))
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

    mainLayout->addWidget(new QLabel(tr("Workspace:")), ++row, 0);
    m_workspace->setExpectedKind(Utils::PathChooser::Directory);
    m_workspace->setPath(QDir::toNativeSeparators(settings->m_workspace));
    mainLayout->addWidget(m_workspace, row, 1);

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

    changed |= m_workspace != jlswidget->workspace();
    m_workspace = jlswidget->workspace();

    changed |= m_executable != jlswidget->java();
    m_executable = jlswidget->java();

    QString arguments = "-Declipse.application=org.eclipse.jdt.ls.core.id1 "
                        "-Dosgi.bundles.defaultStartLevel=4 "
                        "-Declipse.product=org.eclipse.jdt.ls.core.product "
                        "-Dlog.level=WARNING "
                        "-noverify "
                        "-Xmx1G "
                        "-jar \"%1\" "
                        "-configuration \"%2\" "
                        "-data \"%3\"";

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
        arguments = arguments.arg(m_languageServer, configDir.absolutePath(), m_workspace);
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
    return StdIOSettings::isValid() && !m_languageServer.isEmpty() && !m_workspace.isEmpty();
}

QVariantMap JLSSettings::toMap() const
{
    QVariantMap map = StdIOSettings::toMap();
    map.insert(languageServerKey, m_languageServer);
    map.insert(workspaceKey, m_workspace);
    return map;
}

void JLSSettings::fromMap(const QVariantMap &map)
{
    StdIOSettings::fromMap(map);
    m_languageServer = map[languageServerKey].toString();
    m_workspace = map[workspaceKey].toString();
}

LanguageClient::BaseSettings *JLSSettings::copy() const
{
    return new JLSSettings(*this);
}

class JLSClient : public LanguageClient::Client
{
public:
    using Client::Client;

    void executeCommand(const LanguageServerProtocol::Command &command) override;
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

LanguageClient::Client *JLSSettings::createClient(LanguageClient::BaseClientInterface *interface) const
{
    return new JLSClient(interface);
}

} // namespace Internal
} // namespace Android
