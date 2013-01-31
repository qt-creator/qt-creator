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

#include "vcsconfigurationpage.h"

#include "vcsbaseconstants.h"

#include <coreplugin/dialogs/iwizard.h>
#include <coreplugin/icore.h>
#include <coreplugin/iversioncontrol.h>

#include <utils/qtcassert.h>

#include <QPushButton>
#include <QVBoxLayout>
#include <QWizardPage>

namespace VcsBase {
namespace Internal {

class VcsConfigurationPagePrivate
{
public:
    const Core::IVersionControl *m_versionControl;
    QPushButton *m_configureButton;
};

} // namespace Internal

VcsConfigurationPage::VcsConfigurationPage(const Core::IVersionControl *vc, QWidget *parent) :
    QWizardPage(parent),
    d(new Internal::VcsConfigurationPagePrivate)
{
    QTC_CHECK(vc);
    setTitle(tr("Configuration"));
    setSubTitle(tr("Please configure <b>%1</b> now.").arg(vc->displayName()));

    d->m_versionControl = vc;
    d->m_configureButton = new QPushButton(tr("Configure..."), this);

    QVBoxLayout *verticalLayout = new QVBoxLayout(this);
    verticalLayout->addWidget(d->m_configureButton);

    connect(d->m_versionControl, SIGNAL(configurationChanged()), SIGNAL(completeChanged()));
    connect(d->m_configureButton, SIGNAL(clicked()), SLOT(openConfiguration()));
}

VcsConfigurationPage::~VcsConfigurationPage()
{
    delete d;
}

bool VcsConfigurationPage::isComplete() const
{
    return d->m_versionControl->isConfigured();
}

void VcsConfigurationPage::openConfiguration()
{
    Core::ICore::showOptionsDialog(Constants::VCS_SETTINGS_CATEGORY,
                                   d->m_versionControl->id());
}

} // namespace VcsBase
