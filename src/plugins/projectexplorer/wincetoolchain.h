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

#ifndef WINCETOOLCHAIN_H
#define WINCETOOLCHAIN_H

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
                   Detection d = ManualDetection);

    QList<Utils::FileName> suggestedMkspecList() const override;

    static WinCEToolChain *readFromMap(const QVariantMap &data);

    QString typeDisplayName() const override;

    QString ceVer() const;

    QVariantMap toMap() const override;
    bool fromMap(const QVariantMap &data) override;

    ToolChainConfigWidget *configurationWidget() override;

    ToolChain *clone() const override;

    static QString autoDetectCdbDebugger(QStringList *checkedDirectories = 0);

    bool operator ==(const ToolChain &other) const override;

protected:
    Utils::Environment readEnvironmentSetting(Utils::Environment& env) const override;

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

    QList<ToolChain *> autoDetect() override;

    bool canRestore(const QVariantMap &data) override;
    ToolChain *restore(const QVariantMap &data) override;

    ToolChainConfigWidget *configurationWidget(ToolChain *);

private:
    QList<ToolChain *> detectCEToolKits(const QString &msvcPath, const QString &vcvarsbat);
};


} // namespace Internal
} // namespace ProjectExplorer

#endif // MSVCTOOLCHAIN_H
