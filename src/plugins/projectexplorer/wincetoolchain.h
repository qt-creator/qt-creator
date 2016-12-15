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

namespace ProjectExplorer {
namespace Internal {

class WinCEToolChain : public AbstractMsvcToolChain
{
public:
    WinCEToolChain(const QString &name,
                   const Abi &abi,
                   const QString &vcvarsBat,
                   const QString &msvcVer,
                   const QString &ceVer,
                   const QString &binPath,
                   const QString &includePath,
                   const QString &libPath,
                   Core::Id language,
                   Detection d = ManualDetection);

    Utils::FileNameList suggestedMkspecList() const override;

    static WinCEToolChain *readFromMap(const QVariantMap &data);

    QString typeDisplayName() const override;

    QString msvcVer() const { return m_msvcVer; }
    QString ceVer() const;

    QString binPath() const { return m_binPath; }
    QString includePath() const { return m_includePath; }
    QString libPath() const { return m_libPath; }

    QVariantMap toMap() const override;
    bool fromMap(const QVariantMap &data) override;

    ToolChainConfigWidget *configurationWidget() override;

    ToolChain *clone() const override;

    static QString autoDetectCdbDebugger(QStringList *checkedDirectories = nullptr);

    bool operator ==(const ToolChain &other) const override;

protected:
    Utils::Environment readEnvironmentSetting(const Utils::Environment& env) const final;

private:
    WinCEToolChain();

    QString m_msvcVer;
    QString m_ceVer;
    QString m_binPath;
    QString m_includePath;
    QString m_libPath;
};

// --------------------------------------------------------------------------
// WinCEToolChainConfigWidget
// --------------------------------------------------------------------------
class WinCEToolChainConfigWidget : public ToolChainConfigWidget
{
    Q_OBJECT

public:
    WinCEToolChainConfigWidget(ToolChain *);

private:
    void applyImpl() override { }
    void discardImpl() override { }
    bool isDirtyImpl() const override { return false; }
    void makeReadOnlyImpl() override { }
};

class WinCEToolChainFactory : public ToolChainFactory
{
    Q_OBJECT

public:
    WinCEToolChainFactory();
    QSet<Core::Id> supportedLanguages() const override;

    QList<ToolChain *> autoDetect(const QList<ToolChain *> &alreadyKnown) override;

    bool canRestore(const QVariantMap &data) override;
    ToolChain *restore(const QVariantMap &data) override;

    ToolChainConfigWidget *configurationWidget(ToolChain *);

private:
    QList<ToolChain *> detectCEToolKits(const QString &msvcPath, const QString &vcvarsbat);
};

} // namespace Internal
} // namespace ProjectExplorer
