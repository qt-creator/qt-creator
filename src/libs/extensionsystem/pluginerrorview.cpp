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

#include "pluginerrorview.h"
#include "ui_pluginerrorview.h"
#include "pluginspec.h"

#include <QString>

/*!
    \class ExtensionSystem::PluginErrorView
    \brief The PluginErrorView class implements a widget that displays the
    state and error message of a PluginSpec.

    Can be used for integration in the application that
    uses the plugin manager.

    \sa ExtensionSystem::PluginView
*/

using namespace ExtensionSystem;

/*!
    Constructs a new error view with given \a parent widget.
*/
PluginErrorView::PluginErrorView(QWidget *parent)
    : QWidget(parent),
      m_ui(new Internal::Ui::PluginErrorView())
{
    m_ui->setupUi(this);
}

/*!
    \internal
*/
PluginErrorView::~PluginErrorView()
{
    delete m_ui;
}

/*!
    Reads the given \a spec and displays its state and
    error information in this PluginErrorView.
*/
void PluginErrorView::update(PluginSpec *spec)
{
    QString text;
    QString tooltip;
    switch (spec->state()) {
    case PluginSpec::Invalid:
        text = tr("Invalid");
        tooltip = tr("Description file found, but error on read.");
        break;
    case PluginSpec::Read:
        text = tr("Read");
        tooltip = tr("Description successfully read.");
        break;
    case PluginSpec::Resolved:
        text = tr("Resolved");
        tooltip = tr("Dependencies are successfully resolved.");
        break;
    case PluginSpec::Loaded:
        text = tr("Loaded");
        tooltip = tr("Library is loaded.");
        break;
    case PluginSpec::Initialized:
        text = tr("Initialized");
        tooltip = tr("Plugin's initialization function succeeded.");
        break;
    case PluginSpec::Running:
        text = tr("Running");
        tooltip = tr("Plugin successfully loaded and running.");
        break;
    case PluginSpec::Stopped:
        text = tr("Stopped");
        tooltip = tr("Plugin was shut down.");
        break;
    case PluginSpec::Deleted:
        text = tr("Deleted");
        tooltip = tr("Plugin ended its life cycle and was deleted.");
        break;
    }

    m_ui->state->setText(text);
    m_ui->state->setToolTip(tooltip);
    m_ui->errorString->setText(spec->errorString());
}
