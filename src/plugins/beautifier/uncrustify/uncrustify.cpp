// Copyright (C) 2016 Lorenz Haas
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

// Tested with version 0.59 and 0.60

#include "uncrustify.h"

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
#include <texteditor/texteditor.h>

#include <utils/aspects.h>
#include <utils/fileutils.h>
#include <utils/layoutbuilder.h>
#include <utils/pathchooser.h>
#include <utils/qtcprocess.h>

#include <QAction>
#include <QCheckBox>
#include <QDateTime>
#include <QFile>
#include <QFileInfo>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QRegularExpression>
#include <QVersionNumber>
#include <QXmlStreamWriter>

using namespace TextEditor;
using namespace Utils;

namespace Beautifier::Internal {

const char SETTINGS_NAME[] = "uncrustify";

static QString uDisplayName()  { return Tr::tr("Uncrustify"); }

class UncrustifySettings : public AbstractSettings
{
public:
    UncrustifySettings()
        : AbstractSettings(SETTINGS_NAME, ".cfg")
    {
        setVersionRegExp(QRegularExpression("([0-9]{1})\\.([0-9]{2})"));

        command.setDefaultValue("uncrustify");
        command.setLabelText(Tr::tr("Uncrustify command:"));
        command.setPromptDialogTitle(BeautifierTool::msgCommandPromptDialogTitle(uDisplayName()));

        useOtherFiles.setSettingsKey("useOtherFiles");
        useOtherFiles.setDefaultValue(true);
        useOtherFiles.setLabelText(Tr::tr("Use file uncrustify.cfg defined in project files"));

        useHomeFile.setSettingsKey("useHomeFile");
        useHomeFile.setLabelText(Tr::tr("Use file uncrustify.cfg in HOME")
                                     .replace( "HOME", QDir::toNativeSeparators(QDir::home().absolutePath())));

        useCustomStyle.setSettingsKey("useCustomStyle");
        useCustomStyle.setLabelText(Tr::tr("Use customized style:"));

        useSpecificConfigFile.setSettingsKey("useSpecificConfigFile");
        useSpecificConfigFile.setLabelText(Tr::tr("Use file specific uncrustify.cfg"));

        customStyle.setSettingsKey("customStyle");

        formatEntireFileFallback.setSettingsKey("formatEntireFileFallback");
        formatEntireFileFallback.setDefaultValue(true);
        formatEntireFileFallback.setLabelText(Tr::tr("Format entire file if no text was selected"));
        formatEntireFileFallback.setToolTip(Tr::tr("For action Format Selected Text"));

        specificConfigFile.setSettingsKey("specificConfigFile");
        specificConfigFile.setExpectedKind(Utils::PathChooser::File);
        specificConfigFile.setPromptDialogFilter(Tr::tr("Uncrustify file (*.cfg)"));

        documentationFilePath = Core::ICore::userResourcePath(Constants::SETTINGS_DIRNAME)
                                    .pathAppended(Constants::DOCUMENTATION_DIRNAME)
                                    .pathAppended(SETTINGS_NAME).stringAppended(".xml");

        read();
    }

    void createDocumentationFile() const final
    {
        Process process;
        process.setCommand({command(), {"--show-config"}});
        using namespace std::chrono_literals;
        process.runBlocking(2s);
        if (process.result() != ProcessResult::FinishedWithSuccess)
            return;

        QFile file(documentationFilePath.toFSPathString());
        const QFileInfo fi(file);
        if (!fi.exists())
            fi.dir().mkpath(fi.absolutePath());
        if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text))
            return;

        bool contextWritten = false;
        QXmlStreamWriter stream(&file);
        stream.setAutoFormatting(true);
        stream.writeStartDocument("1.0", true);
        stream.writeComment("Created " + QDateTime::currentDateTime().toString(Qt::ISODate));
        stream.writeStartElement(Constants::DOCUMENTATION_XMLROOT);

        const QStringList lines = process.allOutput().split(QLatin1Char('\n'));
        const int totalLines = lines.count();
        for (int i = 0; i < totalLines; ++i) {
            const QString &line = lines.at(i);
            if (line.startsWith('#') || line.trimmed().isEmpty())
                continue;

            const int firstSpace = line.indexOf(' ');
            const QString keyword = line.left(firstSpace);
            const QString options = line.right(line.size() - firstSpace).trimmed();
            QStringList docu;
            while (++i < totalLines) {
                const QString &subline = lines.at(i);
                if (line.startsWith('#') || subline.trimmed().isEmpty()) {
                    const QString text = "<p><span class=\"option\">" + keyword
                                         + "</span> <span class=\"param\">" + options
                                         + "</span></p><p>" + docu.join(' ').toHtmlEscaped() + "</p>";
                    stream.writeStartElement(Constants::DOCUMENTATION_XMLENTRY);
                    stream.writeTextElement(Constants::DOCUMENTATION_XMLKEY, keyword);
                    stream.writeTextElement(Constants::DOCUMENTATION_XMLDOC, text);
                    stream.writeEndElement();
                    contextWritten = true;
                    break;
                } else {
                    docu << subline;
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
    BoolAspect useHomeFile{this};
    BoolAspect useCustomStyle{this};

    StringAspect customStyle{this};
    BoolAspect formatEntireFileFallback{this};

    FilePathAspect specificConfigFile{this};
    BoolAspect useSpecificConfigFile{this};
};

static UncrustifySettings &settings()
{
    static UncrustifySettings theSettings;
    return theSettings;
}

class UncrustifySettingsPageWidget : public Core::IOptionsPageWidget
{
public:
    UncrustifySettingsPageWidget()
    {
        UncrustifySettings &s = settings();

        auto configurations = new ConfigurationPanel(this);
        configurations->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        configurations->setSettings(&settings());
        configurations->setCurrentConfiguration(settings().customStyle());

        QGroupBox *options = nullptr;

        using namespace Layouting;

        Column {
            Group {
                title(Tr::tr("Configuration")),
                Form {
                    s.command, br,
                    s.supportedMimeTypes,
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
                    s.formatEntireFileFallback
                },
            },
            st
        }.attachTo(this);

        s.read();

        connect(s.command.pathChooser(), &PathChooser::validChanged, options, &QWidget::setEnabled);
        options->setEnabled(s.command.pathChooser()->isValid());

        setOnApply([&s, configurations] {
            s.customStyle.setValue(configurations->currentConfiguration());
            settings().apply();
            s.save();
        });

        setOnCancel([] { settings().cancel(); });
    }
};

// Uncrustify

class Uncrustify final : public BeautifierTool
{
public:
    Uncrustify()
    {
        const Id menuId = "Uncrustify.Menu";
        Core::MenuBuilder(menuId)
            .setTitle(Tr::tr("&Uncrustify"))
            .addToContainer(Constants::MENU_ID);

        Core::ActionBuilder(this, "Uncrustify.FormatFile")
            .setText(msgFormatCurrentFile())
            .bindContextAction(&m_formatFile)
            .addToContainer(menuId)
            .addOnTriggered(this, &Uncrustify::formatFile);

        Core::ActionBuilder(this, "Uncrustify.FormatSelectedText")
            .setText(msgFormatSelectedText())
            .bindContextAction(&m_formatRange)
            .addToContainer(menuId)
            .addOnTriggered(this, &Uncrustify::formatSelectedText);

        settings().supportedMimeTypes.addOnChanged(this, [this] {
            updateActions(Core::EditorManager::currentEditor());
        });
    }

    QString id() const final
    {
        return "Uncrustify";
    }

    void updateActions(Core::IEditor *editor) final
    {
        const bool enabled = editor && settings().isApplicable(editor->document());
        m_formatFile->setEnabled(enabled);
        m_formatRange->setEnabled(enabled);
    }

    TextEditor::Command textCommand() const final
    {
        const FilePath cfgFile = configurationFile();
        return cfgFile.isEmpty() ? Command() : textCommand(cfgFile, false);
    }

    bool isApplicable(const Core::IDocument *document) const final
    {
        return settings().isApplicable(document);
    }

private:
    void formatFile();
    void formatSelectedText();
    Utils::FilePath configurationFile() const;
    TextEditor::Command textCommand(const Utils::FilePath &cfgFile, bool fragment = false) const;

    QAction *m_formatFile = nullptr;
    QAction *m_formatRange = nullptr;
};

void Uncrustify::formatFile()
{
    const FilePath cfgFileName = configurationFile();
    if (cfgFileName.isEmpty())
        showError(msgCannotGetConfigurationFile(uDisplayName()));
    else
        formatCurrentFile(textCommand(cfgFileName));
}

void Uncrustify::formatSelectedText()
{
    const FilePath cfgFileName = configurationFile();
    if (cfgFileName.isEmpty()) {
        showError(msgCannotGetConfigurationFile(uDisplayName()));
        return;
    }

    const TextEditorWidget *widget = TextEditorWidget::currentTextEditorWidget();
    if (!widget)
        return;

    QTextCursor tc = widget->textCursor();
    if (tc.hasSelection()) {
        // Extend selection to full lines
        const int posSelectionEnd = tc.selectionEnd();
        tc.setPosition(tc.selectionStart());
        tc.movePosition(QTextCursor::StartOfLine);
        const int startPos = tc.position();
        tc.setPosition(posSelectionEnd);
        // Don't extend the selection if the cursor is at the start of the line
        if (tc.positionInBlock() > 0)
            tc.movePosition(QTextCursor::EndOfLine);
        const int endPos = tc.position();
        formatCurrentFile(textCommand(cfgFileName, true), startPos, endPos);
    } else if (settings().formatEntireFileFallback()) {
        formatFile();
    }
}

FilePath Uncrustify::configurationFile() const
{
    if (settings().useCustomStyle())
        return settings().styleFileName(settings().customStyle());

    if (settings().useOtherFiles()) {
        using namespace ProjectExplorer;
        if (const Project *project = ProjectTree::currentProject()) {
            const FilePaths files = project->files([](const Node *n) {
                const FilePath fp = n->filePath();
                return fp.fileName() == "uncrustify.cfg" && fp.isReadableFile();
            });
            if (!files.isEmpty())
                return files.first();
        }
    }

    if (settings().useSpecificConfigFile()) {
        const FilePath file = settings().specificConfigFile();
        if (file.exists())
            return file;
    }

    if (settings().useHomeFile()) {
        const FilePath file = FileUtils::homePath() / "uncrustify.cfg";
        if (file.exists())
            return file;
    }

    return {};
}

Command Uncrustify::textCommand(const FilePath &cfgFile, bool fragment) const
{
    Command cmd;
    cmd.setExecutable(settings().command());
    cmd.setProcessing(Command::PipeProcessing);
    if (settings().version() >= QVersionNumber(0, 62)) {
        cmd.addOption("--assume");
        cmd.addOption("%file");
    } else {
        cmd.addOption("-l");
        cmd.addOption("cpp");
    }
    cmd.addOption("-L");
    cmd.addOption("1-2");
    if (fragment)
        cmd.addOption("--frag");
    cmd.addOption("-c");
    cmd.addOption(cfgFile.path());
    return cmd;
}


// UncrustifySettingsPage

class UncrustifySettingsPage final : public Core::IOptionsPage
{
public:
    UncrustifySettingsPage()
    {
        setId("Uncrustify");
        setDisplayName(uDisplayName());
        setCategory(Constants::OPTION_CATEGORY);
        setWidgetCreator([] { return new UncrustifySettingsPageWidget; });
    }
};

const UncrustifySettingsPage settingsPage;

void setupUncrustify()
{
    static Uncrustify theUncrustify;
}

} // Beautifier::Internal
