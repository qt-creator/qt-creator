/****************************************************************************
**
** Copyright (C) 2016 BogDan Vatra <bog_dan_ro@yahoo.com>
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

#include <projectexplorer/buildstep.h>
#include <projectexplorer/abstractprocessstep.h>

namespace QmakeAndroidSupport {
namespace Internal {

class AndroidPackageInstallationStep : public ProjectExplorer::AbstractProcessStep
{
    Q_OBJECT
    friend class AndroidPackageInstallationFactory;

public:
    explicit AndroidPackageInstallationStep(ProjectExplorer::BuildStepList *bsl);
    bool init(QList<const BuildStep *> &earlierSteps) override;

    ProjectExplorer::BuildStepConfigWidget *createConfigWidget() override;
    bool immutable() const override;

    void run(QFutureInterface<bool> &fi) override;
private:
    AndroidPackageInstallationStep(ProjectExplorer::BuildStepList *bc,
                                   AndroidPackageInstallationStep *other);
    QStringList m_androidDirsToClean;
    static const Core::Id Id;
};

class AndroidPackageInstallationStepWidget : public ProjectExplorer::BuildStepConfigWidget
{
    Q_OBJECT
public:
    AndroidPackageInstallationStepWidget(AndroidPackageInstallationStep *step);

    QString summaryText() const;
    QString displayName() const;
    bool showWidget() const;
private:
    AndroidPackageInstallationStep *m_step;
};

} // namespace Internal
} // namespace QmakeAndroidSupport
