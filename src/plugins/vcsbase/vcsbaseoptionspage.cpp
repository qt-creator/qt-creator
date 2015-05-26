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

#include "vcsbaseoptionspage.h"

#include "vcsbaseclient.h"
#include "vcsbaseconstants.h"

#include <coreplugin/icore.h>
#include <coreplugin/iversioncontrol.h>

#include <utils/qtcassert.h>

#include <QCoreApplication>

/*!
    \class VcsBase::VcsBaseOptionsPage

    \brief The VcsBaseOptionsPage class is the base class for VCS options pages
    providing a common category and icon.
 */

namespace VcsBase {

VcsBaseOptionsPage::VcsBaseOptionsPage(QObject *parent) : Core::IOptionsPage(parent)
{
    setCategory(Constants::VCS_SETTINGS_CATEGORY);
    setDisplayCategory(QCoreApplication::translate("VcsBase", Constants::VCS_SETTINGS_TR_CATEGORY));
    setCategoryIcon(QLatin1String(Constants::SETTINGS_CATEGORY_VCS_ICON));
}

VcsBaseOptionsPage::~VcsBaseOptionsPage()
{ }

VcsClientOptionsPageWidget::VcsClientOptionsPageWidget(QWidget *parent) : QWidget(parent)
{ }

VcsClientOptionsPage::VcsClientOptionsPage(Core::IVersionControl *control, VcsBaseClientImpl *client,
                                           QObject *parent) :
    VcsBaseOptionsPage(parent),
    m_widget(0),
    m_client(client)
{
    QTC_CHECK(m_client);
    connect(this, &VcsClientOptionsPage::settingsChanged,
            control, &Core::IVersionControl::configurationChanged);
}

void VcsClientOptionsPage::setWidgetFactory(VcsClientOptionsPage::WidgetFactory factory)
{
    QTC_ASSERT(!m_factory, return);
    m_factory = factory;
}

VcsClientOptionsPageWidget *VcsClientOptionsPage::widget()
{
    QTC_ASSERT(m_factory, return 0);
    if (!m_widget)
        m_widget = m_factory();
    QTC_ASSERT(m_widget, return 0);
    m_widget->setSettings(m_client->settings());
    return m_widget;
}

void VcsClientOptionsPage::apply()
{
    QTC_ASSERT(m_widget, return);
    const VcsBaseClientSettings newSettings = m_widget->settings();
    VcsBaseClientSettings &s = m_client->settings();
    if (s != newSettings) {
        s = newSettings;
        emit settingsChanged();
    }
}

void VcsClientOptionsPage::finish()
{
    delete m_widget;
    m_widget = 0;
}

} // namespace VcsBase
