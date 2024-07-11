// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "projectexplorer_export.h"

#include "toolchain.h"

#include <QScrollArea>

#include <utility>

namespace Utils { class PathChooser; }

QT_BEGIN_NAMESPACE
class QCheckBox;
class QFormLayout;
class QLineEdit;
class QLabel;
QT_END_NAMESPACE

namespace ProjectExplorer {

// --------------------------------------------------------------------------
// ToolChainConfigWidget
// --------------------------------------------------------------------------

class PROJECTEXPLORER_EXPORT ToolchainConfigWidget : public QScrollArea
{
    Q_OBJECT

public:
    explicit ToolchainConfigWidget(const ToolchainBundle &bundle);

    ToolchainBundle bundle() const { return m_bundle; }

    void apply();
    void discard();
    bool isDirty() const;
    void makeReadOnly();

signals:
    void compilerCommandChanged(Utils::Id language);
    void dirty();

protected:
    void setErrorMessage(const QString &);
    void clearErrorMessage();

    virtual void applyImpl() = 0;
    virtual void discardImpl() = 0;
    virtual bool isDirtyImpl() const = 0;
    virtual void makeReadOnlyImpl() = 0;

    void addErrorLabel();
    static QStringList splitString(const QString &s);
    Utils::FilePath compilerCommand(Utils::Id language);
    bool hasAnyCompiler() const;
    void setCommandVersionArguments(const QStringList &args);
    void deriveCxxCompilerCommand();

    QFormLayout *m_mainLayout;

private:
    using ToolchainChooser = std::pair<const Toolchain *, Utils::PathChooser *>;
    ToolchainChooser compilerPathChooser(Utils::Id language);
    void setupCompilerPathChoosers();

    ToolchainBundle m_bundle;
    QLineEdit *m_nameLineEdit = nullptr;
    QLabel *m_errorLabel = nullptr;
    QCheckBox *m_deriveCxxCompilerCheckBox = nullptr;
    QList<ToolchainChooser> m_commands;
};

} // namespace ProjectExplorer
