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
class QLabel;
class QFormLayout;
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
    TerminalAspect(RunConfiguration *rc, const QString &settingsKey,
                   bool useTerminal = false);

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
};

class PROJECTEXPLORER_EXPORT WorkingDirectoryAspect : public IRunConfigurationAspect
{
    Q_OBJECT

public:
    explicit WorkingDirectoryAspect(RunConfiguration *runConfig, const QString &settingsKey);

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
};

class PROJECTEXPLORER_EXPORT ArgumentsAspect : public IRunConfigurationAspect
{
    Q_OBJECT

public:
    explicit ArgumentsAspect(RunConfiguration *runConfig, const QString &settingsKey);

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
};

class PROJECTEXPLORER_EXPORT ExecutableAspect : public IRunConfigurationAspect
{
    Q_OBJECT

public:
    ExecutableAspect(RunConfiguration *runConfig,
                     bool isRemote = false,
                     const QString &label = QString());

    void addToMainConfigurationWidget(QWidget *parent, QFormLayout *layout);

    Utils::FileName executable() const;
    void setExecutable(const Utils::FileName &executable);

private:
    QString executableText() const;

    Utils::FileName m_executable;
    bool m_isRemote = false;
    QString m_labelString;
    QPointer<QLabel> m_executableDisplay;
};

class PROJECTEXPLORER_EXPORT BaseBoolAspect : public IRunConfigurationAspect
{
    Q_OBJECT

public:
    BaseBoolAspect(RunConfiguration *rc, const QString &settingsKey);

    void addToMainConfigurationWidget(QWidget *parent, QFormLayout *layout);
    bool value() const;
    void setValue(bool val);

    void setLabel(const QString &label);

signals:
    void changed();

private:
    void fromMap(const QVariantMap &map) override;
    void toMap(QVariantMap &map) const override;

    bool m_value = false;
    QString m_label;
    QPointer<QCheckBox> m_checkBox; // Owned by RunConfigWidget
};

class PROJECTEXPLORER_EXPORT UseLibraryPathsAspect : public BaseBoolAspect
{
    Q_OBJECT

public:
    UseLibraryPathsAspect(RunConfiguration *rc, const QString &settingsKey);
};

class PROJECTEXPLORER_EXPORT UseDyldSuffixAspect : public BaseBoolAspect
{
    Q_OBJECT

public:
    UseDyldSuffixAspect(RunConfiguration *rc, const QString &settingsKey);
};

} // namespace ProjectExplorer
