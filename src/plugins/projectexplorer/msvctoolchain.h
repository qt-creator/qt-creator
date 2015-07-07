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

#ifndef MSVCTOOLCHAIN_H
#define MSVCTOOLCHAIN_H

#include "abstractmsvctoolchain.h"
#include "abi.h"
#include "toolchainconfigwidget.h"

QT_FORWARD_DECLARE_CLASS(QLabel)

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
                    amd64_arm
                  };

    MsvcToolChain(const QString &name, const Abi &abi,
                  const QString &varsBat, const QString &varsBatArg, Detection d = ManualDetection);
    bool isValid() const override;
    QList<Utils::FileName> suggestedMkspecList() const override;

    static MsvcToolChain *readFromMap(const QVariantMap &data);

    QString typeDisplayName() const override;

    QVariantMap toMap() const override;
    bool fromMap(const QVariantMap &data) override;

    ToolChainConfigWidget *configurationWidget() override;

    ToolChain *clone() const override;

    QString varsBatArg() const { return m_varsBatArg; }

    bool operator == (const ToolChain &) const override;

protected:
    Utils::Environment readEnvironmentSetting(Utils::Environment& env) const override;
    QByteArray msvcPredefinedMacros(const QStringList cxxflags,
                                    const Utils::Environment &env) const override;

private:
    MsvcToolChain();

    QString m_varsBatArg; // Argument
};

// --------------------------------------------------------------------------
// MsvcToolChainFactory
// --------------------------------------------------------------------------

class MsvcToolChainFactory : public ToolChainFactory
{
    Q_OBJECT

public:
    MsvcToolChainFactory();

    QList<ToolChain *> autoDetect() override;

    bool canRestore(const QVariantMap &data) override;
    ToolChain *restore(const QVariantMap &data) override
    { return MsvcToolChain::readFromMap(data); }

    ToolChainConfigWidget *configurationWidget(ToolChain *);
    static QString vcVarsBatFor(const QString &basePath, const QString &toolchainName);
    static QString vcVarsBatFor(const QString &basePath, MsvcToolChain::Platform platform);
private:
    static bool checkForVisualStudioInstallation(const QString &vsName);
};

// --------------------------------------------------------------------------
// MsvcToolChainConfigWidget
// --------------------------------------------------------------------------

class MsvcToolChainConfigWidget : public ToolChainConfigWidget
{
    Q_OBJECT

public:
    MsvcToolChainConfigWidget(ToolChain *);

private:
    void applyImpl() override {}
    void discardImpl() override { setFromToolChain(); }
    bool isDirtyImpl() const override { return false; }
    void makeReadOnlyImpl() override {}

    void setFromToolChain();

    QLabel *m_varsBatDisplayLabel;
};

} // namespace Internal
} // namespace ProjectExplorer

#endif // MSVCTOOLCHAIN_H
