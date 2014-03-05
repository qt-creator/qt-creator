/**************************************************************************
**
** Copyright (c) 2014 Lorenz Haas
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "artisticstyle.h"

#include "artisticstyleconstants.h"
#include "artisticstyleoptionspage.h"
#include "artisticstylesettings.h"

#include "../beautifierconstants.h"
#include "../beautifierplugin.h"

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/icontext.h>
#include <coreplugin/icore.h>
#include <cppeditor/cppeditorconstants.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/project.h>
#include <utils/fileutils.h>

#include <QAction>
#include <QMenu>

namespace Beautifier {
namespace Internal {
namespace ArtisticStyle {

ArtisticStyle::ArtisticStyle(QObject *parent) :
    BeautifierAbstractTool(parent),
    m_settings(new ArtisticStyleSettings)
{
}

ArtisticStyle::~ArtisticStyle()
{
    delete m_settings;
}

bool ArtisticStyle::initialize()
{
    Core::ActionContainer *menu = Core::ActionManager::createMenu(Constants::ArtisticStyle::MENU_ID);
    menu->menu()->setTitle(QLatin1String("Artistic Style"));

    m_formatFile = new QAction(BeautifierPlugin::msgFormatCurrentFile(), this);
    Core::Command *cmd
            = Core::ActionManager::registerAction(m_formatFile,
                                                  Constants::ArtisticStyle::ACTION_FORMATFILE,
                                                  Core::Context(Core::Constants::C_GLOBAL));
    menu->addAction(cmd);
    connect(m_formatFile, SIGNAL(triggered()), this, SLOT(formatFile()));

    Core::ActionManager::actionContainer(Constants::MENU_ID)->addMenu(menu);

    return true;
}

void ArtisticStyle::updateActions(Core::IEditor *editor)
{
    m_formatFile->setEnabled(editor && editor->id() == CppEditor::Constants::CPPEDITOR_ID);
}

QList<QObject *> ArtisticStyle::autoReleaseObjects()
{
    ArtisticStyleOptionsPage *optionsPage = new ArtisticStyleOptionsPage(m_settings, this);
    return QList<QObject *>() << optionsPage;
}

void ArtisticStyle::formatFile()
{
    QString cfgFileName;

    if (m_settings->useOtherFiles()) {
        if (const ProjectExplorer::Project *project
                = ProjectExplorer::ProjectExplorerPlugin::currentProject()) {
            const QStringList files = project->files(ProjectExplorer::Project::AllFiles);
            for (int i = 0, total = files.size(); i < total; ++i) {
                const QString &file = files.at(i);
                if (!file.endsWith(QLatin1String(".astylerc")))
                    continue;
                const QFileInfo fi(file);
                if (fi.isReadable()) {
                    cfgFileName = file;
                    break;
                }
            }
        }
    }

    if (cfgFileName.isEmpty() && m_settings->useHomeFile()) {
        QString file = QDir::home().filePath(QLatin1String(".astylerc"));
        if (QFile::exists(file)) {
            cfgFileName = file;
        } else {
            file = QDir::home().filePath(QLatin1String("astylerc"));
            if (QFile::exists(file))
                cfgFileName = file;
        }
    }

    if (m_settings->useCustomStyle())
        cfgFileName = m_settings->styleFileName(m_settings->customStyle());

    if (cfgFileName.isEmpty()) {
        BeautifierPlugin::showError(BeautifierPlugin::msgCannotGetConfigurationFile(
                                        QLatin1String("Artistic Style")));
    } else {
        BeautifierPlugin::formatCurrentFile(QStringList()
                                            << m_settings->command()
                                            << QLatin1String("-q")
                                            << QLatin1String("--options=") + cfgFileName
                                            << QLatin1String("%file"));
    }
}

} // namespace ArtisticStyle
} // namespace Internal
} // namespace Beautifier
