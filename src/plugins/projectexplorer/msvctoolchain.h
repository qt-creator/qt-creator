/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

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
    enum Platform { s32, s64, ia64, amd64 };

    MsvcToolChain(const QString &name, const Abi &abi,
                  const QString &varsBat, const QString &varsBatArg, bool autodetect = false);
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
};

// --------------------------------------------------------------------------
// MsvcToolChainConfigWidget
// --------------------------------------------------------------------------

class MsvcToolChainConfigWidget : public ToolChainConfigWidget
{
    Q_OBJECT

public:
    MsvcToolChainConfigWidget(ToolChain *);

    void apply();
    void discard() { setFromToolChain(); }
    bool isDirty() const;

private:
    void setFromToolChain();

    QLabel *m_varsBatDisplayLabel;
};

} // namespace Internal
} // namespace ProjectExplorer

#endif // MSVCTOOLCHAIN_H
