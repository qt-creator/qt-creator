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

#include "plugincollection.h"
#include "pluginspec.h"

namespace ExtensionSystem {

PluginCollection::PluginCollection(const QString& name) :
    m_name(name)
{

}

PluginCollection::~PluginCollection()
{

}

QString PluginCollection::name() const
{
    return m_name;
}

void PluginCollection::addPlugin(PluginSpec *spec)
{
    m_plugins.append(spec);
}

void PluginCollection::removePlugin(PluginSpec *spec)
{
    m_plugins.removeOne(spec);
}

QList<PluginSpec *> PluginCollection::plugins() const
{
    return m_plugins;
}

}
