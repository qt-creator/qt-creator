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

#include "toolchain.h"
#include "toolchainconfigwidget.h"
#include "abi.h"
#include "abiwidget.h"

#include <QList>
#include <QSet>

#include <functional>

QT_BEGIN_NAMESPACE
class QComboBox;
QT_END_NAMESPACE

namespace Utils { class PathChooser; }

namespace ProjectExplorer {
class ClangToolChain;
class GccToolChain;

namespace Internal {

class GccToolChainFactory : public ToolChainFactory
{
public:
    GccToolChainFactory();

    QList<ToolChain *> autoDetect(const QList<ToolChain *> &alreadyKnown) override;
    QList<ToolChain *> detectForImport(const ProjectExplorer::ToolChainDescription &tcd) override;

protected:
    enum class DetectVariants { Yes, No };
    using ToolchainChecker = std::function<bool(const ToolChain *)>;
    QList<ToolChain *> autoDetectToolchains(
            const QString &compilerName, DetectVariants detectVariants, Utils::Id language,
            const Utils::Id requiredTypeId, const QList<ToolChain *> &alreadyKnown,
            const ToolchainChecker &checker = {});
    QList<ToolChain *> autoDetectToolChain(
            const ToolChainDescription &tcd,
            const ToolchainChecker &checker = {});
};

// --------------------------------------------------------------------------
// GccToolChainConfigWidget
// --------------------------------------------------------------------------

class GccToolChainConfigWidget : public ToolChainConfigWidget
{
    Q_OBJECT

public:
    explicit GccToolChainConfigWidget(GccToolChain *tc);

protected:
    void handleCompilerCommandChange();
    void handlePlatformCodeGenFlagsChange();
    void handlePlatformLinkerFlagsChange();

    void applyImpl() override;
    void discardImpl() override { setFromToolchain(); }
    bool isDirtyImpl() const override;
    void makeReadOnlyImpl() override;

    void setFromToolchain();

    AbiWidget *m_abiWidget;

private:
    Utils::PathChooser *m_compilerCommand;
    QLineEdit *m_platformCodeGenFlagsLineEdit;
    QLineEdit *m_platformLinkerFlagsLineEdit;

    bool m_isReadOnly = false;
    ProjectExplorer::Macros m_macros;
};

// --------------------------------------------------------------------------
// ClangToolChainConfigWidget
// --------------------------------------------------------------------------

class ClangToolChainConfigWidget : public GccToolChainConfigWidget
{
    Q_OBJECT
public:
    explicit ClangToolChainConfigWidget(ClangToolChain *tc);

private:
    void applyImpl() override;
    void discardImpl() override { setFromClangToolchain(); }
    bool isDirtyImpl() const override;
    void makeReadOnlyImpl() override;

    void setFromClangToolchain();
    void updateParentToolChainComboBox();
    QList<QMetaObject::Connection> m_parentToolChainConnections;
    QComboBox *m_parentToolchainCombo = nullptr;
};

// --------------------------------------------------------------------------
// ClangToolChainFactory
// --------------------------------------------------------------------------

class ClangToolChainFactory : public GccToolChainFactory
{
public:
    ClangToolChainFactory();

    QList<ToolChain *> autoDetect(const QList<ToolChain *> &alreadyKnown) override;
    QList<ToolChain *> detectForImport(const ToolChainDescription &tcd) final;
};

// --------------------------------------------------------------------------
// MingwToolChainFactory
// --------------------------------------------------------------------------

class MingwToolChainFactory : public GccToolChainFactory
{
public:
    MingwToolChainFactory();

    QList<ToolChain *> autoDetect(const QList<ToolChain *> &alreadyKnown) override;
    QList<ToolChain *> detectForImport(const ToolChainDescription &tcd) final;
};

// --------------------------------------------------------------------------
// LinuxIccToolChainFactory
// --------------------------------------------------------------------------

class LinuxIccToolChainFactory : public GccToolChainFactory
{
public:
    LinuxIccToolChainFactory();

    QList<ToolChain *> autoDetect(const QList<ToolChain *> &alreadyKnown) override;
    QList<ToolChain *> detectForImport(const ToolChainDescription &tcd) final;
};

} // namespace Internal
} // namespace ProjectExplorer
