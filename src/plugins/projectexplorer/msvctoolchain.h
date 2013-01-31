/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef MSVCTOOLCHAIN_H
#define MSVCTOOLCHAIN_H

#include "abstractmsvctoolchain.h"
#include "abi.h"
#include "toolchainconfigwidget.h"

#include <utils/environment.h>

#include <QLabel>

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
                    ia64,
                    arm
                  };

    MsvcToolChain(const QString &name, const Abi &abi,
                  const QString &varsBat, const QString &varsBatArg, bool autodetect = false);
    bool isValid() const;
    QList<Utils::FileName> suggestedMkspecList() const;

    static MsvcToolChain *readFromMap(const QVariantMap &data);

    QString type() const;
    QString typeDisplayName() const;

    QVariantMap toMap() const;
    bool fromMap(const QVariantMap &data);

    ToolChainConfigWidget *configurationWidget();

    ToolChain *clone() const;

    QString varsBatArg() const { return m_varsBatArg; }

    bool operator == (const ToolChain &) const;

protected:
    Utils::Environment readEnvironmentSetting(Utils::Environment& env) const;
    QByteArray msvcPredefinedMacros(const QStringList cxxflags,
                                    const Utils::Environment &env) const;

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
    QString displayName() const;
    QString id() const;

    QList<ToolChain *> autoDetect();

    virtual bool canRestore(const QVariantMap &data);
    virtual ToolChain *restore(const QVariantMap &data)
        { return MsvcToolChain::readFromMap(data); }

    ToolChainConfigWidget *configurationWidget(ToolChain *);
    static QString vcVarsBatFor(const QString &basePath, const QString &toolchainName);
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
    void applyImpl() {}
    void discardImpl() { setFromToolChain(); }
    bool isDirtyImpl() const { return false; }
    void makeReadOnlyImpl() {}

    void setFromToolChain();

    QLabel *m_varsBatDisplayLabel;
};

} // namespace Internal
} // namespace ProjectExplorer

#endif // MSVCTOOLCHAIN_H
