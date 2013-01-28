/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#ifndef MAEMOPUBLISHINGBUILDSETTINGSPAGEFREMANTLEFREE_H
#define MAEMOPUBLISHINGBUILDSETTINGSPAGEFREMANTLEFREE_H

#include <QList>
#include <QWizardPage>

namespace ProjectExplorer { class Project; }
namespace Qt4ProjectManager { class Qt4BuildConfiguration; }

namespace Madde {
namespace Internal {

class MaemoPublisherFremantleFree;
namespace Ui { class MaemoPublishingWizardPageFremantleFree; }

class MaemoPublishingBuildSettingsPageFremantleFree : public QWizardPage
{
    Q_OBJECT

public:
    explicit MaemoPublishingBuildSettingsPageFremantleFree(const ProjectExplorer::Project *project,
                                                           MaemoPublisherFremantleFree *publisher,
                                                           QWidget *parent = 0);
    ~MaemoPublishingBuildSettingsPageFremantleFree();

private:
    Q_SLOT void handleNoUploadSettingChanged();
    virtual void initializePage();
    virtual bool validatePage();
    void collectBuildConfigurations(const ProjectExplorer::Project *project);
    bool skipUpload() const;

    QList<Qt4ProjectManager::Qt4BuildConfiguration *> m_buildConfigs;
    MaemoPublisherFremantleFree * const m_publisher;
    Ui::MaemoPublishingWizardPageFremantleFree *ui;
};

} // namespace Internal
} // namespace Madde

#endif // MAEMOPUBLISHINGBUILDSETTINGSPAGEFREMANTLEFREE_H
