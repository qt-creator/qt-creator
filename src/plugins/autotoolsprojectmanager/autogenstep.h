/**************************************************************************
**
** Copyright (C) 2015 Openismus GmbH.
** Authors: Peter Penz (ppenz@openismus.com)
**          Patricia Santana Cruz (patriciasantanacruz@gmail.com)
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

#ifndef AUTOGENSTEP_H
#define AUTOGENSTEP_H

#include <projectexplorer/abstractprocessstep.h>

QT_BEGIN_NAMESPACE
class QLineEdit;
QT_END_NAMESPACE

namespace AutotoolsProjectManager {
namespace Internal {

class AutotoolsProject;
class AutogenStep;
class AutogenStepConfigWidget;

/////////////////////////////
// AutogenStepFactory class
/////////////////////////////
/**
 * @brief Implementation of the ProjectExplorer::IBuildStepFactory interface.
 *
 * This factory is used to create instances of AutogenStep.
 */
class AutogenStepFactory : public ProjectExplorer::IBuildStepFactory
{
    Q_OBJECT

public:
    AutogenStepFactory(QObject *parent = 0);

    QList<Core::Id> availableCreationIds(ProjectExplorer::BuildStepList *bc) const;
    QString displayNameForId(Core::Id id) const;
    bool canCreate(ProjectExplorer::BuildStepList *parent, Core::Id id) const;
    ProjectExplorer::BuildStep *create(ProjectExplorer::BuildStepList *parent, Core::Id id);
    bool canClone(ProjectExplorer::BuildStepList *parent, ProjectExplorer::BuildStep *source) const;
    ProjectExplorer::BuildStep *clone(ProjectExplorer::BuildStepList *parent, ProjectExplorer::BuildStep *source);
    bool canRestore(ProjectExplorer::BuildStepList *parent, const QVariantMap &map) const;
    ProjectExplorer::BuildStep *restore(ProjectExplorer::BuildStepList *parent, const QVariantMap &map);

    bool canHandle(ProjectExplorer::BuildStepList *parent) const;
};

///////////////////////
// AutogenStep class
///////////////////////
/**
 * @brief Implementation of the ProjectExplorer::AbstractProcessStep interface.
 *
 * A autogen step can be configured by selecting the "Projects" button of Qt Creator
 * (in the left hand side menu) and under "Build Settings".
 *
 * It is possible for the user to specify custom arguments. The corresponding
 * configuration widget is created by AutogenStep::createConfigWidget and is
 * represented by an instance of the class AutogenStepConfigWidget.
 */

class AutogenStep : public ProjectExplorer::AbstractProcessStep
{
    Q_OBJECT
    friend class AutogenStepFactory;
    friend class AutogenStepConfigWidget;

public:
    explicit AutogenStep(ProjectExplorer::BuildStepList *bsl);

    bool init(QList<const BuildStep *> &earlierSteps) override;
    void run(QFutureInterface<bool> &interface) override;
    ProjectExplorer::BuildStepConfigWidget *createConfigWidget() override;
    bool immutable() const override;
    QString additionalArguments() const;
    QVariantMap toMap() const override;

public slots:
    void setAdditionalArguments(const QString &list);

signals:
    void additionalArgumentsChanged(const QString &);

protected:
    AutogenStep(ProjectExplorer::BuildStepList *bsl, AutogenStep *bs);
    AutogenStep(ProjectExplorer::BuildStepList *bsl, Core::Id id);

    bool fromMap(const QVariantMap &map) override;

private:
    void ctor();

    QString m_additionalArguments;
    bool m_runAutogen;
};

//////////////////////////////////
// AutogenStepConfigWidget class
//////////////////////////////////
/**
 * @brief Implementation of the ProjectExplorer::BuildStepConfigWidget interface.
 *
 * Allows to configure a autogen step in the GUI.
 */
class AutogenStepConfigWidget : public ProjectExplorer::BuildStepConfigWidget
{
    Q_OBJECT

public:
    AutogenStepConfigWidget(AutogenStep *autogenStep);

    QString displayName() const;
    QString summaryText() const;

private slots:
    void updateDetails();

private:
    AutogenStep *m_autogenStep;
    QString m_summaryText;
    QLineEdit *m_additionalArguments;
};

} // namespace Internal
} // namespace AutotoolsProjectManager

#endif // AUTOGENSTEP_H

