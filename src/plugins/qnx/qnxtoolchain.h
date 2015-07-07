/**************************************************************************
**
** Copyright (C) 2015 BlackBerry Limited. All rights reserved.
**
** Contact: BlackBerry (qt@blackberry.com)
** Contact: KDAB (info@kdab.com)
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

#ifndef QNXTOOLCHAIN_H
#define QNXTOOLCHAIN_H

#include <projectexplorer/gcctoolchain.h>
#include <projectexplorer/gcctoolchainfactories.h>

namespace Qnx {
namespace Internal {

class QnxToolChain : public ProjectExplorer::GccToolChain
{
public:
    explicit QnxToolChain(Detection d);

    QString typeDisplayName() const override;

    ProjectExplorer::ToolChainConfigWidget *configurationWidget() override;

    void addToEnvironment(Utils::Environment &env) const override;
    QList<Utils::FileName> suggestedMkspecList() const override;

    QVariantMap toMap() const override;
    bool fromMap(const QVariantMap &data) override;

    QString ndkPath() const;
    void setNdkPath(const QString &ndkPath);

protected:
    virtual QList<ProjectExplorer::Abi> detectSupportedAbis() const override;

    QStringList reinterpretOptions(const QStringList &args) const override;

private:
    QString m_ndkPath;
};

// --------------------------------------------------------------------------
// QnxToolChainFactory
// --------------------------------------------------------------------------

class QnxToolChainFactory : public ProjectExplorer::ToolChainFactory
{
    Q_OBJECT

public:
    QnxToolChainFactory();

    bool canRestore(const QVariantMap &data) override;
    ProjectExplorer::ToolChain *restore(const QVariantMap &data) override;

    bool canCreate() override;
    ProjectExplorer::ToolChain *create() override;
};

//----------------------------------------------------------------------------
// QnxToolChainConfigWidget
//----------------------------------------------------------------------------

class QnxToolChainConfigWidget : public ProjectExplorer::ToolChainConfigWidget
{
    Q_OBJECT

public:
    QnxToolChainConfigWidget(QnxToolChain *tc);

private:
    void applyImpl() override;
    void discardImpl() override;
    bool isDirtyImpl() const override;
    void makeReadOnlyImpl() override { }

    Utils::PathChooser *m_compilerCommand;
    Utils::PathChooser *m_ndkPath;
    ProjectExplorer::AbiWidget *m_abiWidget;

};

} // namespace Internal
} // namespace Qnx

#endif // QNXTOOLCHAIN_H
