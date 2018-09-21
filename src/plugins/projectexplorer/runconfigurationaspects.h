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

#include "projectconfigurationaspects.h"
#include "applicationlauncher.h"
#include "environmentaspect.h"

QT_BEGIN_NAMESPACE
class QCheckBox;
class QToolButton;
QT_END_NAMESPACE

namespace ProjectExplorer {

class PROJECTEXPLORER_EXPORT TerminalAspect : public ProjectConfigurationAspect
{
    Q_OBJECT

public:
    TerminalAspect();

    void addToConfigurationLayout(QFormLayout *layout) override;

    bool useTerminal() const;
    void setUseTerminal(bool useTerminal);

    bool isUserSet() const;

private:
    void fromMap(const QVariantMap &map) override;
    void toMap(QVariantMap &map) const override;

    bool m_useTerminal = false;
    bool m_userSet = false;
    QPointer<QCheckBox> m_checkBox; // Owned by RunConfigWidget
};

class PROJECTEXPLORER_EXPORT WorkingDirectoryAspect : public ProjectConfigurationAspect
{
    Q_OBJECT

public:
    explicit WorkingDirectoryAspect(EnvironmentAspect *envAspect = nullptr);

    void addToConfigurationLayout(QFormLayout *layout) override;

    Utils::FileName workingDirectory(const Utils::MacroExpander *expander) const;
    Utils::FileName defaultWorkingDirectory() const;
    Utils::FileName unexpandedWorkingDirectory() const;
    void setDefaultWorkingDirectory(const Utils::FileName &defaultWorkingDir);
    Utils::PathChooser *pathChooser() const;

private:
    void fromMap(const QVariantMap &map) override;
    void toMap(QVariantMap &map) const override;

    void resetPath();
    QString keyForDefaultWd() const;

    EnvironmentAspect * const m_envAspect = nullptr;
    Utils::FileName m_workingDirectory;
    Utils::FileName m_defaultWorkingDirectory;
    QPointer<Utils::PathChooser> m_chooser;
    QPointer<QToolButton> m_resetButton;
};

class PROJECTEXPLORER_EXPORT ArgumentsAspect : public ProjectConfigurationAspect
{
    Q_OBJECT

public:
    ArgumentsAspect();

    void addToConfigurationLayout(QFormLayout *layout) override;

    QString arguments(const Utils::MacroExpander *expander) const;
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

class PROJECTEXPLORER_EXPORT UseLibraryPathsAspect : public BaseBoolAspect
{
    Q_OBJECT

public:
    UseLibraryPathsAspect();
};

class PROJECTEXPLORER_EXPORT UseDyldSuffixAspect : public BaseBoolAspect
{
    Q_OBJECT

public:
    UseDyldSuffixAspect();
};

class PROJECTEXPLORER_EXPORT ExecutableAspect : public ProjectConfigurationAspect
{
    Q_OBJECT

public:
    ExecutableAspect();
    ~ExecutableAspect() override;

    Utils::FileName executable() const;
    void setExecutable(const Utils::FileName &executable);

    void setSettingsKey(const QString &key);
    void makeOverridable(const QString &overridingKey, const QString &useOverridableKey);
    void addToConfigurationLayout(QFormLayout *layout) override;
    void setLabelText(const QString &labelText);
    void setPlaceHolderText(const QString &placeHolderText);
    void setExecutablePathStyle(Utils::OsType osType);
    void setHistoryCompleter(const QString &historyCompleterKey);
    void setExpectedKind(const Utils::PathChooser::Kind expectedKind);
    void setEnvironment(const Utils::Environment &env);
    void setDisplayStyle(BaseStringAspect::DisplayStyle style);

protected:
    void fromMap(const QVariantMap &map) override;
    void toMap(QVariantMap &map) const override;

private:
    QString executableText() const;

    BaseStringAspect m_executable;
    BaseStringAspect *m_alternativeExecutable = nullptr;
};

class PROJECTEXPLORER_EXPORT SymbolFileAspect : public BaseStringAspect
{
    Q_OBJECT

public:
     SymbolFileAspect() = default;
};

} // namespace ProjectExplorer
