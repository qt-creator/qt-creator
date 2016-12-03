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

#include <projectexplorer/iprojectmanager.h>

QT_BEGIN_NAMESPACE
class QAction;
class QDir;
QT_END_NAMESPACE

namespace ProjectExplorer { class Node; }
namespace Utils {
class Environment;
class QtcProcess;
} // namespace Utils

namespace CMakeProjectManager {
namespace Internal {

class CMakeSettingsPage;

class CMakeManager : public ProjectExplorer::IProjectManager
{
    Q_OBJECT
public:
    CMakeManager();

    ProjectExplorer::Project *openProject(const QString &fileName, QString *errorString) override;
    QString mimeType() const override;

    static void createXmlFile(Utils::QtcProcess *process, const QString &executable,
                              const QString &arguments, const QString &sourceDirectory,
                              const QDir &buildDirectory, const Utils::Environment &env);
    static QString findCbpFile(const QDir &);

private:
    void updateCmakeActions();
    void clearCMakeCache(ProjectExplorer::Project *project);
    void runCMake(ProjectExplorer::Project *project);
    void rescanProject(ProjectExplorer::Project *project);

    QAction *m_runCMakeAction;
    QAction *m_clearCMakeCacheAction;
    QAction *m_runCMakeActionContextMenu;
    QAction *m_rescanProjectAction;
};

} // namespace Internal
} // namespace CMakeProjectManager
