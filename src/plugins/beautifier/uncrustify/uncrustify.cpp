/**************************************************************************
**
** Copyright (C) 2015 Lorenz Haas
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

// Tested with version 0.59 and 0.60

#include "uncrustify.h"

#include "uncrustifyconstants.h"
#include "uncrustifyoptionspage.h"
#include "uncrustifysettings.h"

#include "../beautifierconstants.h"
#include "../beautifierplugin.h"

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/icontext.h>
#include <coreplugin/icore.h>
#include <coreplugin/idocument.h>
#include <cppeditor/cppeditorconstants.h>
#include <projectexplorer/projecttree.h>
#include <projectexplorer/project.h>
#include <texteditor/texteditor.h>
#include <utils/fileutils.h>

#include <QAction>
#include <QMenu>

namespace Beautifier {
namespace Internal {
namespace Uncrustify {

Uncrustify::Uncrustify(BeautifierPlugin *parent) :
    BeautifierAbstractTool(parent),
    m_beautifierPlugin(parent),
    m_formatFile(nullptr),
    m_formatRange(nullptr),
    m_settings(new UncrustifySettings)
{
}

Uncrustify::~Uncrustify()
{
    delete m_settings;
}

bool Uncrustify::initialize()
{
    Core::ActionContainer *menu = Core::ActionManager::createMenu(Constants::Uncrustify::MENU_ID);
    menu->menu()->setTitle(QLatin1String(Constants::Uncrustify::DISPLAY_NAME));

    m_formatFile = new QAction(BeautifierPlugin::msgFormatCurrentFile(), this);
    Core::Command *cmd
            = Core::ActionManager::registerAction(m_formatFile,
                                                  Constants::Uncrustify::ACTION_FORMATFILE);
    menu->addAction(cmd);
    connect(m_formatFile, &QAction::triggered, this, &Uncrustify::formatFile);

    m_formatRange = new QAction(BeautifierPlugin::msgFormatSelectedText(), this);
    cmd = Core::ActionManager::registerAction(m_formatRange,
                                              Constants::Uncrustify::ACTION_FORMATSELECTED);
    menu->addAction(cmd);
    connect(m_formatRange, &QAction::triggered, this, &Uncrustify::formatSelectedText);

    Core::ActionManager::actionContainer(Constants::MENU_ID)->addMenu(menu);

    return true;
}

void Uncrustify::updateActions(Core::IEditor *editor)
{
    const bool enabled = (editor && editor->document()->id() == CppEditor::Constants::CPPEDITOR_ID);
    m_formatFile->setEnabled(enabled);
    m_formatRange->setEnabled(enabled);
}

QList<QObject *> Uncrustify::autoReleaseObjects()
{
    UncrustifyOptionsPage *optionsPage = new UncrustifyOptionsPage(m_settings, this);
    return QList<QObject *>() << optionsPage;
}

void Uncrustify::formatFile()
{
    const QString cfgFileName = configurationFile();
    if (cfgFileName.isEmpty()) {
        BeautifierPlugin::showError(BeautifierPlugin::msgCannotGetConfigurationFile(
                                        QLatin1String(Constants::Uncrustify::DISPLAY_NAME)));
    } else {
        m_beautifierPlugin->formatCurrentFile(command(cfgFileName));
    }
}

void Uncrustify::formatSelectedText()
{
    const QString cfgFileName = configurationFile();
    if (cfgFileName.isEmpty()) {
        BeautifierPlugin::showError(BeautifierPlugin::msgCannotGetConfigurationFile(
                                        QLatin1String(Constants::Uncrustify::DISPLAY_NAME)));
        return;
    }

    const TextEditor::TextEditorWidget *widget
            = TextEditor::TextEditorWidget::currentTextEditorWidget();
    if (!widget)
        return;

    QTextCursor tc = widget->textCursor();
    if (tc.hasSelection()) {
        // Extend selection to full lines
        const int posSelectionEnd = tc.selectionEnd();
        tc.movePosition(QTextCursor::StartOfLine);
        const int startPos = tc.position();
        tc.setPosition(posSelectionEnd);
        tc.movePosition(QTextCursor::EndOfLine);
        const int endPos = tc.position();
        m_beautifierPlugin->formatCurrentFile(command(cfgFileName, true), startPos, endPos);
    }
}

QString Uncrustify::configurationFile() const
{
    if (m_settings->useCustomStyle())
        return m_settings->styleFileName(m_settings->customStyle());

    if (m_settings->useOtherFiles()) {
        if (const ProjectExplorer::Project *project
                = ProjectExplorer::ProjectTree::currentProject()) {
            const QStringList files = project->files(ProjectExplorer::Project::AllFiles);
            foreach (const QString &file, files) {
                if (!file.endsWith(QLatin1String("cfg")))
                    continue;
                const QFileInfo fi(file);
                if (fi.isReadable() && fi.fileName() == QLatin1String("uncrustify.cfg"))
                    return file;
            }
        }
    }

    if (m_settings->useHomeFile()) {
        const QString file = QDir::home().filePath(QLatin1String("uncrustify.cfg"));
        if (QFile::exists(file))
            return file;
    }

    return QString();
}

Command Uncrustify::command(const QString &cfgFile, bool fragment) const
{
    Command command;
    command.setExecutable(m_settings->command());
    command.setProcessing(Command::PipeProcessing);
    command.addOption(QLatin1String("-l"));
    command.addOption(QLatin1String("cpp"));
    command.addOption(QLatin1String("-L"));
    command.addOption(QLatin1String("1-2"));
    if (fragment)
        command.addOption(QLatin1String("--frag"));
    command.addOption(QLatin1String("-c"));
    command.addOption(cfgFile);
    return command;
}

} // namespace Uncrustify
} // namespace Internal
} // namespace Beautifier
