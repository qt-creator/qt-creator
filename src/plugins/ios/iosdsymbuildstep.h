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
#ifndef IOSDSYMBUILDSTEP_H
#define IOSDSYMBUILDSTEP_H

#include <projectexplorer/abstractprocessstep.h>

namespace Ios {
namespace Internal {
namespace Ui { class IosPresetBuildStep; }

class IosPresetBuildStepConfigWidget;
class IosPresetBuildStepFactory;

class IosPresetBuildStep : public ProjectExplorer::AbstractProcessStep
{
    Q_OBJECT

    friend class IosPresetBuildStepConfigWidget;
    friend class IosPresetBuildStepFactory;

public:
    ~IosPresetBuildStep();

    bool init() Q_DECL_OVERRIDE;
    void run(QFutureInterface<bool> &fi) Q_DECL_OVERRIDE;

    ProjectExplorer::BuildStepConfigWidget *createConfigWidget() Q_DECL_OVERRIDE;
    bool immutable() const Q_DECL_OVERRIDE;
    void setArguments(const QStringList &args);
    QStringList arguments() const;
    QStringList defaultArguments() const;
    QString defaultCommand() const;
    QString command() const;
    void setCommand(const QString &command);
    bool clean() const;
    void setClean(bool clean);
    bool isDefault() const;

    QVariantMap toMap() const Q_DECL_OVERRIDE;
protected:
    IosPresetBuildStep(ProjectExplorer::BuildStepList *parent, Core::Id id);
    virtual bool completeSetup();
    virtual bool completeSetupWithStep(ProjectExplorer::BuildStep *bs);
    bool fromMap(const QVariantMap &map) Q_DECL_OVERRIDE;
    virtual QStringList defaultCleanCmdList() const = 0;
    virtual QStringList defaultCmdList() const = 0;
private:
    QStringList m_arguments;
    QString m_command;
    bool m_clean;
};

class IosPresetBuildStepConfigWidget : public ProjectExplorer::BuildStepConfigWidget
{
    Q_OBJECT

public:
    IosPresetBuildStepConfigWidget(IosPresetBuildStep *buildStep);
    ~IosPresetBuildStepConfigWidget();
    QString displayName() const Q_DECL_OVERRIDE;
    QString summaryText() const Q_DECL_OVERRIDE;

private slots:
    void commandChanged();
    void argumentsChanged();
    void resetDefaults();
    void updateDetails();

private:
    Ui::IosPresetBuildStep *m_ui;
    IosPresetBuildStep *m_buildStep;
    QString m_summaryText;
};

class IosPresetBuildStepFactory : public ProjectExplorer::IBuildStepFactory
{
    Q_OBJECT

public:
    explicit IosPresetBuildStepFactory(QObject *parent = 0);

    ProjectExplorer::BuildStep *create(ProjectExplorer::BuildStepList *parent, Core::Id id) Q_DECL_OVERRIDE;
    bool canClone(ProjectExplorer::BuildStepList *parent,
                  ProjectExplorer::BuildStep *source) const Q_DECL_OVERRIDE;
    ProjectExplorer::BuildStep *clone(ProjectExplorer::BuildStepList *parent,
                                      ProjectExplorer::BuildStep *source) Q_DECL_OVERRIDE;
    bool canRestore(ProjectExplorer::BuildStepList *parent, const QVariantMap &map) const Q_DECL_OVERRIDE;
    ProjectExplorer::BuildStep *restore(ProjectExplorer::BuildStepList *parent,
                                        const QVariantMap &map) Q_DECL_OVERRIDE;

protected:
    virtual IosPresetBuildStep *createPresetStep(ProjectExplorer::BuildStepList *parent,
                                                 const Core::Id id) const = 0;
};

class IosDsymBuildStep : public IosPresetBuildStep
{
    Q_OBJECT
    friend class IosDsymBuildStepFactory;
protected:
    QStringList defaultCleanCmdList() const Q_DECL_OVERRIDE;
    QStringList defaultCmdList() const Q_DECL_OVERRIDE;
    IosDsymBuildStep(ProjectExplorer::BuildStepList *parent, Core::Id id);
};

class IosDsymBuildStepFactory : public IosPresetBuildStepFactory
{
    Q_OBJECT
public:
    bool canCreate(ProjectExplorer::BuildStepList *parent, Core::Id id) const Q_DECL_OVERRIDE;
    QList<Core::Id> availableCreationIds(ProjectExplorer::BuildStepList *bc) const Q_DECL_OVERRIDE;
    QString displayNameForId(Core::Id id) const Q_DECL_OVERRIDE;
    IosPresetBuildStep *createPresetStep(ProjectExplorer::BuildStepList *parent,
                                         const Core::Id id) const Q_DECL_OVERRIDE;
};

} // namespace Internal
} // namespace Ios

#endif // IOSDSYMBUILDSTEP_H
