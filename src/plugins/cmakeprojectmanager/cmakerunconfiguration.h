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

#ifndef CMAKERUNCONFIGURATION_H
#define CMAKERUNCONFIGURATION_H

#include <projectexplorer/localapplicationrunconfiguration.h>
#include <utils/environment.h>

QT_BEGIN_NAMESPACE
class QComboBox;
QT_END_NAMESPACE

namespace Utils {
class PathChooser;
class DetailsWidget;
}

namespace CMakeProjectManager {
namespace Internal {

class CMakeTarget;

class CMakeRunConfiguration : public ProjectExplorer::LocalApplicationRunConfiguration
{
    Q_OBJECT
    friend class CMakeRunConfigurationWidget;
    friend class CMakeRunConfigurationFactory;

public:
    CMakeRunConfiguration(ProjectExplorer::Target *parent, Core::Id id, const QString &target,
                          const QString &workingDirectory, const QString &title);

    QString executable() const override;
    ProjectExplorer::ApplicationLauncher::Mode runMode() const override;
    QString workingDirectory() const override;
    QString commandLineArguments() const override;
    QWidget *createConfigurationWidget() override;

    void setExecutable(const QString &executable);
    void setBaseWorkingDirectory(const QString &workingDirectory);

    QString title() const;

    QVariantMap toMap() const override;

    void setEnabled(bool b);

    bool isEnabled() const override;
    QString disabledReason() const override;

protected:
    CMakeRunConfiguration(ProjectExplorer::Target *parent, CMakeRunConfiguration *source);
    bool fromMap(const QVariantMap &map) override;
    QString defaultDisplayName() const;

private:
    QString baseWorkingDirectory() const;
    void ctor();

    QString m_buildTarget;
    QString m_title;
    bool m_enabled = true;
};

class CMakeRunConfigurationWidget : public QWidget
{
    Q_OBJECT

public:
    explicit CMakeRunConfigurationWidget(CMakeRunConfiguration *cmakeRunConfiguration, QWidget *parent = 0);
};

class CMakeRunConfigurationFactory : public ProjectExplorer::IRunConfigurationFactory
{
    Q_OBJECT

public:
    explicit CMakeRunConfigurationFactory(QObject *parent = 0);

    bool canCreate(ProjectExplorer::Target *parent, Core::Id id) const override;
    bool canRestore(ProjectExplorer::Target *parent, const QVariantMap &map) const override;
    bool canClone(ProjectExplorer::Target *parent, ProjectExplorer::RunConfiguration *product) const override;
    ProjectExplorer::RunConfiguration *clone(ProjectExplorer::Target *parent,
                                             ProjectExplorer::RunConfiguration *product) override;

    QList<Core::Id> availableCreationIds(ProjectExplorer::Target *parent, CreationMode mode) const override;
    QString displayNameForId(Core::Id id) const override;

    static Core::Id idFromBuildTarget(const QString &target);
    static QString buildTargetFromId(Core::Id id);

private:
    bool canHandle(ProjectExplorer::Target *parent) const;

    ProjectExplorer::RunConfiguration *doCreate(ProjectExplorer::Target *parent, Core::Id id) override;
    ProjectExplorer::RunConfiguration *doRestore(ProjectExplorer::Target *parent,
                                                 const QVariantMap &map) override;
};

} // namespace Internal
} // namespace CMakeProjectManager

#endif // CMAKERUNCONFIGURATION_H
