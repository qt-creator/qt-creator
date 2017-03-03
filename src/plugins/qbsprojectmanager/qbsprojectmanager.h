/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#pragma once

#include "qbsprojectmanager_global.h"

#include <QList>
#include <QString>
#include <QVariantMap>

namespace qbs { class Settings; }

namespace ProjectExplorer {
class Kit;
class Project;
} // namespace ProjectExplorer

namespace QbsProjectManager {
namespace Internal {
class DefaultPropertyProvider;
class QbsLogSink;

class QbsManager : public QObject
{
    Q_OBJECT

public:
    QbsManager();
    ~QbsManager();

    // QBS profiles management:
    static QString profileForKit(const ProjectExplorer::Kit *k);
    void setProfileForKit(const QString &name, const ProjectExplorer::Kit *k);

    void updateProfileIfNecessary(const ProjectExplorer::Kit *kit);

    static qbs::Settings *settings();
    static Internal::QbsLogSink *logSink() { return m_logSink; }
    static QbsManager *instance() { return m_instance; }

private:
    void addProfile(const QString &name, const QVariantMap &data);
    void addQtProfileFromKit(const QString &profileName, const ProjectExplorer::Kit *k);
    void addProfileFromKit(const ProjectExplorer::Kit *k);
    void updateAllProfiles();

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
