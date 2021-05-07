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

#include "cppprojectupdaterinterface.h"
#include "cpptools_global.h"
#include "projectinfo.h"

#include <projectexplorer/extracompiler.h>
#include <utils/futuresynchronizer.h>

#include <QFutureWatcher>

namespace CppTools {

class ProjectInfo;

// registered in extensionsystem's object pool for plugins with weak dependency to CppTools
class CPPTOOLS_EXPORT CppProjectUpdaterFactory : public QObject
{
    Q_OBJECT
public:
    CppProjectUpdaterFactory();

    // keep the namespace, for the type name in the invokeMethod call
    Q_INVOKABLE CppTools::CppProjectUpdaterInterface *create();
};

class CPPTOOLS_EXPORT CppProjectUpdater final : public QObject, public CppProjectUpdaterInterface
{
    Q_OBJECT

public:
    CppProjectUpdater();
    ~CppProjectUpdater() override;

    void update(const ProjectExplorer::ProjectUpdateInfo &projectUpdateInfo) override;
    void update(const ProjectExplorer::ProjectUpdateInfo &projectUpdateInfo,
                const QList<ProjectExplorer::ExtraCompiler *> &extraCompilers);
    void cancel() override;

private:
    void onProjectInfoGenerated();
    void checkForExtraCompilersFinished();

private:
    ProjectExplorer::ProjectUpdateInfo m_projectUpdateInfo;
    QList<QPointer<ProjectExplorer::ExtraCompiler>> m_extraCompilers;

    QFutureWatcher<ProjectInfo::Ptr> m_generateFutureWatcher;
    bool m_isProjectInfoGenerated = false;
    QSet<QFutureWatcher<void> *> m_extraCompilersFutureWatchers;
    std::unique_ptr<QFutureInterface<void>> m_projectUpdateFutureInterface;
    Utils::FutureSynchronizer m_futureSynchronizer;
};

} // namespace CppTools
