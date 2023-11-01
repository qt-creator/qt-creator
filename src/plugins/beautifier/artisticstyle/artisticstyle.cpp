// Copyright (C) 2016 Lorenz Haas
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

// Tested with version 2.01, 2.02, 2.02.1, 2.03 and 2.04

#include "artisticstyle.h"

#include "../beautifierconstants.h"
#include "../beautifiertool.h"
#include "../beautifiertr.h"
#include "../configurationpanel.h"

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/icore.h>
#include <coreplugin/idocument.h>

#include <projectexplorer/project.h>
#include <projectexplorer/projectnodes.h>
#include <projectexplorer/projecttree.h>

#include <texteditor/formattexteditor.h>

#include <utils/fileutils.h>
#include <utils/hostosinfo.h>
#include <utils/layoutbuilder.h>
#include <utils/pathchooser.h>
#include <utils/process.h>
#include <utils/stringutils.h>

#include <QAction>
#include <QApplication>
#include <QDateTime>
#include <QFile>
#include <QGroupBox>
#include <QMenu>
#include <QRegularExpression>
#include <QVersionNumber>
#include <QXmlStreamWriter>

using namespace TextEditor;
using namespace Utils;

namespace Beautifier::Internal {

// Settings

static QString asDisplayName() { return Tr::tr("Artistic Style"); }

const char SETTINGS_NAME[]            = "artisticstyle";

class ArtisticStyleSettings : public AbstractSettings
{
public:
    ArtisticStyleSettings()
        : AbstractSettings(SETTINGS_NAME, ".astyle")
    {
        setVersionRegExp(QRegularExpression("([2-9]{1})\\.([0-9]{1,2})(\\.[1-9]{1})?$"));
        command.setLabelText(Tr::tr("Artistic Style command:"));
        command.setDefaultValue("astyle");
        command.setPromptDialogTitle(BeautifierTool::msgCommandPromptDialogTitle(asDisplayName()));

        useOtherFiles.setSettingsKey("useOtherFiles");
        useOtherFiles.setLabelText(Tr::tr("Use file *.astylerc defined in project files"));
        useOtherFiles.setDefaultValue(true);

        useSpecificConfigFile.setSettingsKey("useSpecificConfigFile");
        useSpecificConfigFile.setLabelText(Tr::tr("Use specific config file:"));

        specificConfigFile.setSettingsKey("specificConfigFile");
        specificConfigFile.setExpectedKind(PathChooser::File);
        specificConfigFile.setPromptDialogFilter(Tr::tr("AStyle (*.astylerc)"));

        useHomeFile.setSettingsKey("useHomeFile");
        useHomeFile.setLabelText(Tr::tr("Use file .astylerc or astylerc in HOME").
                                 replace("HOME", QDir::toNativeSeparators(QDir::home().absolutePath())));

        useCustomStyle.setSettingsKey("useCustomStyle");
        useCustomStyle.setLabelText(Tr::tr("Use customized style:"));

        customStyle.setSettingsKey("customStyle");

        documentationFilePath =
            Core::ICore::userResourcePath(Beautifier::Constants::SETTINGS_DIRNAME)
                .pathAppended(Beautifier::Constants::DOCUMENTATION_DIRNAME)
                .pathAppended(SETTINGS_NAME)
                .stringAppended(".xml");

        read();
    }

    void createDocumentationFile() const override
    {
        Process process;
        process.setTimeoutS(2);
        process.setCommand({command(), {"-h"}});
        process.runBlocking();
        if (process.result() != ProcessResult::FinishedWithSuccess)
            return;

        if (!documentationFilePath.exists())
            documentationFilePath.parentDir().ensureWritableDir();

        QFile file(documentationFilePath.toFSPathString());
        if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text))
            return;

        bool contextWritten = false;
        QXmlStreamWriter stream(&file);
        stream.setAutoFormatting(true);
        stream.writeStartDocument("1.0", true);
        stream.writeComment("Created " + QDateTime::currentDateTime().toString(Qt::ISODate));
        stream.writeStartElement(Constants::DOCUMENTATION_XMLROOT);

        // astyle writes its output to 'error'...
        const QStringList lines = process.cleanedStdErr().split(QLatin1Char('\n'));
        QStringList keys;
        QStringList docu;
        for (QString line : lines) {
            line = line.trimmed();
            if ((line.startsWith("--") && !line.startsWith("---")) || line.startsWith("OR ")) {
                const QStringList rawKeys = line.split(" OR ", Qt::SkipEmptyParts);
                for (QString k : rawKeys) {
                    k = k.trimmed();
                    k.remove('#');
                    keys << k;
                    if (k.startsWith("--"))
                        keys << k.right(k.size() - 2);
                }
            } else {
                if (line.isEmpty()) {
                    if (!keys.isEmpty()) {
                        // Write entry
                        stream.writeStartElement(Constants::DOCUMENTATION_XMLENTRY);
                        stream.writeStartElement(Constants::DOCUMENTATION_XMLKEYS);
                        for (const QString &key : std::as_const(keys))
                            stream.writeTextElement(Constants::DOCUMENTATION_XMLKEY, key);
                        stream.writeEndElement();
                        const QString text = "<p><span class=\"option\">"
                                             + keys.filter(QRegularExpression("^\\-")).join(", ") + "</span></p><p>"
                                             + (docu.join(' ').toHtmlEscaped()) + "</p>";
                        stream.writeTextElement(Constants::DOCUMENTATION_XMLDOC, text);
                        stream.writeEndElement();
                        contextWritten = true;
                    }
                    keys.clear();
                    docu.clear();
                } else if (!keys.isEmpty()) {
                    docu << line;
                }
            }
        }

        stream.writeEndElement();
        stream.writeEndDocument();

        // An empty file causes error messages and a contextless file preventing this function to run
        // again in order to generate the documentation successfully. Thus delete the file.
        if (!contextWritten) {
            file.close();
            file.remove();
        }
    }

    BoolAspect useOtherFiles{this};
    BoolAspect useSpecificConfigFile{this};
    FilePathAspect specificConfigFile{this};
    BoolAspect useHomeFile{this};
    BoolAspect useCustomStyle{this};
    StringAspect customStyle{this};
};

static ArtisticStyleSettings &settings()
{
    static ArtisticStyleSettings theSettings;
    return theSettings;
}

// ArtisticStyleOptionsPage

class ArtisticStyleSettingsPageWidget : public Core::IOptionsPageWidget
{
public:
    ArtisticStyleSettingsPageWidget()
    {
        QGroupBox *options = nullptr;

        auto configurations = new ConfigurationPanel(this);
        configurations->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        configurations->setSettings(&settings());
        configurations->setCurrentConfiguration(settings().customStyle());

        using namespace Layouting;

        ArtisticStyleSettings &s = settings();

        Column {
            Group {
                title(Tr::tr("Configuration")),
                Form {
                    s.command, br,
                    s.supportedMimeTypes
                }
            },
            Group {
                title(Tr::tr("Options")),
                bindTo(&options),
                Column {
                    s.useOtherFiles,
                    Row { s.useSpecificConfigFile, s.specificConfigFile },
                    s.useHomeFile,
                    Row { s.useCustomStyle, configurations },
                }
            },
            st
        }.attachTo(this);

        setOnApply([&s, configurations] {
            s.customStyle.setValue(configurations->currentConfiguration());
            settings().apply();
            s.save();
        });

        s.read();

        connect(s.command.pathChooser(), &PathChooser::validChanged, options, &QWidget::setEnabled);
        options->setEnabled(s.command.pathChooser()->isValid());
    }
};

// Style

ArtisticStyle::ArtisticStyle()
{
    Core::ActionContainer *menu = Core::ActionManager::createMenu("ArtisticStyle.Menu");
    menu->menu()->setTitle(Tr::tr("&Artistic Style"));

    m_formatFile = new QAction(msgFormatCurrentFile(), this);
    menu->addAction(Core::ActionManager::registerAction(m_formatFile, "ArtisticStyle.FormatFile"));
    connect(m_formatFile, &QAction::triggered, this, &ArtisticStyle::formatFile);

    Core::ActionManager::actionContainer(Constants::MENU_ID)->addMenu(menu);

    connect(&settings().supportedMimeTypes, &Utils::BaseAspect::changed,
            this, [this] { updateActions(Core::EditorManager::currentEditor()); });
}

QString ArtisticStyle::id() const
{
    return "Artistic Style";
}

void ArtisticStyle::updateActions(Core::IEditor *editor)
{
    m_formatFile->setEnabled(editor && settings().isApplicable(editor->document()));
}

void ArtisticStyle::formatFile()
{
    const FilePath cfgFileName = configurationFile();
    if (cfgFileName.isEmpty())
        showError(BeautifierTool::msgCannotGetConfigurationFile(asDisplayName()));
    else
        formatCurrentFile(textCommand(cfgFileName.toFSPathString()));
}

FilePath ArtisticStyle::configurationFile() const
{
    if (settings().useCustomStyle())
        return settings().styleFileName(settings().customStyle());

    if (settings().useOtherFiles()) {
        if (const ProjectExplorer::Project *project
                = ProjectExplorer::ProjectTree::currentProject()) {
            const FilePaths astyleRcfiles = project->files(
                [](const ProjectExplorer::Node *n) { return n->filePath().endsWith(".astylerc"); });
            for (const FilePath &file : astyleRcfiles) {
                if (file.isReadableFile())
                    return file;
            }
        }
    }

    if (settings().useSpecificConfigFile()) {
        const FilePath file = settings().specificConfigFile();
        if (file.exists())
            return file;
    }

    if (settings().useHomeFile()) {
        const FilePath homeDirectory = FileUtils::homePath();
        FilePath file = homeDirectory / ".astylerc";
        if (file.exists())
            return file;
        file = homeDirectory / "astylerc";
        if (file.exists())
            return file;
    }

    return {};
}

Command ArtisticStyle::textCommand() const
{
    const FilePath cfgFile = configurationFile();
    return cfgFile.isEmpty() ? Command() : textCommand(cfgFile.toFSPathString());
}

bool ArtisticStyle::isApplicable(const Core::IDocument *document) const
{
    return settings().isApplicable(document);
}

Command ArtisticStyle::textCommand(const QString &cfgFile) const
{
    Command cmd;
    cmd.setExecutable(settings().command());
    cmd.addOption("-q");
    cmd.addOption("--options=" + cfgFile);

    const QVersionNumber version = settings().version();
    if (version > QVersionNumber(2, 3)) {
        cmd.setProcessing(Command::PipeProcessing);
        if (version == QVersionNumber(2, 4))
            cmd.setPipeAddsNewline(true);
        cmd.setReturnsCRLF(Utils::HostOsInfo::isWindowsHost());
        cmd.addOption("-z2");
    } else {
        cmd.addOption("%file");
    }

    return cmd;
}


//  ArtisticStyleSettingsPage

class ArtisticStyleSettingsPage final : public Core::IOptionsPage
{
public:
    ArtisticStyleSettingsPage()
    {
        setId("ArtisticStyle");
        setDisplayName(asDisplayName());
        setCategory(Constants::OPTION_CATEGORY);
        setWidgetCreator([] { return new ArtisticStyleSettingsPageWidget; });
    }
};

const ArtisticStyleSettingsPage settingsPage;

} // Beautifier::Internal
