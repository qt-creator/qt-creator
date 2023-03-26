// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "vcsconfigurationpage.h"

#include "../vcsbasetr.h"

#include <coreplugin/icore.h>
#include <coreplugin/iversioncontrol.h>
#include <coreplugin/vcsmanager.h>

#include <extensionsystem/pluginmanager.h>

#include <projectexplorer/projectexplorertr.h>
#include <projectexplorer/jsonwizard/jsonwizard.h>
#include <projectexplorer/jsonwizard/jsonwizardfactory.h>

#include <utils/algorithm.h>
#include <utils/qtcassert.h>

#include <QPushButton>
#include <QVBoxLayout>
#include <QWizardPage>

using namespace Core;
using namespace ProjectExplorer;
using namespace Utils;

namespace VcsBase {
namespace Internal {

VcsConfigurationPageFactory::VcsConfigurationPageFactory()
{
    setTypeIdsSuffix(QLatin1String("VcsConfiguration"));
}

WizardPage *VcsConfigurationPageFactory::create(JsonWizard *wizard, Id typeId,
                                                const QVariant &data)
{
    Q_UNUSED(wizard)

    QTC_ASSERT(canCreate(typeId), return nullptr);

    QVariantMap tmp = data.toMap();
    const QString vcsId = tmp.value(QLatin1String("vcsId")).toString();
    QTC_ASSERT(!vcsId.isEmpty(), return nullptr);

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
        *errorMessage = ProjectExplorer::Tr::tr("\"data\" must be a JSON object for \"VcsConfiguration\" pages.");
        return false;
    }

    QVariantMap tmp = data.toMap();
    const QString vcsId = tmp.value(QLatin1String("vcsId")).toString();
    if (vcsId.isEmpty()) {
        //: Do not translate "VcsConfiguration", because it is the id of a page.
        *errorMessage = ProjectExplorer::Tr::tr("\"VcsConfiguration\" page requires a \"vcsId\" set.");
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
    setTitle(Tr::tr("Configuration"));

    d->m_versionControl = nullptr;
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
    d->m_versionControl = nullptr;
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
            emit reportError(Tr::tr("No version control set on \"VcsConfiguration\" page."));
        }

        const QString vcsId = jw ? jw->expander()->expand(d->m_versionControlId) : d->m_versionControlId;

        d->m_versionControl = VcsManager::versionControl(Id::fromString(vcsId));
        if (!d->m_versionControl) {
            const QString values = Utils::transform(VcsManager::versionControls(),
                [](const IVersionControl *vc) { return vc->id().toString(); }).join(", ");
            //: Do not translate "VcsConfiguration", because it is the id of a page.
            emit reportError(Tr::tr("\"vcsId\" (\"%1\") is invalid for \"VcsConfiguration\" page. "
                                    "Possible values are: %2.").arg(vcsId, values));
        }
    }

    connect(d->m_versionControl, &IVersionControl::configurationChanged,
            this, &QWizardPage::completeChanged);

    d->m_configureButton->setEnabled(d->m_versionControl);
    if (d->m_versionControl)
        setSubTitle(Tr::tr("Please configure <b>%1</b> now.").arg(d->m_versionControl->displayName()));
    else
        setSubTitle(Tr::tr("No known version control selected."));
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
