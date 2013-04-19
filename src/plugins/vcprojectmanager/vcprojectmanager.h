/**************************************************************************
**
** Copyright (c) 2013 Bojan Petrovic
** Copyright (c) 2013 Radovan Zivkvoic
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
#ifndef VCPROJECTMANAGER_H
#define VCPROJECTMANAGER_H

#include <projectexplorer/iprojectmanager.h>
#include <projectexplorer/projectnodes.h>

class QAction;

namespace VcProjectManager {
namespace Internal {

class VcProjectBuildOptionsPage;
struct MsBuildInformation;

class VcManager : public ProjectExplorer::IProjectManager
{
    Q_OBJECT

public:
    VcManager(VcProjectBuildOptionsPage *configPage);

    QString mimeType() const;
    ProjectExplorer::Project *openProject(const QString &fileName, QString *errorString);
    QVector<MsBuildInformation *> msBuilds() const;
    VcProjectBuildOptionsPage *buildOptionsPage();

private slots:
    void updateContextMenu(ProjectExplorer::Project *project, ProjectExplorer::Node *node);
    void onOptionsPageUpdate();

private:
    bool checkIfVersion2003(const QString &filePath) const;
    bool checkIfVersion2005(const QString &filePath) const;
    bool checkIfVersion2008(const QString &filePath) const;
    void readSchemaPath();

private:
    ProjectExplorer::Project *m_contextProject;
    VcProjectBuildOptionsPage *m_configPage;

    QString m_vc2003Schema;
    QString m_vc2005Schema;
    QString m_vc2008Schema;
};

} // namespace Internal
} // namespace VcProjectManager

#endif // VCPROJECTMANAGER_H
