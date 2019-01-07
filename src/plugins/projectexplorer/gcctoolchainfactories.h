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
    Q_OBJECT

public:
    GccToolChainFactory();
    QSet<Core::Id> supportedLanguages() const override;

    QList<ToolChain *> autoDetect(const QList<ToolChain *> &alreadyKnown) override;
    QList<ToolChain *> autoDetect(const Utils::FileName &compilerPath, const Core::Id &language) override;

    bool canCreate() override;
    ToolChain *create(Core::Id language) override;

    bool canRestore(const QVariantMap &data) override;
    ToolChain *restore(const QVariantMap &data) override;

protected:
    virtual GccToolChain *createToolChain(bool autoDetect);
    void versionProbe(const QString &name,
                      Core::Id language,
                      Core::Id type,
                      QList<ToolChain *> &tcs,
                      QList<ToolChain *> &known,
                      const QSet<QString> &filteredNames = {});

    Utils::FileName compilerPathFromEnvironment(const QString &compilerName);

    QList<ToolChain *> autoDetectToolchains(
            const Utils::FileName &compilerPath, const Abi &requiredAbi, Core::Id language,
            const Core::Id requiredTypeId, const QList<ToolChain *> &alreadyKnown);
    QList<ToolChain *> autoDetectToolChain(const Utils::FileName &compilerPath, const Core::Id language,
                                           const Abi &requiredAbi = Abi());
};

// --------------------------------------------------------------------------
// GccToolChainConfigWidget
// --------------------------------------------------------------------------

class GccToolChainConfigWidget : public ToolChainConfigWidget
{
    Q_OBJECT

public:
    explicit GccToolChainConfigWidget(GccToolChain *tc);
    static QStringList splitString(const QString &s);

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
    Q_OBJECT

public:
    ClangToolChainFactory();
    QSet<Core::Id> supportedLanguages() const override;

    QList<ToolChain *> autoDetect(const QList<ToolChain *> &alreadyKnown) override;
    QList<ToolChain *> autoDetect(const Utils::FileName &compilerPath, const Core::Id &language) final;

    bool canRestore(const QVariantMap &data) override;

protected:
    GccToolChain *createToolChain(bool autoDetect) override;
};

// --------------------------------------------------------------------------
// MingwToolChainFactory
// --------------------------------------------------------------------------

class MingwToolChainFactory : public GccToolChainFactory
{
    Q_OBJECT

public:
    MingwToolChainFactory();
    QSet<Core::Id> supportedLanguages() const override;

    QList<ToolChain *> autoDetect(const QList<ToolChain *> &alreadyKnown) override;
    QList<ToolChain *> autoDetect(const Utils::FileName &compilerPath, const Core::Id &language) final;

    bool canRestore(const QVariantMap &data) override;

protected:
    GccToolChain *createToolChain(bool autoDetect) override;
};

// --------------------------------------------------------------------------
// LinuxIccToolChainFactory
// --------------------------------------------------------------------------

class LinuxIccToolChainFactory : public GccToolChainFactory
{
    Q_OBJECT

public:
    LinuxIccToolChainFactory();
    QSet<Core::Id> supportedLanguages() const override;

    QList<ToolChain *> autoDetect(const QList<ToolChain *> &alreadyKnown) override;
    QList<ToolChain *> autoDetect(const Utils::FileName &compilerPath, const Core::Id &language) final;

    bool canRestore(const QVariantMap &data) override;

protected:
    GccToolChain *createToolChain(bool autoDetect) override;
};

} // namespace Internal
} // namespace ProjectExplorer
