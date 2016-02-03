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

#include "../vcsbase_global.h"

#include <projectexplorer/jsonwizard/jsonwizardpagefactory.h>

#include <utils/wizardpage.h>

namespace Core { class IVersionControl; }

namespace VcsBase {

namespace Internal {
class VcsConfigurationPagePrivate;

class VcsConfigurationPageFactory : public ProjectExplorer::JsonWizardPageFactory
{
public:
    VcsConfigurationPageFactory();

    Utils::WizardPage *create(ProjectExplorer::JsonWizard *wizard, Core::Id typeId,
                              const QVariant &data) override;
    bool validateData(Core::Id typeId, const QVariant &data, QString *errorMessage) override;
};

} // namespace Internal

class VCSBASE_EXPORT VcsConfigurationPage : public Utils::WizardPage
{
    Q_OBJECT

public:
    VcsConfigurationPage();
    ~VcsConfigurationPage() override;

    void setVersionControl(const Core::IVersionControl *vc);
    void setVersionControlId(const QString &id);

    void initializePage() override;
    bool isComplete() const override;

private:
    void openConfiguration();

    Internal::VcsConfigurationPagePrivate *const d;
};

} // namespace VcsBase
