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

#ifndef RUNCONFIGURATION_ASPECTS_H
#define RUNCONFIGURATION_ASPECTS_H

#include "runconfiguration.h"
#include "applicationlauncher.h"

QT_BEGIN_NAMESPACE
class QCheckBox;
class QFormLayout;
class QLineEdit;
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
    explicit TerminalAspect(RunConfiguration *rc, const QString &key, bool useTerminal = false, bool userSet = false);

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

    bool m_useTerminal;
    bool m_userSet;
    QPointer<QCheckBox> m_checkBox; // Owned by RunConfigWidget
    QString m_key;
};

class PROJECTEXPLORER_EXPORT WorkingDirectoryAspect : public IRunConfigurationAspect
{
    Q_OBJECT

public:
    explicit WorkingDirectoryAspect(RunConfiguration *runConfig, const QString &key, const QString &dir = QString());

    WorkingDirectoryAspect *create(RunConfiguration *runConfig) const override;
    WorkingDirectoryAspect *clone(RunConfiguration *runConfig) const override;

    void addToMainConfigurationWidget(QWidget *parent, QFormLayout *layout);

    QString workingDirectory() const;
    QString unexpandedWorkingDirectory() const;
    void setWorkingDirectory(const QString &workingDirectory);
    Utils::PathChooser *pathChooser() const;

private:
    void fromMap(const QVariantMap &map) override;
    void toMap(QVariantMap &map) const override;

    void resetPath();
    void updateEnvironment();

    QString m_workingDirectory;
    QPointer<Utils::PathChooser> m_chooser;
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

#endif // RUNCONFIGURATION_ASPECTS_H
