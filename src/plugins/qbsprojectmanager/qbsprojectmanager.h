/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
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

#ifndef QBSPROJECTMANAGER_H
#define QBSPROJECTMANAGER_H

#include "qbsprojectmanager_global.h"

#include <projectexplorer/iprojectmanager.h>

#include <QList>

namespace qbs {
class Settings;
class Preferences;
} // namespace qbs

#include <QString>
#include <QVariantMap>

namespace ProjectExplorer {
class Kit;
class Project;
class ProjectExplorerPlugin;
} // namespace ProjectExplorer

namespace QbsProjectManager {
namespace Internal {
class DefaultPropertyProvider;
class QbsLogSink;
class QbsProject;

class QbsManager : public ProjectExplorer::IProjectManager
{
    Q_OBJECT

public:
    QbsManager();
    ~QbsManager();

    QString mimeType() const;
    ProjectExplorer::Project *openProject(const QString &fileName, QString *errorString);

    // QBS profiles management:
    QString profileForKit(const ProjectExplorer::Kit *k);
    void setProfileForKit(const QString &name, const ProjectExplorer::Kit *k);

    void updateProfileIfNecessary(const ProjectExplorer::Kit *kit);

    static qbs::Settings *settings() { return m_settings; }
    static Internal::QbsLogSink *logSink() { return m_logSink; }
    static QbsManager *instance() { return m_instance; }

private:
    void addProfile(const QString &name, const QVariantMap &data);
    void addQtProfileFromKit(const QString &profileName, const ProjectExplorer::Kit *k);
    void addProfileFromKit(const ProjectExplorer::Kit *k);

    void handleKitUpdate(ProjectExplorer::Kit *kit);
    void handleKitRemoval(ProjectExplorer::Kit *kit);

    static QbsLogSink *m_logSink;
    static qbs::Settings *m_settings;

    DefaultPropertyProvider *m_defaultPropertyProvider;
    QList<ProjectExplorer::Kit *> m_kitsToBeSetupForQbs;
    static QbsManager *m_instance;
};

} // namespace Internal
} // namespace QbsProjectManager

#endif // QBSPROJECTMANAGER_H
