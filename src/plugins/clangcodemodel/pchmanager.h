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

#ifndef PCHMANAGER_H
#define PCHMANAGER_H

#include "clangprojectsettings.h"
#include "pchinfo.h"

#include <cpptools/cppmodelmanagerinterface.h>
#include <projectexplorer/project.h>
#include <coreplugin/messagemanager.h>

#include <QFutureWatcher>
#include <QHash>
#include <QMutex>
#include <QObject>

namespace ClangCodeModel {
namespace Internal {

class PCHManager: public QObject
{
    Q_OBJECT

    typedef CppTools::ProjectPart ProjectPart;

    static PCHManager *m_instance;

public:
    PCHManager(QObject *parent = 0);
    virtual ~PCHManager();

    static PCHManager *instance();

    PchInfo::Ptr pchInfo(const ProjectPart::Ptr &projectPart) const;
    ClangProjectSettings *settingsForProject(ProjectExplorer::Project *project);

signals:
    void pchInfoUpdated(); // TODO: check if this is used
    void pchMessage(const QString &message, Core::MessageManager::PrintToOutputPaneFlags flags);

public slots:
    void clangProjectSettingsChanged();
    void onAboutToRemoveProject(ProjectExplorer::Project *project);
    void onProjectPartsUpdated(ProjectExplorer::Project *project);

private slots:
    void updateActivePCHFiles();

private:
    void updatePchInfo(ClangProjectSettings *cps,
                       const QList<ProjectPart::Ptr> &projectParts);
    static void doPchInfoUpdate(QFutureInterface<void> &future,
                                ClangProjectSettings::PchUsage pchUsage,
                                const QString customPchFile,
                                const QList<ProjectPart::Ptr> projectParts);
    void setPCHInfo(const QList<ProjectPart::Ptr> &projectParts,
                    const PchInfo::Ptr &pchInfo,
                    const QPair<bool, QStringList> &msgs);
    PchInfo::Ptr findMatchingPCH(const QString &inputFileName, const QStringList &options,
                                 bool fuzzyMatching) const;

private:
    mutable QMutex m_mutex;
    QHash<ProjectPart::Ptr, PchInfo::Ptr> m_activePCHFiles;
    QHash<ProjectExplorer::Project *, ClangProjectSettings *> m_projectSettings;
    QFutureWatcher<void> m_pchGenerationWatcher;
};

} // namespace Internal
} // namespace ClangCodeModel

#endif // PCHMANAGER_H
