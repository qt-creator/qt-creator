/****************************************************************************
**
** Copyright (C) 2016 Denis Mingulov, The Qt Company Ltd.
** Copyright (C) 2016 Denis Mingulov, The Qt Company Ltd.
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
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
****************************************************************************/

#include "vcsconfigurationpage.h"

#include "../vcsbaseconstants.h"

#include <coreplugin/icore.h>
#include <coreplugin/iversioncontrol.h>
#include <coreplugin/vcsmanager.h>
#include <projectexplorer/jsonwizard/jsonwizard.h>
#include <projectexplorer/jsonwizard/jsonwizardfactory.h>

#include <extensionsystem/pluginmanager.h>
#include <utils/algorithm.h>
#include <utils/qtcassert.h>

#include <QPushButton>
#include <QVBoxLayout>
#include <QWizardPage>

using namespace Core;
using namespace ProjectExplorer;

namespace VcsBase {
namespace Internal {

VcsConfigurationPageFactory::VcsConfigurationPageFactory()
{
    setTypeIdsSuffix(QLatin1String("VcsConfiguration"));
}

Utils::WizardPage *VcsConfigurationPageFactory::create(JsonWizard *wizard, Id typeId,
                                                       const QVariant &data)
{
    Q_UNUSED(wizard);

    QTC_ASSERT(canCreate(typeId), return 0);

    QVariantMap tmp = data.toMap();
    const QString vcsId = tmp.value(QLatin1String("vcsId")).toString();
    QTC_ASSERT(!vcsId.isEmpty(), return 0);

    auto page = new VcsConfigurationPage;
    page->setVersionControlId(vcsId);

    return page;
}

bool VcsConfigurationPageFactory::validateData(Id typeId, const QVariant &data,
                                               QString *errorMessage)
{
    QTC_ASSERT(canCreate(typeId), return false);

    if (data.isNull() || data.type() != QVariant::Map) {
        //: Do not translate "VcsConfiguration", because it is the id of a page.
        *errorMessage = QCoreApplication::translate("ProjectExplorer::JsonWizard",
                                                    "\"data\" must be a JSON object for \"VcsConfiguration\" pages.");
        return false;
    }

    QVariantMap tmp = data.toMap();
    const QString vcsId = tmp.value(QLatin1String("vcsId")).toString();
    if (vcsId.isEmpty()) {
        //: Do not translate "VcsConfiguration", because it is the id of a page.
        *errorMessage = QCoreApplication::translate("ProjectExplorer::JsonWizard",
                                                    "\"VcsConfiguration\" page requires a \"vcsId\" set.");
        return false;
    }
    return true;
}

class VcsConfigurationPagePrivate
{
public:
    const IVersionControl *m_versionControl;
    QString m_versionControlId;
    QPushButton *m_configureButton;
};

} // namespace Internal

VcsConfigurationPage::VcsConfigurationPage() : d(new Internal::VcsConfigurationPagePrivate)
{
    setTitle(tr("Configuration"));

    d->m_versionControl = 0;
    d->m_configureButton = new QPushButton(ICore::msgShowOptionsDialog(), this);
    d->m_configureButton->setEnabled(false);

    auto verticalLayout = new QVBoxLayout(this);
    verticalLayout->addWidget(d->m_configureButton);

    connect(d->m_configureButton, &QAbstractButton::clicked,
            this, &VcsConfigurationPage::openConfiguration);
}

VcsConfigurationPage::~VcsConfigurationPage()
{
    delete d;
}

void VcsConfigurationPage::setVersionControl(const IVersionControl *vc)
{
    if (vc)
        d->m_versionControlId = vc->id().toString();
    else
        d->m_versionControlId.clear();
    d->m_versionControl = 0;
}

void VcsConfigurationPage::setVersionControlId(const QString &id)
{
    d->m_versionControlId = id;
}

void VcsConfigurationPage::initializePage()
{
    if (d->m_versionControl) {
        disconnect(d->m_versionControl, &IVersionControl::configurationChanged,
                   this, &QWizardPage::completeChanged);
    }

    if (!d->m_versionControlId.isEmpty()) {
        auto jw = qobject_cast<JsonWizard *>(wizard());
        if (!jw) {
            //: Do not translate "VcsConfiguration", because it is the id of a page.
            emit reportError(tr("No version control set on \"VcsConfiguration\" page."));
        }

        const QString vcsId = jw ? jw->expander()->expand(d->m_versionControlId) : d->m_versionControlId;

        d->m_versionControl = VcsManager::versionControl(Id::fromString(vcsId));
        if (!d->m_versionControl) {
            emit reportError(
                        //: Do not translate "VcsConfiguration", because it is the id of a page.
                        tr("\"vcsId\" (\"%1\") is invalid for \"VcsConfiguration\" page. "
                           "Possible values are: %2.")
                        .arg(vcsId)
                        .arg(QStringList(Utils::transform(VcsManager::versionControls(), [](const IVersionControl *vc) {
                return vc->id().toString();
            })).join(QLatin1String(", "))));
        }
    }

    connect(d->m_versionControl, &IVersionControl::configurationChanged,
            this, &QWizardPage::completeChanged);

    d->m_configureButton->setEnabled(d->m_versionControl);
    if (d->m_versionControl)
        setSubTitle(tr("Please configure <b>%1</b> now.").arg(d->m_versionControl->displayName()));
    else
        setSubTitle(tr("No known version control selected."));
}

bool VcsConfigurationPage::isComplete() const
{
    return d->m_versionControl ? d->m_versionControl->isConfigured() : false;
}

void VcsConfigurationPage::openConfiguration()
{
    ICore::showOptionsDialog(d->m_versionControl->id(), this);
}

} // namespace VcsBase
