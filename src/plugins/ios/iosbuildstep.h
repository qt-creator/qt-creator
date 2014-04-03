/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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
#ifndef IOSBUILDSTEP_H
#define IOSBUILDSTEP_H

#include <projectexplorer/abstractprocessstep.h>
#include <utils/qtcoverride.h>

QT_BEGIN_NAMESPACE
class QListWidgetItem;
QT_END_NAMESPACE

namespace Ios {
namespace Internal {

class IosBuildStepConfigWidget;
class IosBuildStepFactory;
namespace Ui { class IosBuildStep; }

class IosBuildStep : public ProjectExplorer::AbstractProcessStep
{
    Q_OBJECT

    friend class IosBuildStepConfigWidget;
    friend class IosBuildStepFactory;

public:
    IosBuildStep(ProjectExplorer::BuildStepList *parent);
    ~IosBuildStep();

    bool init() QTC_OVERRIDE;
    void run(QFutureInterface<bool> &fi) QTC_OVERRIDE;

    ProjectExplorer::BuildStepConfigWidget *createConfigWidget() QTC_OVERRIDE;
    bool immutable() const QTC_OVERRIDE;
    void setBaseArguments(const QStringList &args);
    void setExtraArguments(const QStringList &extraArgs);
    QStringList baseArguments() const;
    QStringList allArguments() const;
    QStringList defaultArguments() const;
    QString buildCommand() const;

    void setClean(bool clean);
    bool isClean() const;

    QVariantMap toMap() const QTC_OVERRIDE;
protected:
    IosBuildStep(ProjectExplorer::BuildStepList *parent, IosBuildStep *bs);
    IosBuildStep(ProjectExplorer::BuildStepList *parent, const Core::Id id);
    bool fromMap(const QVariantMap &map) QTC_OVERRIDE;

private:
    void ctor();

    QStringList m_baseBuildArguments;
    QStringList m_extraArguments;
    QString m_buildCommand;
    bool m_useDefaultArguments;
    bool m_clean;
};

class IosBuildStepConfigWidget : public ProjectExplorer::BuildStepConfigWidget
{
    Q_OBJECT

public:
    IosBuildStepConfigWidget(IosBuildStep *buildStep);
    ~IosBuildStepConfigWidget();
    QString displayName() const QTC_OVERRIDE;
    QString summaryText() const QTC_OVERRIDE;

private slots:
    void buildArgumentsChanged();
    void resetDefaultArguments();
    void extraArgumentsChanged();
    void updateDetails();

private:
    Ui::IosBuildStep *m_ui;
    IosBuildStep *m_buildStep;
    QString m_summaryText;
};

class IosBuildStepFactory : public ProjectExplorer::IBuildStepFactory
{
    Q_OBJECT

public:
    explicit IosBuildStepFactory(QObject *parent = 0);

    bool canCreate(ProjectExplorer::BuildStepList *parent, const Core::Id id) const QTC_OVERRIDE;
    ProjectExplorer::BuildStep *create(ProjectExplorer::BuildStepList *parent, const Core::Id id) QTC_OVERRIDE;
    bool canClone(ProjectExplorer::BuildStepList *parent,
                  ProjectExplorer::BuildStep *source) const QTC_OVERRIDE;
    ProjectExplorer::BuildStep *clone(ProjectExplorer::BuildStepList *parent,
                                      ProjectExplorer::BuildStep *source) QTC_OVERRIDE;
    bool canRestore(ProjectExplorer::BuildStepList *parent, const QVariantMap &map) const QTC_OVERRIDE;
    ProjectExplorer::BuildStep *restore(ProjectExplorer::BuildStepList *parent,
                                        const QVariantMap &map) QTC_OVERRIDE;

    QList<Core::Id> availableCreationIds(ProjectExplorer::BuildStepList *bc) const QTC_OVERRIDE;
    QString displayNameForId(const Core::Id id) const QTC_OVERRIDE;
};

} // namespace Internal
} // namespace Ios

#endif // IOSBUILDSTEP_H
