/****************************************************************************
**
** Copyright (C) 2016 Jochen Becher
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

#include "extpropertiesmview.h"

#include "qmt/model/mpackage.h"
#include "qmt/project/project.h"
#include "qmt/project_controller/projectcontroller.h"

#include "utils/pathchooser.h"

#include <QLabel>
#include <QFileInfo>
#include <QDir>

namespace ModelEditor {
namespace Internal {

ExtPropertiesMView::ExtPropertiesMView(qmt::PropertiesView *view)
    : qmt::PropertiesView::MView(view)
{
}

ExtPropertiesMView::~ExtPropertiesMView()
{
}

void ExtPropertiesMView::setProjectController(qmt::ProjectController *projectController)
{
    m_projectController = projectController;
}

void ExtPropertiesMView::visitMPackage(const qmt::MPackage *package)
{
    qmt::PropertiesView::MView::visitMPackage(package);
    if (m_modelElements.size() == 1 && !package->owner()) {
        qmt::Project *project = m_projectController->project();
        if (!m_configPath) {
            m_configPath = new Utils::PathChooser(m_topWidget);
            m_configPath->setPromptDialogTitle(tr("Select Custom Configuration Folder"));
            m_configPath->setExpectedKind(Utils::PathChooser::ExistingDirectory);
            m_configPath->setValidationFunction([=](Utils::FancyLineEdit *edit, QString *errorMessage) {
                return edit->text().isEmpty() || m_configPath->defaultValidationFunction()(edit, errorMessage);
            });
            m_configPath->setInitialBrowsePathBackup(QFileInfo(project->fileName()).absolutePath());
            addRow(tr("Config path:"), m_configPath, "configpath");
            connect(m_configPath, &Utils::PathChooser::pathChanged,
                    this, &ExtPropertiesMView::onConfigPathChanged);
        }
        if (!m_configPath->hasFocus()) {
            if (project->configPath().isEmpty()) {
                m_configPath->setPath(QString());
            } else {
                // make path absolute (may be relative to current project's directory)
                QDir projectDir = QFileInfo(project->fileName()).absoluteDir();
                m_configPath->setPath(QFileInfo(projectDir, project->configPath()).canonicalFilePath());
            }
        }
        if (!m_configPathInfo) {
            m_configPathInfo = new QLabel(m_topWidget);
            addRow(QString(), m_configPathInfo, "configpathinfo");
        }
    }
}

void ExtPropertiesMView::onConfigPathChanged(const QString &path)
{
    bool modified = false;
    qmt::Project *project = m_projectController->project();
    if (path.isEmpty()) {
        if (!project->configPath().isEmpty()) {
            project->setConfigPath(QString());
            m_projectController->setModified();
            modified = true;
        }
    } else {
        // make path relative to current project's directory
        QFileInfo absConfigPath(path);
        absConfigPath.makeAbsolute();
        QDir projectDir = QFileInfo(project->fileName()).dir();
        QString configPath = projectDir.relativeFilePath(absConfigPath.filePath());
        if (configPath != project->configPath()) {
            project->setConfigPath(configPath);
            m_projectController->setModified();
            modified = true;
        }
    }
    if (modified && m_configPathInfo)
        m_configPathInfo->setText(tr("<font color=red>Model file must be reloaded.</font>"));
}

} // namespace Interal
} // namespace ModelEditor
