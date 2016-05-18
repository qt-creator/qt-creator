/****************************************************************************
**
** Copyright (C) 2016 Openismus GmbH.
** Author: Peter Penz (ppenz@openismus.com)
** Author: Patricia Santana Cruz (patriciasantanacruz@gmail.com)
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

#include <projectexplorer/abstractprocessstep.h>

QT_BEGIN_NAMESPACE
class QLineEdit;
QT_END_NAMESPACE

namespace AutotoolsProjectManager {
namespace Internal {

class AutotoolsProject;
class AutoreconfStep;

////////////////////////////////
// AutoreconfStepFactory class
////////////////////////////////
/**
 * @brief Implementation of the ProjectExplorer::IBuildStepFactory interface.
 *
 * The factory is used to create instances of AutoreconfStep.
 */
class AutoreconfStepFactory : public ProjectExplorer::IBuildStepFactory
{
    Q_OBJECT

public:
    AutoreconfStepFactory(QObject *parent = 0);

    QList<ProjectExplorer::BuildStepInfo>
        availableSteps(ProjectExplorer::BuildStepList *parent) const override;

    ProjectExplorer::BuildStep *create(ProjectExplorer::BuildStepList *parent, Core::Id id) override;
    ProjectExplorer::BuildStep *clone(ProjectExplorer::BuildStepList *parent, ProjectExplorer::BuildStep *source) override;
};

/////////////////////////
// AutoreconfStep class
/////////////////////////
/**
 * @brief Implementation of the ProjectExplorer::AbstractProcessStep interface.
 *
 * A autoreconf step can be configured by selecting the "Projects" button
 * of Qt Creator (in the left hand side menu) and under "Build Settings".
 *
 * It is possible for the user to specify custom arguments. The corresponding
 * configuration widget is created by AutoreconfStep::createConfigWidget and is
 * represented by an instance of the class AutoreconfStepConfigWidget.
 */

class AutoreconfStep : public ProjectExplorer::AbstractProcessStep
{
    Q_OBJECT
    friend class AutoreconfStepFactory;
    friend class AutoreconfStepConfigWidget;

public:
    explicit AutoreconfStep(ProjectExplorer::BuildStepList *bsl);

    bool init(QList<const BuildStep *> &earlierSteps) override;
    void run(QFutureInterface<bool> &fi) override;
    ProjectExplorer::BuildStepConfigWidget *createConfigWidget() override;
    bool immutable() const override;
    QString additionalArguments() const;
    QVariantMap toMap() const override;

    void setAdditionalArguments(const QString &list);

signals:
    void additionalArgumentsChanged(const QString &);

protected:
    AutoreconfStep(ProjectExplorer::BuildStepList *bsl, AutoreconfStep *bs);
    AutoreconfStep(ProjectExplorer::BuildStepList *bsl, Core::Id id);

    bool fromMap(const QVariantMap &map) override;

private:
    void ctor();

    QString m_additionalArguments;
    bool m_runAutoreconf = false;
};

//////////////////////////////////////
// AutoreconfStepConfigWidget class
//////////////////////////////////////
/**
 * @brief Implementation of the ProjectExplorer::BuildStepConfigWidget interface.
 *
 * Allows to configure a autoreconf step in the GUI..
 */
class AutoreconfStepConfigWidget : public ProjectExplorer::BuildStepConfigWidget
{
    Q_OBJECT

public:
    AutoreconfStepConfigWidget(AutoreconfStep *autoreconfStep);

    QString displayName() const override;
    QString summaryText() const override;

private:
    void updateDetails();

    AutoreconfStep *m_autoreconfStep;
    QString m_summaryText;
    QLineEdit *m_additionalArguments;
};

} // namespace Internal
} // namespace AutotoolsProjectManager
