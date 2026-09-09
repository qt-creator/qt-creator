// Copyright (C) 2016 Lorenz Haas
// Copyright (C) 2022 Xavier BESSON
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cmakeformatter.h"

#include "cmakeprojectconstants.h"
#include "cmakeprojectmanagertr.h"

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/dialogs/ioptionspage.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/idocument.h>

#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorericons.h>
#include <projectexplorer/projectnodes.h>
#include <projectexplorer/projecttree.h>

#include <texteditor/command.h>
#include <texteditor/formattexteditor.h>
#include <texteditor/textdocument.h>
#include <texteditor/texteditor.h>

#include <utils/algorithm.h>
#include <utils/layoutbuilder.h>
#include <utils/mimeconstants.h>
#include <utils/mimeutils.h>
#include <utils/pathchooser.h>
#include <utils/qtcsettings.h>

#include <QMenu>

using namespace Core;
using namespace TextEditor;
using namespace Utils;

namespace CMakeProjectManager::Internal {

// Where cmake-format stands unless the user points somewhere else.
static const char defaultFormatCommand[] = "cmake-format";

class CMakeFormatterSettings : public AspectContainer
{
public:
    enum Formatter {
        BuiltIn,
        CMakeFormat
    };

    CMakeFormatterSettings()
    {
        setAutoApply(false);
        setSettingsGroups(Constants::CMAKEFORMATTER_SETTINGS_GROUP,
                          Constants::CMAKEFORMATTER_GENERAL_GROUP);

        formatter.setSettingsKey("formatter");
        formatter.setDefaultValue(formatterOfStoredSettings());
        formatter.setDisplayStyle(SelectionAspect::DisplayStyle::RadioButtons);
        formatter.setLabelText(Tr::tr("Formatter:"));
        formatter.addOption(Tr::tr("Built-in"),
                            Tr::tr("Lays the file out the way the CMake files of Qt are laid "
                                   "out. Needs nothing installed."));
        formatter.addOption(Tr::tr("CMakeFormat"),
                            Tr::tr("Hands the file to the cmake-format command below."));

        command.setSettingsKey("autoFormatCommand");
        command.setDefaultValue(defaultFormatCommand);
        command.setExpectedKind(PathChooserKind::ExistingCommand);

        autoFormatOnSave.setSettingsKey("autoFormatOnSave");
        autoFormatOnSave.setLabelText(Tr::tr("Enable auto format on file save"));

        autoFormatOnlyCurrentProject.setSettingsKey("autoFormatOnlyCurrentProject");
        autoFormatOnlyCurrentProject.setDefaultValue(true);
        autoFormatOnlyCurrentProject.setLabelText(Tr::tr("Restrict to files contained in the current project"));
        autoFormatOnlyCurrentProject.setLabelPlacement(BoolAspect::LabelPlacement::AtCheckBox);

        autoFormatMime.setSettingsKey("autoFormatMime");
        autoFormatMime.setDefaultValue(Utils::Constants::CMAKE_MIMETYPE);
        autoFormatMime.setLabelText(Tr::tr("Restrict to MIME types:"));
        autoFormatMime.setDisplayStyle(StringAspect::LineEditDisplay);

        setLayouter([this] {
            using namespace Layouting;

            auto cmakeFormatter = new QLabel(
                Tr::tr("<a href=\"%1\">CMakeFormat</a> command:")
                    .arg("qthelp://org.qt-project.qtcreator/doc/"
                         "creator-project-cmake.html#formatting-cmake-files"));
            cmakeFormatter->setOpenExternalLinks(true);

            return Column {
                formatter,
                Space(10),
                Row { cmakeFormatter, command },
                Space(10),
                Group {
                    title(Tr::tr("Automatic Formatting on File Save")),
                    groupChecker(autoFormatOnSave.groupChecker()),
                    // Conceptually, that's a Form, but this would look odd:
                    // xxxxxx [____]
                    //        [x] xxxxxxxxxxxxxx
                    Column {
                        Row { autoFormatMime },
                        autoFormatOnlyCurrentProject
                    }
                },
                st
            };
        });

        MenuBuilder(Constants::CMAKEFORMATTER_MENU_ID)
            .setTitle(Tr::tr("CMakeFormatter"))
            .setIcon(ProjectExplorer::Icons::CMAKE_LOGO.icon())
            .setOnAllDisabledBehavior(ActionContainer::Show)
            .addToContainer(Core::Constants::M_TOOLS);

        Core::Command *cmd = ActionManager::registerAction(&formatFile, Constants::CMAKEFORMATTER_ACTION_ID);
        connect(&formatFile, &QAction::triggered, this, [this] {
            IEditor *editor = EditorManager::currentEditor();
            if (formatter() == BuiltIn) {
                if (editor)
                    format(editor->document());
                return;
            }

            auto command = formatCommand();
            if (editor)
                extendCommandWithConfigs(command, editor->document()->filePath());

            TextEditor::formatCurrentFile(command);
        });

        ActionManager::actionContainer(Constants::CMAKEFORMATTER_MENU_ID)->addAction(cmd);

        auto updateActions = [this] {
            auto editor = EditorManager::currentEditor();

            formatFile.setEnabled(haveFormatter() && editor && isApplicable(editor->document()));
        };

        autoFormatMime.addOnChanged(this, updateActions);
        formatter.addOnChanged(this, updateActions);
        connect(EditorManager::instance(), &EditorManager::currentEditorChanged,
                this, updateActions);
        connect(EditorManager::instance(), &EditorManager::aboutToSave,
                this, &CMakeFormatterSettings::applyIfNecessary);

        readSettings();

        const FilePath commandPath = command().searchInPath();
        haveValidFormatCommand = commandPath.exists() && commandPath.isExecutableFile();

        updateActions();
        connect(&command, &FilePathAspect::validChanged, this, [this, updateActions](bool validState) {
            haveValidFormatCommand = validState;
            updateActions();
        });
    }

    // Settings written before there was a built-in formatter carry no
    // "formatter" key. Where they have cmake-format wired up - formatting on
    // save, or a command of their own - stay with cmake-format, so that such
    // an installation keeps laying its files out as it did, through the
    // configuration files findConfigs() picks up next to them.
    static Formatter formatterOfStoredSettings()
    {
        const SettingsGroupNester nester({Constants::CMAKEFORMATTER_SETTINGS_GROUP,
                                          Constants::CMAKEFORMATTER_GENERAL_GROUP});
        QtcSettings &settings = userSettings();
        const QString storedCommand = settings.value("autoFormatCommand").toString();
        const bool wiredUp = settings.value("autoFormatOnSave", false).toBool()
                             || (!storedCommand.isEmpty()
                                 && storedCommand != QLatin1String(defaultFormatCommand));
        return wiredUp ? CMakeFormat : BuiltIn;
    }

    bool haveFormatter() const { return formatter() == BuiltIn || haveValidFormatCommand; }

    // Lays the document out through the indenter of the CMake editor.
    static void format(IDocument *document)
    {
        if (auto textDocument = qobject_cast<TextEditor::TextDocument *>(document))
            textDocument->formatContents();
    }

    bool isApplicable(const IDocument *document) const;

    void applyIfNecessary(IDocument *document, IDocument::SaveOption option) const;

    TextEditor::Command formatCommand() const
    {
        TextEditor::Command cmd;
        cmd.setExecutable(command());
        cmd.setProcessing(TextEditor::Command::FileProcessing);
        cmd.addOption("--in-place");
        cmd.addOption("%file");
        return cmd;
    }

    static FilePaths formatConfigFiles(const FilePath &dir)
    {
        if (dir.isEmpty())
            return FilePaths();

        static const QStringList files = {
            ".cmake-format",
            ".cmake-format.py",
            ".cmake-format.json",
            ".cmake-format.yaml",
            "cmake-format.py",
            "cmake-format.json",
            "cmake-format.yaml"
        };

        return filtered(transform(files,
                                  [dir](const QString &fileName) {
                                      return dir.pathAppended(fileName);
                                  }),
                        &FilePath::exists);
    }

    static FilePaths findConfigs(const FilePath &fileName)
    {
        for (const FilePath &parentDirectory : PathAndParents(fileName.parentDir())) {
            FilePaths configFiles = formatConfigFiles(parentDirectory);
            if (!configFiles.isEmpty())
                return configFiles;
        }
        return FilePaths();
    }

    static void extendCommandWithConfigs(TextEditor::Command &command, const FilePath &source)
    {
        const FilePaths configFiles = findConfigs(source);
        if (!configFiles.isEmpty()) {
            command.addOption("--config-files");
            command.addOptions(Utils::transform(configFiles, &FilePath::nativePath));
        }
    }

    SelectionAspect formatter{this};
    FilePathAspect command{this};
    bool haveValidFormatCommand{false};
    BoolAspect autoFormatOnSave{this};
    BoolAspect autoFormatOnlyCurrentProject{this};
    StringAspect autoFormatMime{this};

    QAction formatFile{Tr::tr("Format &Current File")};
};

bool CMakeFormatterSettings::isApplicable(const IDocument *document) const
{
    if (!document)
        return false;

    if (autoFormatMime().isEmpty())
        return true;

    const QStringList allowedMimeTypes = autoFormatMime().split(';');
    const MimeType documentMimeType = Utils::mimeTypeForName(document->mimeType());

    return anyOf(allowedMimeTypes, [&documentMimeType](const QString &mime) {
        return documentMimeType.inherits(mime);
    });
}

void CMakeFormatterSettings::applyIfNecessary(IDocument *document, IDocument::SaveOption option) const
{
    if (!autoFormatOnSave() || option != IDocument::SaveOption::None)
        return;

    if (!document)
        return;

    if (!isApplicable(document))
        return;

    // Check if file is contained in the current project (if wished)
    if (autoFormatOnlyCurrentProject()) {
        const ProjectExplorer::Project *pro = ProjectExplorer::ProjectTree::currentProject();
        if (!pro || pro->files([document](const ProjectExplorer::Node *n) {
                      return ProjectExplorer::Project::SourceFiles(n)
                             && n->filePath() == document->filePath();
                  }).isEmpty()) {
            return;
        }
    }

    if (formatter() == BuiltIn) {
        format(document);
        return;
    }

    TextEditor::Command command = formatCommand();
    if (!command.isValid())
        return;

    const QList<IEditor *> editors = DocumentModel::editorsForDocument(document);
    if (editors.isEmpty())
        return;

    IEditor *currentEditor = EditorManager::currentEditor();
    IEditor *editor = editors.contains(currentEditor) ? currentEditor : editors.first();
    if (auto widget = TextEditorWidget::fromEditor(editor)) {
        extendCommandWithConfigs(command, editor->document()->filePath());
        TextEditor::formatEditor(widget, command);
    }
}

static CMakeFormatterSettings &formatterSettings()
{
    static CMakeFormatterSettings theSettings;
    return theSettings;
}

bool cmakeFormatIsFormatter()
{
    return formatterSettings().formatter() == CMakeFormatterSettings::CMakeFormat;
}

void onFormatterChanged(QObject *guard, const std::function<void()> &handler)
{
    formatterSettings().formatter.addOnChanged(guard, handler);
}

class CMakeFormatterSettingsPage final : public Core::IOptionsPage
{
public:
    CMakeFormatterSettingsPage()
    {
        setId(Constants::Settings::FORMATTER_ID);
        setDisplayName(Tr::tr("Formatter"));
        setCategory(Constants::Settings::CATEGORY);
        setSettingsProvider([] { return &formatterSettings(); });
    }
};

void setupCMakeFormatter()
{
    static const CMakeFormatterSettingsPage theCMakeFormatterSettingsPage;

    formatterSettings();
};

} // CMakeProjectManager::Internal
