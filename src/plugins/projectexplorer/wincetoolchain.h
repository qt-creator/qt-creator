/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#ifndef WINCETOOLCHAIN_H
#define WINCETOOLCHAIN_H

#include "abstractmsvctoolchain.h"
#include "abi.h"
#include "toolchainconfigwidget.h"

#include <utils/environment.h>

#include <QLabel>

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
                   bool autodetect = false);

    QList<Utils::FileName> suggestedMkspecList() const;

    static WinCEToolChain *readFromMap(const QVariantMap &data);

    QString type() const;
    QString typeDisplayName() const;

    QString ceVer() const;

    QVariantMap toMap() const;
    bool fromMap(const QVariantMap &data);

    ToolChainConfigWidget *configurationWidget();

    ToolChain *clone() const;

    static QString autoDetectCdbDebugger(QStringList *checkedDirectories = 0);

    bool operator ==(const ToolChain &other) const;
protected:
    Utils::Environment readEnvironmentSetting(Utils::Environment& env) const;

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
    void applyImpl() {}
    void discardImpl() { }
    bool isDirtyImpl() const {return false;}
    void makeReadOnlyImpl() {}
};

class WinCEToolChainFactory : public ToolChainFactory
{
    Q_OBJECT

public:
    QString displayName() const;
    QString id() const;

    QList<ToolChain *> autoDetect();

    bool canRestore(const QVariantMap &data);
    ToolChain *restore(const QVariantMap &data);

    ToolChainConfigWidget *configurationWidget(ToolChain *);

private:
    QList<ToolChain *> detectCEToolKits(const QString &msvcPath, const QString &vcvarsbat);
};


} // namespace Internal
} // namespace ProjectExplorer

#endif // MSVCTOOLCHAIN_H
