/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/
#ifndef MAEMOPUBLISHINGBUILDSETTINGSPAGEFREMANTLEFREE_H
#define MAEMOPUBLISHINGBUILDSETTINGSPAGEFREMANTLEFREE_H

#include <QtCore/QList>
#include <QtGui/QWizardPage>

QT_BEGIN_NAMESPACE
namespace Ui {
    class MaemoPublishingWizardPageFremantleFree;
}
QT_END_NAMESPACE

namespace ProjectExplorer { class Project; }

namespace Qt4ProjectManager {
class Qt4BuildConfiguration;

namespace Internal {
class MaemoPublisherFremantleFree;

class MaemoPublishingBuildSettingsPageFremantleFree : public QWizardPage
{
    Q_OBJECT

public:
    explicit MaemoPublishingBuildSettingsPageFremantleFree(const ProjectExplorer::Project *project,
        MaemoPublisherFremantleFree *publisher, QWidget *parent = 0);
    ~MaemoPublishingBuildSettingsPageFremantleFree();

private:
    Q_SLOT void handleNoUploadSettingChanged();
    virtual bool validatePage();
    void collectBuildConfigurations(const ProjectExplorer::Project *project);
    bool skipUpload() const;

    QList<Qt4BuildConfiguration *> m_buildConfigs;
    MaemoPublisherFremantleFree * const m_publisher;
    Ui::MaemoPublishingWizardPageFremantleFree *ui;
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // MAEMOPUBLISHINGBUILDSETTINGSPAGEFREMANTLEFREE_H
