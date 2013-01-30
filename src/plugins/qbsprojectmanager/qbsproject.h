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

#ifndef QBSPROJECT_H
#define QBSPROJECT_H

#include "qbsprojectmanager.h"

#include <language/language.h>
#include <tools/buildoptions.h>

#include <projectexplorer/project.h>
#include <projectexplorer/projectnodes.h>
#include <projectexplorer/task.h>

#include <QFuture>
// <debug>
#include <QTimer>
// </debug>
#include <QVariantMap>

namespace qbs {
class BuildJob;
class CleanJob;
class Error;
class ProjectData;
class SetupProjectJob;
} // namespace qbs

namespace Core { class IDocument; }
namespace ProjectExplorer { class BuildConfiguration; }

namespace QbsProjectManager {
namespace Internal {

class QbsProjectNode;
class QbsBuildConfiguration;

class QbsProject : public ProjectExplorer::Project
{
    Q_OBJECT

public:
    QbsProject(QbsManager *manager, const QString &filename);
    ~QbsProject();

    QString displayName() const;
    Core::Id id() const;
    Core::IDocument *document() const;
    QbsManager *projectManager() const;

    ProjectExplorer::ProjectNode *rootProjectNode() const;

    QStringList files(FilesMode fileMode) const;

    qbs::BuildJob *build(const qbs::BuildOptions &opts);
    qbs::CleanJob *clean(const qbs::BuildOptions &opts, bool everything);

    static ProjectExplorer::FileType fileTypeFor(const QSet<QString> &tags);

    QString profileForTarget(const ProjectExplorer::Target *t) const;
    bool isParsing() const;
    bool hasParseResult() const;

    Utils::FileName defaultBuildDirectory() const;

public slots:
    void invalidate();
    void parseCurrentBuildConfiguration();

signals:
    void projectParsingStarted();
    void projectParsingDone(bool);

private slots:
    void handleQbsParsingDone(bool success);
    void handleQbsParsingProgress(int progress);
    void handleQbsParsingTaskSetup(const QString &description, int maximumProgressValue);

    void targetWasAdded(ProjectExplorer::Target *t);
    void changeActiveTarget(ProjectExplorer::Target *t);
    void buildConfigurationChanged(ProjectExplorer::BuildConfiguration *bc);

private:
    bool fromMap(const QVariantMap &map);

    void parse(const QVariantMap &config, const QString &dir = QString());

    void generateErrors(const qbs::Error &e);
    void prepareForParsing();
    void updateDocuments(const qbs::ProjectData *prj);
    void updateCppCodeModel(const qbs::ProjectData *prj);
    void updateQmlJsCodeModel(const qbs::ProjectData *prj);

    QbsManager *const m_manager;
    const QString m_projectName;
    const QString m_fileName;
    QSet<Core::IDocument *> m_qbsDocuments;
    QbsProjectNode *const m_rootProjectNode;

    qbs::SetupProjectJob *m_qbsSetupProjectJob;
    QVariantMap m_qbsBuildConfig;
    QString m_qbsBuildRoot;

    QFutureInterface<void> *m_qbsUpdateFutureInterface;
    int m_currentProgressBase;

    QFuture<void> m_codeModelFuture;

    QbsBuildConfiguration *m_currentBc;
};

} // namespace Internal
} // namespace QbsProjectManager

#endif // QBSPROJECT_H
