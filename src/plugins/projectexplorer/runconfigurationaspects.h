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

#include "runconfiguration.h"
#include "applicationlauncher.h"

#include <utils/fileutils.h>

QT_BEGIN_NAMESPACE
class QCheckBox;
class QFormLayout;
class QLineEdit;
class QToolButton;
QT_END_NAMESPACE

namespace Utils {
class FancyLineEdit;
class PathChooser;
}

namespace ProjectExplorer {

class PROJECTEXPLORER_EXPORT TerminalAspect : public IRunConfigurationAspect
{
    Q_OBJECT

public:
    explicit TerminalAspect(RunConfiguration *rc, const QString &key,
                            bool useTerminal = false, bool userSet = false);

    TerminalAspect *create(RunConfiguration *runConfig) const override;
    TerminalAspect *clone(RunConfiguration *runConfig) const override;

    void addToMainConfigurationWidget(QWidget *parent, QFormLayout *layout);

    bool useTerminal() const;
    void setUseTerminal(bool useTerminal);

    ApplicationLauncher::Mode runMode() const;
    void setRunMode(ApplicationLauncher::Mode runMode);

    bool isUserSet() const;

signals:
    void useTerminalChanged(bool);

private:
    void fromMap(const QVariantMap &map) override;
    void toMap(QVariantMap &map) const override;

    bool m_useTerminal = false;
    bool m_userSet = false;
    QPointer<QCheckBox> m_checkBox; // Owned by RunConfigWidget
    QString m_key;
};

class PROJECTEXPLORER_EXPORT WorkingDirectoryAspect : public IRunConfigurationAspect
{
    Q_OBJECT

public:
    explicit WorkingDirectoryAspect(RunConfiguration *runConfig, const QString &key);

    WorkingDirectoryAspect *create(RunConfiguration *runConfig) const override;
    WorkingDirectoryAspect *clone(RunConfiguration *runConfig) const override;

    void addToMainConfigurationWidget(QWidget *parent, QFormLayout *layout);

    Utils::FileName workingDirectory() const;
    Utils::FileName defaultWorkingDirectory() const;
    Utils::FileName unexpandedWorkingDirectory() const;
    void setDefaultWorkingDirectory(const Utils::FileName &defaultWorkingDir);
    Utils::PathChooser *pathChooser() const;

private:
    void fromMap(const QVariantMap &map) override;
    void toMap(QVariantMap &map) const override;

    void resetPath();
    QString keyForDefaultWd() const;

    Utils::FileName m_workingDirectory;
    Utils::FileName m_defaultWorkingDirectory;
    QPointer<Utils::PathChooser> m_chooser;
    QPointer<QToolButton> m_resetButton;
    QString m_key;
};

class PROJECTEXPLORER_EXPORT ArgumentsAspect : public IRunConfigurationAspect
{
    Q_OBJECT

public:
    explicit ArgumentsAspect(RunConfiguration *runConfig, const QString &key, const QString &arguments = QString());

    ArgumentsAspect *create(RunConfiguration *runConfig) const override;
    ArgumentsAspect *clone(RunConfiguration *runConfig) const override;

    void addToMainConfigurationWidget(QWidget *parent, QFormLayout *layout);

    QString arguments() const;
    QString unexpandedArguments() const;

    void setArguments(const QString &arguments);

signals:
    void argumentsChanged(const QString &arguments);

private:
    void fromMap(const QVariantMap &map) override;
    void toMap(QVariantMap &map) const override;

    QString m_arguments;
    QPointer<Utils::FancyLineEdit> m_chooser;
    QString m_key;
};

} // namespace ProjectExplorer
