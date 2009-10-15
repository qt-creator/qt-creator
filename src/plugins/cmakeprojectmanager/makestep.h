/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef MAKESTEP_H
#define MAKESTEP_H

#include <projectexplorer/abstractmakestep.h>

QT_BEGIN_NAMESPACE
class QLineEdit;
class QListWidget;
class QListWidgetItem;
QT_END_NAMESPACE

namespace CMakeProjectManager {
namespace Internal {

class CMakeProject;

struct MakeStepSettings
{
    QStringList buildTargets;
    QStringList additionalArguments;
};

class MakeStep : public ProjectExplorer::AbstractMakeStep
{
    Q_OBJECT
    friend class MakeStepConfigWidget; // TODO remove
    // This is for modifying m_values
public:
    MakeStep(CMakeProject *pro);
    ~MakeStep();
    virtual bool init(const QString &buildConfiguration);

    virtual void run(QFutureInterface<bool> &fi);

    virtual QString name();
    virtual QString displayName();
    virtual ProjectExplorer::BuildStepConfigWidget *createConfigWidget();
    virtual bool immutable() const;
    CMakeProject *project() const;
    bool buildsTarget(const QString &buildConfiguration, const QString &target) const;
    void setBuildTarget(const QString &buildConfiguration, const QString &target, bool on);
    QStringList additionalArguments(const QString &buildConfiguration) const;
    void setAdditionalArguments(const QString &buildConfiguration, const QStringList &list);

    virtual void restoreFromMap(const QMap<QString, QVariant> &map);
    virtual void storeIntoMap(QMap<QString, QVariant> &map);

    void setClean(bool clean);

    virtual void restoreFromMap(const QString &buildConfiguration, const QMap<QString, QVariant> &map);
    virtual void storeIntoMap(const QString &buildConfiguration, QMap<QString, QVariant> &map);

    virtual void addBuildConfiguration(const QString & name);
    virtual void removeBuildConfiguration(const QString & name);
    virtual void copyBuildConfiguration(const QString &source, const QString &dest);
protected:
    // For parsing [ 76%]
    virtual void stdOut(const QString &line);
private:
    CMakeProject *m_pro;
    bool m_clean;
    QRegExp m_percentProgress;
    QFutureInterface<bool> *m_futureInterface;
    QMap<QString, MakeStepSettings> m_values;
};

class MakeStepConfigWidget :public ProjectExplorer::BuildStepConfigWidget
{
    Q_OBJECT
public:
    MakeStepConfigWidget(MakeStep *makeStep);
    virtual QString displayName() const;
    virtual void init(const QString &buildConfiguration);
    virtual QString summaryText() const;
private slots:
    void itemChanged(QListWidgetItem*);
    void additionalArgumentsEdited();
    void updateDetails();
private:
    QString m_buildConfiguration;
    MakeStep *m_makeStep;
    QListWidget *m_targetsList;
    QLineEdit *m_additionalArguments;
    QString m_summaryText;
};

class MakeStepFactory : public ProjectExplorer::IBuildStepFactory
{
    virtual bool canCreate(const QString &name) const;
    virtual ProjectExplorer::BuildStep *create(ProjectExplorer::Project *pro, const QString &name) const;
    virtual QStringList canCreateForProject(ProjectExplorer::Project *pro) const;
    virtual QString displayNameForName(const QString &name) const;
};

} // namespace Internal
} // namespace CMakeProjectManager

#endif // MAKESTEP_H
