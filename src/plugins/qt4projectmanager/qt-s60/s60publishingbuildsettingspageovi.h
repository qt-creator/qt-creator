/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#ifndef S60PUBLISHINGBUILDSETTINGSPAGEOVI_H
#define S60PUBLISHINGBUILDSETTINGSPAGEOVI_H

#include <QWizardPage>

namespace ProjectExplorer {
class Project;
class BuildConfiguration;
class ToolChain;
}

namespace Qt4ProjectManager {
namespace Internal {

namespace Ui { class S60PublishingBuildSettingsPageOvi; }

class S60PublisherOvi;


class S60PublishingBuildSettingsPageOvi : public QWizardPage
{
    Q_OBJECT

public:
    explicit S60PublishingBuildSettingsPageOvi(S60PublisherOvi *publisher,
        const ProjectExplorer::Project *project, QWidget *parent = 0);
    ~S60PublishingBuildSettingsPageOvi();

    virtual bool isComplete() const;

signals:
    void configurationChosen();
    void toolchainConfigurationChosen();

private slots:
    void buildConfigChosen();
    void toolchainChosen();

private:
    void populateToolchainList(ProjectExplorer::BuildConfiguration *bc);

private:
    ProjectExplorer::BuildConfiguration *m_bc;
    ProjectExplorer::ToolChain *m_toolchain;
    Ui::S60PublishingBuildSettingsPageOvi *m_ui;
    S60PublisherOvi * const m_publisher;
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // S60PUBLISHINGBUILDSETTINGSPAGEOVI_H
