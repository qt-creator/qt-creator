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

#include "abstractmsvctoolchain.h"
#include "abi.h"
#include "toolchainconfigwidget.h"

QT_FORWARD_DECLARE_CLASS(QLabel)
QT_FORWARD_DECLARE_CLASS(QVersionNumber)

namespace ProjectExplorer {
namespace Internal {

// --------------------------------------------------------------------------
// MsvcToolChain
// --------------------------------------------------------------------------

class MsvcToolChain : public AbstractMsvcToolChain
{
public:
    enum Type { WindowsSDK, VS };
    enum Platform { x86,
                    amd64,
                    x86_amd64,
                    ia64,
                    x86_ia64,
                    arm,
                    x86_arm,
                    amd64_arm,
                    amd64_x86
                  };

    explicit MsvcToolChain(const QString &name, const Abi &abi,
                           const QString &varsBat, const QString &varsBatArg,
                           Core::Id l, Detection d = ManualDetection);
    MsvcToolChain();

    Utils::FileNameList suggestedMkspecList() const override;

    QString typeDisplayName() const override;

    QVariantMap toMap() const override;
    bool fromMap(const QVariantMap &data) override;

    ToolChainConfigWidget *configurationWidget() override;

    ToolChain *clone() const override;

    QString varsBatArg() const { return m_varsBatArg; }

    bool operator == (const ToolChain &) const override;

protected:
    explicit MsvcToolChain(Core::Id typeId, const QString &name, const Abi &abi,
                           const QString &varsBat, const QString &varsBatArg,
                           Core::Id l, Detection d);
    explicit MsvcToolChain(Core::Id typeId);

    Utils::Environment readEnvironmentSetting(const Utils::Environment& env) const final;
    QByteArray msvcPredefinedMacros(const QStringList cxxflags,
                                    const Utils::Environment &env) const override;

private:
    QString m_varsBatArg; // Argument
};

class ClangClToolChain : public MsvcToolChain
{
public:
    explicit ClangClToolChain(const QString &name, const QString &llvmDir,
                              const Abi &abi,
                              const QString &varsBat, const QString &varsBatArg,
                              Core::Id language,
                              Detection d = ManualDetection);
    ClangClToolChain();

    bool isValid() const override;
    QString typeDisplayName() const override;
    QList<Utils::FileName> suggestedMkspecList() const override;
    void addToEnvironment(Utils::Environment &env) const override;
    Utils::FileName compilerCommand() const override { return m_compiler; }
    IOutputParser *outputParser() const override;
    ToolChain *clone() const override;
    QVariantMap toMap() const override;
    bool fromMap(const QVariantMap &data) override;
    ToolChainConfigWidget *configurationWidget() override;

    QString llvmDir() const { return m_llvmDir; }

private:
    QString m_llvmDir;
    Utils::FileName m_compiler;
};

// --------------------------------------------------------------------------
// MsvcToolChainFactory
// --------------------------------------------------------------------------

class MsvcToolChainFactory : public ToolChainFactory
{
    Q_OBJECT

public:
    MsvcToolChainFactory();
    QSet<Core::Id> supportedLanguages() const override;

    QList<ToolChain *> autoDetect(const QList<ToolChain *> &alreadyKnown) override;

    bool canRestore(const QVariantMap &data) override;
    ToolChain *restore(const QVariantMap &data) override;

    ToolChainConfigWidget *configurationWidget(ToolChain *);
    static QString vcVarsBatFor(const QString &basePath, MsvcToolChain::Platform platform,
                                const QVersionNumber &v);
};

// --------------------------------------------------------------------------
// MsvcBasedToolChainConfigWidget
// --------------------------------------------------------------------------

class MsvcBasedToolChainConfigWidget : public ToolChainConfigWidget
{
    Q_OBJECT

public:
    explicit MsvcBasedToolChainConfigWidget(ToolChain *);

protected:
    void applyImpl() override { }
    void discardImpl() override { setFromMsvcToolChain(); }
    bool isDirtyImpl() const override { return false; }
    void makeReadOnlyImpl() override { }

    void setFromMsvcToolChain();

private:
    QLabel *m_nameDisplayLabel;
    QLabel *m_varsBatDisplayLabel;
};

// --------------------------------------------------------------------------
// MsvcToolChainConfigWidget
// --------------------------------------------------------------------------

class MsvcToolChainConfigWidget : public MsvcBasedToolChainConfigWidget
{
    Q_OBJECT

public:
    explicit MsvcToolChainConfigWidget(ToolChain *);
};

// --------------------------------------------------------------------------
// ClangClToolChainConfigWidget
// --------------------------------------------------------------------------

class ClangClToolChainConfigWidget : public MsvcBasedToolChainConfigWidget
{
    Q_OBJECT

public:
    explicit ClangClToolChainConfigWidget(ToolChain *);

protected:
    void discardImpl() override { setFromClangClToolChain(); }

private:
    void setFromClangClToolChain();

    QLabel *m_llvmDirLabel;
};

} // namespace Internal
} // namespace ProjectExplorer
