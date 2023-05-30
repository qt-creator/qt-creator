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
#include <projectexplorer/projectnodes.h>
#include <projectexplorer/projecttree.h>

#include <texteditor/command.h>
#include <texteditor/formattexteditor.h>
#include <texteditor/texteditor.h>

#include <utils/algorithm.h>
#include <utils/genericconstants.h>
#include <utils/layoutbuilder.h>
#include <utils/mimeutils.h>

#include <QMenu>

using namespace Core;
using namespace TextEditor;
using namespace Utils;

namespace CMakeProjectManager::Internal {

class CMakeFormatterPrivate : public PagedSettings
{
public:
    CMakeFormatterPrivate()
    {
        setSettingsGroups(Constants::CMAKEFORMATTER_SETTINGS_GROUP,
                          Constants::CMAKEFORMATTER_GENERAL_GROUP);

        setId(Constants::Settings::FORMATTER_ID);
        setDisplayName(Tr::tr("Formatter"));
        setDisplayCategory("CMake");
        setCategory(Constants::Settings::CATEGORY);

        command.setSettingsKey("autoFormatCommand");
        command.setDefaultValue("cmake-format");
        command.setExpectedKind(PathChooser::ExistingCommand);

        autoFormatOnSave.setSettingsKey("autoFormatOnSave");
        autoFormatOnSave.setLabelText(Tr::tr("Enable auto format on file save"));

        autoFormatOnlyCurrentProject.setSettingsKey("autoFormatOnlyCurrentProject");
        autoFormatOnlyCurrentProject.setDefaultValue(true);
        autoFormatOnlyCurrentProject.setLabelText(Tr::tr("Restrict to files contained in the current project"));

        autoFormatMime.setSettingsKey("autoFormatMime");
        autoFormatMime.setDefaultValue("text/x-cmake");
        autoFormatMime.setLabelText(Tr::tr("Restrict to MIME types:"));

        setLayouter([this] {
            using namespace Layouting;
            return Column {
                Row { Tr::tr("CMakeFormat command:"), command },
                Space(10),
                Group {
                    title(Tr::tr("Automatic Formatting on File Save")),
                    autoFormatOnSave.groupChecker(),
                    Form {
                        autoFormatMime, br,
                        Span(2, autoFormatOnlyCurrentProject)
                    }
                },
                st
            };
        });

        ActionContainer *menu = ActionManager::createMenu(Constants::CMAKEFORMATTER_MENU_ID);
        menu->menu()->setTitle(Tr::tr("CMakeFormatter"));
        menu->setOnAllDisabledBehavior(ActionContainer::Show);
        ActionManager::actionContainer(Core::Constants::M_TOOLS)->addMenu(menu);

        Core::Command *cmd = ActionManager::registerAction(&formatFile, Constants::CMAKEFORMATTER_ACTION_ID);
        connect(&formatFile, &QAction::triggered, this, [this] {
            TextEditor::formatCurrentFile(formatCommand());
        });

        ActionManager::actionContainer(Constants::CMAKEFORMATTER_MENU_ID)->addAction(cmd);

        auto updateActions = [this] {
            auto editor = EditorManager::currentEditor();
            formatFile.setEnabled(editor && isApplicable(editor->document()));
        };

        connect(&autoFormatMime, &Utils::StringAspect::changed,
                this, updateActions);
        connect(EditorManager::instance(), &EditorManager::currentEditorChanged,
                this, updateActions);
        connect(EditorManager::instance(), &EditorManager::aboutToSave,
                this, &CMakeFormatterPrivate::applyIfNecessary);

        readSettings();
    }

    bool isApplicable(const IDocument *document) const;

    void applyIfNecessary(IDocument *document) const;

    TextEditor::Command formatCommand() const
    {
        TextEditor::Command cmd;
        cmd.setExecutable(command());
        cmd.setProcessing(TextEditor::Command::FileProcessing);
        cmd.addOption("--in-place");
        cmd.addOption("%file");
        return cmd;
    }

    FilePathAspect command{this};
    BoolAspect autoFormatOnSave{this};
    BoolAspect autoFormatOnlyCurrentProject{this};
    StringAspect autoFormatMime{this};

    QAction formatFile{Tr::tr("Format &Current File")};
};

bool CMakeFormatterPrivate::isApplicable(const IDocument *document) const
{
    if (!document)
        return false;

    if (autoFormatMime.value().isEmpty())
        return true;

    const QStringList allowedMimeTypes = autoFormatMime.value().split(';');
    const MimeType documentMimeType = Utils::mimeTypeForName(document->mimeType());

    return anyOf(allowedMimeTypes, [&documentMimeType](const QString &mime) {
        return documentMimeType.inherits(mime);
    });
}

void CMakeFormatterPrivate::applyIfNecessary(IDocument *document) const
{
    if (!autoFormatOnSave.value())
        return;

    if (!document)
        return;

    if (!isApplicable(document))
        return;

    // Check if file is contained in the current project (if wished)
    if (autoFormatOnlyCurrentProject.value()) {
        const ProjectExplorer::Project *pro = ProjectExplorer::ProjectTree::currentProject();
        if (!pro || pro->files([document](const ProjectExplorer::Node *n) {
                      return ProjectExplorer::Project::SourceFiles(n)
                             && n->filePath() == document->filePath();
                  }).isEmpty()) {
            return;
        }
    }

    TextEditor::Command command = formatCommand();
    if (!command.isValid())
        return;

    const QList<IEditor *> editors = DocumentModel::editorsForDocument(document);
    if (editors.isEmpty())
        return;

    IEditor *currentEditor = EditorManager::currentEditor();
    IEditor *editor = editors.contains(currentEditor) ? currentEditor : editors.first();
    if (auto widget = TextEditorWidget::fromEditor(editor))
        TextEditor::formatEditor(widget, command);
}

// CMakeFormatter

CMakeFormatter::CMakeFormatter()
    : d(new CMakeFormatterPrivate)
{}

CMakeFormatter::~CMakeFormatter()
{
    delete d;
}

void CMakeFormatter::applyIfNecessary(IDocument *document) const
{
    d->applyIfNecessary(document);
}

} // CMakeProjectManager::Internal
