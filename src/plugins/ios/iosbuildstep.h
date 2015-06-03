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
#ifndef IOSBUILDSTEP_H
#define IOSBUILDSTEP_H

#include <projectexplorer/abstractprocessstep.h>

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

    bool init() override;
    void run(QFutureInterface<bool> &fi) override;

    ProjectExplorer::BuildStepConfigWidget *createConfigWidget() override;
    bool immutable() const override;
    void setBaseArguments(const QStringList &args);
    void setExtraArguments(const QStringList &extraArgs);
    QStringList baseArguments() const;
    QStringList allArguments() const;
    QStringList defaultArguments() const;
    QString buildCommand() const;

    void setClean(bool clean);
    bool isClean() const;

    QVariantMap toMap() const override;
protected:
    IosBuildStep(ProjectExplorer::BuildStepList *parent, IosBuildStep *bs);
    IosBuildStep(ProjectExplorer::BuildStepList *parent, Core::Id id);
    bool fromMap(const QVariantMap &map) override;

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
    QString displayName() const override;
    QString summaryText() const override;

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

    bool canCreate(ProjectExplorer::BuildStepList *parent, Core::Id id) const override;
    ProjectExplorer::BuildStep *create(ProjectExplorer::BuildStepList *parent, Core::Id id) override;
    bool canClone(ProjectExplorer::BuildStepList *parent,
                  ProjectExplorer::BuildStep *source) const override;
    ProjectExplorer::BuildStep *clone(ProjectExplorer::BuildStepList *parent,
                                      ProjectExplorer::BuildStep *source) override;
    bool canRestore(ProjectExplorer::BuildStepList *parent, const QVariantMap &map) const override;
    ProjectExplorer::BuildStep *restore(ProjectExplorer::BuildStepList *parent,
                                        const QVariantMap &map) override;

    QList<Core::Id> availableCreationIds(ProjectExplorer::BuildStepList *bc) const override;
    QString displayNameForId(Core::Id id) const override;
};

} // namespace Internal
} // namespace Ios

#endif // IOSBUILDSTEP_H
