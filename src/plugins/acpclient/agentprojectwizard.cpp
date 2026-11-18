// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "agentprojectwizard.h"

#include "acpchattab.h"
#include "acpchatwidget.h"
#include "acpclientconstants.h"
#include "acpclienttr.h"
#include "acpsettings.h"

#include <coreplugin/icore.h>
#include <coreplugin/iwizardfactory.h>

#include <projectexplorer/baseprojectwizarddialog.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectmanager.h>

#include <utils/algorithm.h>
#include <utils/async.h>
#include <utils/infolabel.h>
#include <utils/layoutbuilder.h>
#include <utils/mimeutils.h>
#include <utils/projectintropage.h>
#include <utils/qtcassert.h>
#include <utils/qtcsettings.h>
#include <utils/stylehelper.h>
#include <utils/wizard.h>

#include <QComboBox>
#include <QGuiApplication>
#include <QLabel>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>

#include <memory>

using namespace Core;
using namespace ProjectExplorer;
using namespace Utils;

namespace AcpClient::Internal {

const char LAST_AGENT_KEY[] = "AcpClient/ProjectWizardAgent";
const char WORKSPACE_SCHEMA_URL[]
    = "https://download.qt.io/official_releases/qtcreator/latest/installer_source/"
      "jsonschemas/project.json";

QString agentProjectInstructions(const QString &projectName, const FilePath &projectDirectory)
{
    return Tr::tr(
               "Create a new project named \"%1\" in the directory \"%2\". Do not create or "
               "change files outside of that directory.\n"
               "\n"
               "If you need more information about the project, ask the user. Use "
               "elicitation for that if the client supports it, otherwise ask in the chat. "
               "Wait for the answer instead of guessing important decisions.\n"
               "\n"
               "If one of the following project formats fits the project, use it: Qt "
               "Creator opens them directly.\n"
               "%4\n"
               "\n"
               "As soon as the project files exist, open the project in Qt Creator with the "
               "project_open tool of the Qt Creator MCP server. Pass it the project file of "
               "the chosen format, for example the top-level CMakeLists.txt of a CMake "
               "project. If none of the formats fits, pass it the project directory itself. "
               "Qt Creator then opens the directory as a workspace project, which can be "
               "configured in .qtcreator/project.json in the project directory, as described "
               "by the schema at %3.")
        .arg(projectName,
             projectDirectory.toUserOutput(),
             QString(WORKSPACE_SCHEMA_URL),
             projectFormatsDescription());
}

QString projectFormatsDescription()
{
    QStringList formats;
    for (const QString &name : ProjectExplorerPlugin::projectMimeTypes()) {
        // Described in the instructions as the fallback for everything else.
        if (name == "inode/directory" || name == "text/x-workspace-project")
            continue;
        const MimeType mimeType = Utils::mimeTypeForName(name);
        if (!mimeType.isValid() || mimeType.globPatterns().isEmpty())
            continue;
        formats.append(QString("- %1: %2").arg(mimeType.comment(),
                                               mimeType.globPatterns().join(", ")));
    }
    formats.sort();
    formats.removeDuplicates();
    return formats.join('\n');
}

QString agentProjectPrompt(const QString &instructions, const QString &description)
{
    return instructions.trimmed() + "\n\n" + Tr::tr("Project description:") + '\n'
           + description.trimmed();
}

/*!
    Returns the project file in \a directory that Qt Creator opens, or
    \a directory itself if it holds the configuration of a workspace project.
    Returns an empty path if there is neither.
*/
FilePath projectToOpen(const FilePath &directory)
{
    if (directory.pathAppended(".qtcreator/project.json").isFile())
        return directory;
    for (const QString &pattern : ProjectExplorerPlugin::projectFilePatterns()) {
        const FilePaths files = directory.dirEntries(FileFilter({pattern}, DirFilterFlag::Files),
                                                     DirSortFlag::Name);
        if (!files.isEmpty())
            return files.first();
    }
    return {};
}

// For agents that do not open the project themselves.
static void openProjectInDirectory(const FilePath &projectDirectory, const FilePath &project)
{
    const bool isOpen = Utils::anyOf(ProjectManager::projects(), [&](Project *openProject) {
        const FilePath dir = openProject->projectDirectory();
        return dir == projectDirectory || dir.isChildOf(projectDirectory);
    });
    if (isOpen)
        return;
    const OpenProjectResult result = ProjectExplorerPlugin::openProject(project);
    if (!result)
        ProjectExplorerPlugin::showOpenProjectError(result);
}

class AgentPromptPage final : public WizardPage
{
public:
    explicit AgentPromptPage(ProjectIntroPage *introPage)
        : m_introPage(introPage)
    {
        setTitle(Tr::tr("Project Description"));

        m_agentCombo = new QComboBox;

        m_noAgentLabel = new InfoLabel(
            Tr::tr("No agent is configured. <a href=\"settings\">Configure agents</a>."),
            InfoLabelType::Warning);
        m_noAgentLabel->setElideMode(Qt::ElideNone);
        m_noAgentLabel->setWordWrap(true);
        connect(m_noAgentLabel, &QLabel::linkActivated, this, [] {
            ICore::showSettings("AI.ACPSERVERS");
        });

        m_description = new QPlainTextEdit;
        m_description->setPlaceholderText(
            Tr::tr("Describe the project, for example: A Qt Widgets application that shows "
                   "the current weather for a city."));

        m_instructions = new QPlainTextEdit;

        auto resetButton = new QPushButton(Tr::tr("Reset"));
        resetButton->setToolTip(Tr::tr("Restore the default instructions."));
        connect(resetButton, &QPushButton::clicked, this, [this] {
            m_instructions->setPlainText(defaultInstructions());
        });

        using namespace Layouting;
        // clang-format off
        Column {
            spacing(StyleHelper::SpacingTokens::GapVM),
            Form {
                Tr::tr("Agent:"), m_agentCombo, br,
            },
            m_noAgentLabel,
            TabWidget {
                Tab { Tr::tr("Description"), Column { m_description } },
                Tab { Tr::tr("Instructions to the Agent"),
                    Column { m_instructions, Row { st, resetButton } }
                },
            },
        }.attachTo(this);
        // clang-format on

        connect(m_agentCombo, &QComboBox::currentIndexChanged, this, &QWizardPage::completeChanged);
        connect(m_description, &QPlainTextEdit::textChanged, this, &QWizardPage::completeChanged);
        connect(m_instructions, &QPlainTextEdit::textChanged, this, &QWizardPage::completeChanged);
        connect(&AcpSettings::instance(), &AcpSettings::serversChanged,
                this, &AgentPromptPage::updateAgents);
        updateAgents();
    }

    void initializePage() final
    {
        // Keep edited instructions, but follow name and location while there are none.
        const QString instructions = defaultInstructions();
        if (m_instructions->toPlainText() == m_shownDefaultInstructions)
            m_instructions->setPlainText(instructions);
        m_shownDefaultInstructions = instructions;
        m_description->setFocus();
    }

    bool isComplete() const final
    {
        return m_agentCombo->currentIndex() >= 0
               && !m_description->toPlainText().trimmed().isEmpty()
               && !m_instructions->toPlainText().trimmed().isEmpty();
    }

    bool validatePage() final
    {
        const Result<> created = projectDirectory().ensureWritableDir();
        if (!created) {
            QMessageBox::critical(this, Tr::tr("Cannot Create Project Directory"),
                                  created.error());
            return false;
        }
        ICore::settings()->setValue(LAST_AGENT_KEY, serverId());
        return true;
    }

    QString serverId() const { return m_agentCombo->currentData().toString(); }

    FilePath projectDirectory() const
    {
        return m_introPage->filePath().pathAppended(m_introPage->projectName());
    }

    QString prompt() const
    {
        return agentProjectPrompt(m_instructions->toPlainText(), m_description->toPlainText());
    }

private:
    QString defaultInstructions() const
    {
        return agentProjectInstructions(m_introPage->projectName(), projectDirectory());
    }

    void updateAgents()
    {
        const QString current = m_agentCombo->count() > 0
                                    ? serverId()
                                    : ICore::settings()->value(LAST_AGENT_KEY).toString();
        m_agentCombo->clear();
        for (const AcpSettings::ServerInfo &info : AcpSettings::servers()) {
            m_agentCombo->addItem(info.name, info.id);
            const int index = m_agentCombo->count() - 1;
            Utils::onResultReady(AcpSettings::iconForUrl(info.iconUrl), m_agentCombo,
                                 [combo = m_agentCombo, id = info.id](const QIcon &icon) {
                                     const int i = combo->findData(id);
                                     if (i >= 0)
                                         combo->setItemIcon(i, icon);
                                 });
            if (info.id == current)
                m_agentCombo->setCurrentIndex(index);
        }
        m_noAgentLabel->setVisible(m_agentCombo->count() == 0);
        emit completeChanged();
    }

    ProjectIntroPage *m_introPage;
    QComboBox *m_agentCombo;
    InfoLabel *m_noAgentLabel;
    QPlainTextEdit *m_description;
    QPlainTextEdit *m_instructions;
    QString m_shownDefaultInstructions;
};

class AgentProjectWizardFactory final : public IWizardFactory
{
public:
    explicit AgentProjectWizardFactory(const std::function<AcpChatWidget *()> &showChat)
        : m_showChat(showChat)
    {
        setId("Z.AcpAgentProject");
        setSupportedProjectTypes({"UNKNOWN_PROJECT"});
        setDisplayName(Tr::tr("Project Generated by AI Agent"));
        setDescription(Tr::tr("Describes the project to an AI agent, which creates it and "
                              "opens it in %1.").arg(QGuiApplication::applicationDisplayName()));
        setCategory(Constants::AGENT_PROJECT_WIZARD_CATEGORY);
        setDisplayCategory(Tr::tr("AI"));
        setIcon(themedIcon(":/acpclient/images/wizardicon-agentproject.png"));
        setFlags(PlatformIndependent);
    }

private:
    Wizard *runWizardImpl(const FilePath &path, Id, const QVariantMap &, bool) final
    {
        auto wizard = new Wizard;
        wizard->setWindowTitle(displayName());

        auto introPage = new ProjectIntroPage;
        introPage->setDescription(
            Tr::tr("Choose the name and location of the directory for the project. The agent "
                   "creates the project in it."));
        introPage->setFilePath(path);
        introPage->setProjectName(BaseProjectWizardDialog::uniqueProjectName(path));
        wizard->addPage(introPage);

        auto promptPage = new AgentPromptPage(introPage);
        wizard->addPage(promptPage);

        QObject::connect(wizard, &QDialog::accepted, wizard, [this, promptPage] {
            startAgent(promptPage->serverId(), promptPage->projectDirectory(),
                       promptPage->prompt());
        });
        return wizard;
    }

    void startAgent(const QString &serverId, const FilePath &projectDirectory,
                    const QString &prompt) const
    {
        const AcpSettings::ServerInfo serverInfo
            = Utils::findOrDefault(AcpSettings::servers(),
                                   Utils::equal(&AcpSettings::ServerInfo::id, serverId));
        QTC_ASSERT(!serverInfo.id.isEmpty(), return);
        AcpChatWidget *chat = m_showChat();
        QTC_ASSERT(chat, return);
        chat->startChat(serverInfo, projectDirectory, prompt, [projectDirectory](AcpChatTab *tab) {
            auto connection = std::make_shared<QMetaObject::Connection>();
            *connection = QObject::connect(
                tab, &AcpChatTab::autoStartedTurnFinished, tab, [connection, tab, projectDirectory] {
                    if (tab->workingDirectory() != projectDirectory)
                        return;
                    const FilePath project = projectToOpen(projectDirectory);
                    if (project.isEmpty())
                        return;
                    QObject::disconnect(*connection);
                    openProjectInDirectory(projectDirectory, project);
                });
        });
    }

    const std::function<AcpChatWidget *()> m_showChat;
};

void setupAgentProjectWizard(const std::function<AcpChatWidget *()> &showChat)
{
    IWizardFactory::registerFactoryCreator(
        [showChat]() -> IWizardFactory * { return new AgentProjectWizardFactory(showChat); });
}

} // namespace AcpClient::Internal
