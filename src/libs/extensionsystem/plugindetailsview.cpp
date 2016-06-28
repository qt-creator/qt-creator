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

#include "plugindetailsview.h"
#include "ui_plugindetailsview.h"

#include "pluginmanager.h"
#include "pluginspec.h"

#include <utils/algorithm.h>

#include <QDir>
#include <QRegExp>

/*!
    \class ExtensionSystem::PluginDetailsView
    \brief The PluginDetailsView class implements a widget that displays the
    contents of a PluginSpec.

    Can be used for integration in the application that
    uses the plugin manager.

    \sa ExtensionSystem::PluginView
*/

using namespace ExtensionSystem;

/*!
    Constructs a new view with given \a parent widget.
*/
PluginDetailsView::PluginDetailsView(QWidget *parent)
	: QWidget(parent),
          m_ui(new Internal::Ui::PluginDetailsView())
{
    m_ui->setupUi(this);
}

/*!
    \internal
*/
PluginDetailsView::~PluginDetailsView()
{
    delete m_ui;
}

/*!
    Reads the given \a spec and displays its values
    in this PluginDetailsView.
*/
void PluginDetailsView::update(PluginSpec *spec)
{
    m_ui->name->setText(spec->name());
    m_ui->version->setText(spec->version());
    m_ui->compatVersion->setText(spec->compatVersion());
    m_ui->vendor->setText(spec->vendor());
    const QString link = QString::fromLatin1("<a href=\"%1\">%1</a>").arg(spec->url());
    m_ui->url->setText(link);
    QString component = tr("None");
    if (!spec->category().isEmpty())
        component = spec->category();
    m_ui->component->setText(component);
    m_ui->location->setText(QDir::toNativeSeparators(spec->filePath()));
    m_ui->description->setText(spec->description());
    m_ui->copyright->setText(spec->copyright());
    m_ui->license->setText(spec->license());
    const QRegExp platforms = spec->platformSpecification();
    const QString pluginPlatformString = platforms.isEmpty() ? tr("All") : platforms.pattern();
    const QString platformString = tr("%1 (current: \"%2\")").arg(pluginPlatformString,
                                                                  PluginManager::platformName());
    m_ui->platforms->setText(platformString);
    const QStringList depStrings = Utils::transform<QList>(spec->dependencies(), &PluginDependency::toString);
    m_ui->dependencies->addItems(depStrings);
}
