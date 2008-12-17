/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.3, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

#include "plugindetailsview.h"
#include "ui_plugindetailsview.h"

/*!
    \class ExtensionSystem::PluginDetailsView
    \brief Widget that displays the contents of a PluginSpec.

    Can be used for integration in the application that
    uses the plugin manager.

    \sa ExtensionSystem::PluginView
*/

using namespace ExtensionSystem;

/*!
    \fn PluginDetailsView::PluginDetailsView(QWidget *parent)
    Constructs a new view with given \a parent widget.
*/
PluginDetailsView::PluginDetailsView(QWidget *parent)
	: QWidget(parent),
          m_ui(new Internal::Ui::PluginDetailsView())
{
    m_ui->setupUi(this);
}

/*!
    \fn PluginDetailsView::~PluginDetailsView()
    \internal
*/
PluginDetailsView::~PluginDetailsView()
{
    delete m_ui;
}

/*!
    \fn void PluginDetailsView::update(PluginSpec *spec)
    Reads the given \a spec and displays its values
    in this PluginDetailsView.
*/
void PluginDetailsView::update(PluginSpec *spec)
{
    m_ui->name->setText(spec->name());
    m_ui->version->setText(spec->version());
    m_ui->compatVersion->setText(spec->compatVersion());
    m_ui->vendor->setText(spec->vendor());
    m_ui->url->setText(spec->url());
    m_ui->location->setText(spec->filePath());
    m_ui->description->setText(spec->description());
    m_ui->copyright->setText(spec->copyright());
    m_ui->license->setText(spec->license());
    QStringList depStrings;
    foreach (PluginDependency dep, spec->dependencies()) {
        depStrings << QString("%1 (%2)").arg(dep.name).arg(dep.version);
    }
    m_ui->dependencies->addItems(depStrings);
}
