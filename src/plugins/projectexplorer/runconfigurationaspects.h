// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "environmentaspect.h"

#include <utils/aspects.h>

#include <QPointer>

QT_BEGIN_NAMESPACE
class QCheckBox;
class QComboBox;
class QPlainTextEdit;
class QToolButton;
QT_END_NAMESPACE

namespace Utils { class ExpandButton; }

namespace ProjectExplorer {

class ProjectConfiguration;

class PROJECTEXPLORER_EXPORT TerminalAspect : public Utils::BaseAspect
{
    Q_OBJECT

public:
    explicit TerminalAspect(Utils::AspectContainer *container = nullptr);

    void addToLayout(Layouting::LayoutItem &parent) override;

    bool useTerminal() const;
    void setUseTerminalHint(bool useTerminal);

    bool isUserSet() const;

    struct Data : BaseAspect::Data
    {
        bool useTerminal;
        bool isUserSet;
    };

private:
    void fromMap(const Utils::Store &map) override;
    void toMap(Utils::Store &map) const override;

    void calculateUseTerminal();

    bool m_useTerminalHint = false;
    bool m_useTerminal = false;
    bool m_userSet = false;
    QPointer<QCheckBox> m_checkBox; // Owned by RunConfigWidget
};

class PROJECTEXPLORER_EXPORT WorkingDirectoryAspect : public Utils::BaseAspect
{
    Q_OBJECT

public:
    explicit WorkingDirectoryAspect(Utils::AspectContainer *container = nullptr);

    void addToLayout(Layouting::LayoutItem &parent) override;

    Utils::FilePath operator()() const { return workingDirectory(); }
    Utils::FilePath workingDirectory() const;
    Utils::FilePath defaultWorkingDirectory() const;
    Utils::FilePath unexpandedWorkingDirectory() const;
    void setDefaultWorkingDirectory(const Utils::FilePath &defaultWorkingDirectory);
    Utils::PathChooser *pathChooser() const;
    void setMacroExpander(const Utils::MacroExpander *expander);
    void setEnvironment(EnvironmentAspect *envAspect);

private:
    void fromMap(const Utils::Store &map) override;
    void toMap(Utils::Store &map) const override;

    void resetPath();

    EnvironmentAspect *m_envAspect = nullptr;
    Utils::FilePath m_workingDirectory;
    Utils::FilePath m_defaultWorkingDirectory;
    QPointer<Utils::PathChooser> m_chooser;
    QPointer<QToolButton> m_resetButton;
    const Utils::MacroExpander *m_macroExpander = nullptr;
};

class PROJECTEXPLORER_EXPORT ArgumentsAspect : public Utils::BaseAspect
{
    Q_OBJECT

public:
    explicit ArgumentsAspect(Utils::AspectContainer *container = nullptr);

    void addToLayout(Layouting::LayoutItem &parent) override;

    QString operator()() const { return arguments(); }
    QString arguments() const;
    QString unexpandedArguments() const;

    void setArguments(const QString &arguments);
    void setLabelText(const QString &labelText);
    void setResetter(const std::function<QString()> &resetter);
    void resetArguments();
    void setMacroExpander(const Utils::MacroExpander *macroExpander);

    struct Data : BaseAspect::Data
    {
        QString arguments;
    };

private:
    void fromMap(const Utils::Store &map) override;
    void toMap(Utils::Store &map) const override;

    QWidget *setupChooser();

    QString m_arguments;
    QString m_labelText;
    QPointer<Utils::FancyLineEdit> m_chooser;
    QPointer<QPlainTextEdit> m_multiLineChooser;
    QPointer<Utils::ExpandButton> m_multiLineButton;
    QPointer<QToolButton> m_resetButton;
    bool m_multiLine = false;
    mutable bool m_currentlyExpanding = false;
    std::function<QString()> m_resetter;
    const Utils::MacroExpander *m_macroExpander = nullptr;
};

class PROJECTEXPLORER_EXPORT UseLibraryPathsAspect : public Utils::BoolAspect
{
    Q_OBJECT

public:
    UseLibraryPathsAspect(Utils::AspectContainer *container = nullptr);
};

class PROJECTEXPLORER_EXPORT UseDyldSuffixAspect : public Utils::BoolAspect
{
    Q_OBJECT

public:
    UseDyldSuffixAspect(Utils::AspectContainer *container = nullptr);
};

class PROJECTEXPLORER_EXPORT RunAsRootAspect : public Utils::BoolAspect
{
    Q_OBJECT

public:
    RunAsRootAspect(Utils::AspectContainer *container = nullptr);
};

class PROJECTEXPLORER_EXPORT ExecutableAspect : public Utils::BaseAspect
{
    Q_OBJECT

public:
    enum ExecutionDeviceSelector { HostDevice, BuildDevice, RunDevice };

    explicit ExecutableAspect(Utils::AspectContainer *container = nullptr);
    ~ExecutableAspect() override;

    Utils::FilePath operator()() const { return executable(); }
    Utils::FilePath executable() const;
    void setExecutable(const Utils::FilePath &executable);

    void setDeviceSelector(Target *target, ExecutionDeviceSelector selector);
    void setSettingsKey(const Utils::Key &key);
    void makeOverridable(const Utils::Key &overridingKey, const Utils::Key &useOverridableKey);
    void addToLayout(Layouting::LayoutItem &parent) override;
    void setLabelText(const QString &labelText);
    void setPlaceHolderText(const QString &placeHolderText);
    void setHistoryCompleter(const Utils::Key &historyCompleterKey);
    void setExpectedKind(const Utils::PathChooser::Kind expectedKind);
    void setEnvironment(const Utils::Environment &env);
    void setReadOnly(bool readOnly);

    struct Data : BaseAspect::Data
    {
        Utils::FilePath executable;
    };

protected:
    void fromMap(const Utils::Store &map) override;
    void toMap(Utils::Store &map) const override;

private:
    QString executableText() const;

    Utils::FilePathAspect m_executable;
    Utils::FilePathAspect *m_alternativeExecutable = nullptr;
    Target *m_target = nullptr;
    ExecutionDeviceSelector m_selector = RunDevice;
};

class PROJECTEXPLORER_EXPORT SymbolFileAspect : public Utils::FilePathAspect
{
    Q_OBJECT

public:
     SymbolFileAspect(Utils::AspectContainer *container = nullptr);
};

class PROJECTEXPLORER_EXPORT Interpreter
{
public:
    Interpreter();
    Interpreter(const QString &id,
                const QString &name,
                const Utils::FilePath &command,
                bool autoDetected = true);

    inline bool operator==(const Interpreter &other) const
    {
        return id == other.id && name == other.name && command == other.command
               && detectionSource == other.detectionSource;
    }

    QString id;
    QString name;
    Utils::FilePath command;
    bool autoDetected = true;
    QString detectionSource;
};

class PROJECTEXPLORER_EXPORT InterpreterAspect : public Utils::BaseAspect
{
    Q_OBJECT

public:
    InterpreterAspect(Utils::AspectContainer *container = nullptr);

    Interpreter currentInterpreter() const;
    void updateInterpreters(const QList<Interpreter> &interpreters);
    void setDefaultInterpreter(const Interpreter &interpreter);
    void setCurrentInterpreter(const Interpreter &interpreter);
    void setSettingsDialogId(Utils::Id id) { m_settingsDialogId = id; }

    void fromMap(const Utils::Store &) override;
    void toMap(Utils::Store &) const override;
    void addToLayout(Layouting::LayoutItem &parent) override;

    struct Data : Utils::BaseAspect::Data { Interpreter interpreter; };

private:
    void setCurrentInterpreterId(const QString &id);
    void updateCurrentInterpreter();
    void updateComboBox();
    QList<Interpreter> m_interpreters;
    QPointer<QComboBox> m_comboBox;
    QString m_defaultId;
    QString m_currentId;
    Utils::Id m_settingsDialogId;
};

class PROJECTEXPLORER_EXPORT MainScriptAspect : public Utils::FilePathAspect
{
    Q_OBJECT

public:
    MainScriptAspect(Utils::AspectContainer *container = nullptr);
};

class PROJECTEXPLORER_EXPORT X11ForwardingAspect : public Utils::StringAspect
{
    Q_OBJECT

public:
    X11ForwardingAspect(Utils::AspectContainer *container = nullptr);

    void setMacroExpander(const Utils::MacroExpander *macroExpander);

    struct Data : StringAspect::Data { QString display; };

    QString display() const;

private:
    const Utils::MacroExpander *m_macroExpander;
};

} // namespace ProjectExplorer
