/****************************************************************************
**
** Copyright (C) 2016 Lorenz Haas
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

#include "beautifierplugin.h"

#include "beautifierconstants.h"
#include "generaloptionspage.h"
#include "generalsettings.h"

#include "artisticstyle/artisticstyle.h"
#include "clangformat/clangformat.h"
#include "uncrustify/uncrustify.h"

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/editormanager/documentmodel.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/messagemanager.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectnodes.h>
#include <projectexplorer/projecttree.h>
#include <texteditor/formattexteditor.h>
#include <texteditor/textdocument.h>
#include <texteditor/textdocumentlayout.h>
#include <texteditor/texteditor.h>
#include <texteditor/texteditorconstants.h>
#include <utils/algorithm.h>
#include <utils/fileutils.h>
#include <utils/mimetypes/mimedatabase.h>
#include <utils/qtcassert.h>
#include <utils/runextensions.h>
#include <utils/synchronousprocess.h>
#include <utils/temporarydirectory.h>
#include <utils/textutils.h>

#include <QDir>
#include <QFileInfo>
#include <QFutureWatcher>
#include <QMenu>
#include <QPlainTextEdit>
#include <QProcess>
#include <QScrollBar>
#include <QTextBlock>

using namespace TextEditor;

namespace Beautifier {
namespace Internal {

bool isAutoFormatApplicable(const Core::IDocument *document,
                            const QList<Utils::MimeType> &allowedMimeTypes)
{
    if (!document)
        return false;

    if (allowedMimeTypes.isEmpty())
        return true;

    const Utils::MimeType documentMimeType = Utils::mimeTypeForName(document->mimeType());
    return Utils::anyOf(allowedMimeTypes, [&documentMimeType](const Utils::MimeType &mime) {
        return documentMimeType.inherits(mime.name());
    });
}

class BeautifierPluginPrivate : public QObject
{
public:
    BeautifierPluginPrivate();

    void updateActions(Core::IEditor *editor = nullptr);

    void autoFormatOnSave(Core::IDocument *document);

    GeneralSettings generalSettings;

    ArtisticStyle artisticStyleBeautifier;
    ClangFormat clangFormatBeautifier;
    Uncrustify uncrustifyBeautifier;

    BeautifierAbstractTool *m_tools[3] {
        &artisticStyleBeautifier,
        &uncrustifyBeautifier,
        &clangFormatBeautifier
    };

    GeneralOptionsPage optionsPage {{
        artisticStyleBeautifier.id(),
        uncrustifyBeautifier.id(),
        clangFormatBeautifier.id()
    }};
};

static BeautifierPluginPrivate *dd = nullptr;

bool BeautifierPlugin::initialize(const QStringList &arguments, QString *errorString)
{
    Q_UNUSED(arguments)
    Q_UNUSED(errorString)

    Core::ActionContainer *menu = Core::ActionManager::createMenu(Constants::MENU_ID);
    menu->menu()->setTitle(QCoreApplication::translate("Beautifier", "Bea&utifier"));
    menu->setOnAllDisabledBehavior(Core::ActionContainer::Show);
    Core::ActionManager::actionContainer(Core::Constants::M_TOOLS)->addMenu(menu);
    return true;
}

void BeautifierPlugin::extensionsInitialized()
{
    dd = new BeautifierPluginPrivate;
}

BeautifierPluginPrivate::BeautifierPluginPrivate()
{
    updateActions();

    const Core::EditorManager *editorManager = Core::EditorManager::instance();
    connect(editorManager, &Core::EditorManager::currentEditorChanged,
            this, &BeautifierPluginPrivate::updateActions);
    connect(editorManager, &Core::EditorManager::aboutToSave,
            this, &BeautifierPluginPrivate::autoFormatOnSave);
}

void BeautifierPluginPrivate::updateActions(Core::IEditor *editor)
{
    for (BeautifierAbstractTool *tool : m_tools)
        tool->updateActions(editor);
}

void BeautifierPluginPrivate::autoFormatOnSave(Core::IDocument *document)
{
    if (!generalSettings.autoFormatOnSave())
        return;

    if (!isAutoFormatApplicable(document, generalSettings.autoFormatMime()))
        return;

    // Check if file is contained in the current project (if wished)
    if (generalSettings.autoFormatOnlyCurrentProject()) {
        const ProjectExplorer::Project *pro = ProjectExplorer::ProjectTree::currentProject();
        if (!pro
            || pro->files([document](const ProjectExplorer::Node *n) {
                      return ProjectExplorer::Project::SourceFiles(n)
                             && n->filePath() == document->filePath();
                  })
                   .isEmpty()) {
            return;
        }
    }

    // Find tool to use by id and format file!
    const QString id = generalSettings.autoFormatTool();
    auto tool = std::find_if(std::begin(m_tools), std::end(m_tools),
                             [&id](const BeautifierAbstractTool *t){return t->id() == id;});
    if (tool != std::end(m_tools)) {
        if (!(*tool)->isApplicable(document))
            return;
        const TextEditor::Command command = (*tool)->command();
        if (!command.isValid())
            return;
        const QList<Core::IEditor *> editors = Core::DocumentModel::editorsForDocument(document);
        if (editors.isEmpty())
            return;
        if (auto widget = TextEditorWidget::fromEditor(editors.first()))
            TextEditor::formatEditor(widget, command);
    }
}

void BeautifierPlugin::showError(const QString &error)
{
    Core::MessageManager::write(tr("Error in Beautifier: %1").arg(error.trimmed()));
}

QString BeautifierPlugin::msgCannotGetConfigurationFile(const QString &command)
{
    return tr("Cannot get configuration file for %1.").arg(command);
}

QString BeautifierPlugin::msgFormatCurrentFile()
{
    //: Menu entry
    return tr("Format &Current File");
}

QString BeautifierPlugin::msgFormatSelectedText()
{
    //: Menu entry
    return tr("Format &Selected Text");
}

QString BeautifierPlugin::msgFormatAtCursor()
{
    //: Menu entry
    return tr("&Format at Cursor");
}

QString BeautifierPlugin::msgDisableFormattingSelectedText()
{
    //: Menu entry
    return tr("&Disable Formatting for Selected Text");
}

QString BeautifierPlugin::msgCommandPromptDialogTitle(const QString &command)
{
    //: File dialog title for path chooser when choosing binary
    return tr("%1 Command").arg(command);
}

} // namespace Internal
} // namespace Beautifier
