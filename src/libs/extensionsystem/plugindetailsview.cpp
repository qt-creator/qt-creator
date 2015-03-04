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

#include "plugindetailsview.h"
#include "ui_plugindetailsview.h"

#include "pluginmanager.h"
#include "pluginspec.h"

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
    QStringList depStrings;
    foreach (const PluginDependency &dep, spec->dependencies()) {
        QString depString = dep.name;
        depString += QLatin1String(" (");
        depString += dep.version;
        switch (dep.type) {
        case PluginDependency::Required:
            break;
        case PluginDependency::Optional:
            depString += QLatin1String(", optional");
            break;
        case PluginDependency::Test:
            depString += QLatin1String(", test");
            break;
        }
        depString += QLatin1Char(')');
        depStrings.append(depString);
    }
    m_ui->dependencies->addItems(depStrings);
}
