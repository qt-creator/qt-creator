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

#ifndef PCHMANAGER_H
#define PCHMANAGER_H

#include "clangprojectsettings.h"
#include "pchinfo.h"

#include <cpptools/cppmodelmanager.h>
#include <projectexplorer/project.h>
#include <coreplugin/messagemanager.h>

#include <QFutureWatcher>
#include <QHash>
#include <QMutex>
#include <QObject>

namespace ClangCodeModel {
namespace Internal {

class PchManager : public QObject
{
    Q_OBJECT

    typedef CppTools::ProjectPart ProjectPart;

    static PchManager *m_instance;

public:
    PchManager(QObject *parent = 0);
    virtual ~PchManager();

    static PchManager *instance();

    PchInfo::Ptr pchInfo(const ProjectPart::Ptr &projectPart) const;
    ClangProjectSettings *settingsForProject(ProjectExplorer::Project *project);

signals:
    void pchMessage(const QString &message, Core::MessageManager::PrintToOutputPaneFlags flags);

public slots:
    void clangProjectSettingsChanged();
    void onAboutToRemoveProject(ProjectExplorer::Project *project);
    void onProjectPartsUpdated(ProjectExplorer::Project *project);

private slots:
    void updateActivePchFiles();

private:
    struct UpdateParams {
        UpdateParams(const QString &customPchFile, const QList<ProjectPart::Ptr> &projectParts)
            : customPchFile(customPchFile) , projectParts(projectParts) {}
        const QString customPchFile;
        const QList<ProjectPart::Ptr> projectParts;
    };

    void updatePchInfo(ClangProjectSettings *cps,
                       const QList<ProjectPart::Ptr> &projectParts);

    static void doPchInfoUpdateNone(QFutureInterface<void> &future, const UpdateParams params);
    static void doPchInfoUpdateFuzzy(QFutureInterface<void> &future, const UpdateParams params);
    static void doPchInfoUpdateExact(QFutureInterface<void> &future, const UpdateParams params);
    static void doPchInfoUpdateCustom(QFutureInterface<void> &future, const UpdateParams params);

    void setPCHInfo(const QList<ProjectPart::Ptr> &projectParts,
                    const PchInfo::Ptr &pchInfo,
                    const QPair<bool, QStringList> &msgs);
    PchInfo::Ptr findMatchingPCH(const QString &inputFileName, const QStringList &options,
                                 bool fuzzyMatching) const;

private:
    mutable QMutex m_mutex;
    QHash<ProjectPart::Ptr, PchInfo::Ptr> m_activePchFiles;
    QHash<ProjectExplorer::Project *, ClangProjectSettings *> m_projectSettings;
    QFutureWatcher<void> m_pchGenerationWatcher;
};

} // namespace Internal
} // namespace ClangCodeModel

#endif // PCHMANAGER_H
