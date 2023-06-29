// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "extpropertiesmview.h"

#include "modeleditortr.h"

#include "qmt/model/mpackage.h"
#include "qmt/project/project.h"
#include "qmt/project_controller/projectcontroller.h"

#include <utils/pathchooser.h>

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
            m_configPath->setPromptDialogTitle(Tr::tr("Select Custom Configuration Folder"));
            m_configPath->setExpectedKind(Utils::PathChooser::ExistingDirectory);
            m_configPath->setInitialBrowsePathBackup(
                Utils::FilePath::fromString(project->fileName()).absolutePath());
            addRow(Tr::tr("Config path:"), m_configPath, "configpath");
            connect(m_configPath, &Utils::PathChooser::textChanged,
                    this, &ExtPropertiesMView::onConfigPathChanged);
        }
        if (!m_configPath->hasFocus()) {
            if (project->configPath().isEmpty()) {
                m_configPath->setFilePath({});
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

void ExtPropertiesMView::onConfigPathChanged()
{
    const Utils::FilePath path = m_configPath->rawFilePath();
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
        QFileInfo absConfigPath = path.toFileInfo();
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
        m_configPathInfo->setText(Tr::tr("<font color=red>Model file must be reloaded.</font>"));
}

} // namespace Interal
} // namespace ModelEditor
