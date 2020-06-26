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

#include "projectconfiguration.h"
#include "target.h"

#include <utils/algorithm.h>
#include <utils/qtcassert.h>

#include <QFormLayout>
#include <QWidget>

using namespace ProjectExplorer;

const char CONFIGURATION_ID_KEY[] = "ProjectExplorer.ProjectConfiguration.Id";
const char DISPLAY_NAME_KEY[] = "ProjectExplorer.ProjectConfiguration.DisplayName";

// ProjectConfigurationAspect

ProjectConfigurationAspect::ProjectConfigurationAspect() = default;

ProjectConfigurationAspect::~ProjectConfigurationAspect() = default;

void ProjectConfigurationAspect::setConfigWidgetCreator
    (const ConfigWidgetCreator &configWidgetCreator)
{
    m_configWidgetCreator = configWidgetCreator;
}

QWidget *ProjectConfigurationAspect::createConfigWidget() const
{
    return m_configWidgetCreator ? m_configWidgetCreator() : nullptr;
}

void ProjectConfigurationAspect::addToLayout(LayoutBuilder &)
{
}

// LayoutBuilder

LayoutBuilder::LayoutBuilder(QWidget *parent)
    : m_layout(new QFormLayout(parent))
{
    m_layout->setContentsMargins(0, 0, 0, 0);
    if (auto fl = qobject_cast<QFormLayout *>(m_layout))
        fl->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
}

LayoutBuilder::~LayoutBuilder()
{
    flushPendingItems();
}

LayoutBuilder &LayoutBuilder::startNewRow()
{
    flushPendingItems();
    return *this;
}

void LayoutBuilder::flushPendingItems()
{
    if (m_pendingItems.isEmpty())
        return;

    if (auto fl = qobject_cast<QFormLayout *>(m_layout)) {
        // If there are more than two items, we cram the last ones in one hbox.
        if (m_pendingItems.size() > 2) {
            auto hbox = new QHBoxLayout;
            for (int i = 1; i < m_pendingItems.size(); ++i) {
                if (QWidget *w = m_pendingItems.at(i).widget)
                    hbox->addWidget(w);
                else if (QLayout *l = m_pendingItems.at(i).layout)
                    hbox->addItem(l);
                else
                    QTC_CHECK(false);
            }
            while (m_pendingItems.size() >= 2)
                m_pendingItems.takeLast();
            m_pendingItems.append(LayoutItem(hbox));
        }

        if (m_pendingItems.size() == 1) { // One one item given, so this spans both columns.
            if (auto layout = m_pendingItems.at(0).layout)
                fl->addRow(layout);
            else if (auto widget = m_pendingItems.at(0).widget)
                fl->addRow(widget);
        } else if (m_pendingItems.size() == 2) { // Normal case, both columns used.
            if (auto label = m_pendingItems.at(0).widget) {
                if (auto layout = m_pendingItems.at(1).layout)
                    fl->addRow(label, layout);
                else if (auto widget = m_pendingItems.at(1).widget)
                    fl->addRow(label, widget);
            } else  {
                if (auto layout = m_pendingItems.at(1).layout)
                    fl->addRow(m_pendingItems.at(0).text, layout);
                else if (auto widget = m_pendingItems.at(1).widget)
                    fl->addRow(m_pendingItems.at(0).text, widget);
            }
        } else {
            QTC_CHECK(false);
        }
    } else {
        QTC_CHECK(false);
    }

    m_pendingItems.clear();
}

QLayout *LayoutBuilder::layout() const
{
    return m_layout;
}

LayoutBuilder &LayoutBuilder::addItem(LayoutItem item)
{
    if (item.widget && !item.widget->parent())
        item.widget->setParent(m_layout->parentWidget());
    m_pendingItems.append(item);
    return *this;
}


// ProjectConfigurationAspects

ProjectConfigurationAspects::ProjectConfigurationAspects() = default;

ProjectConfigurationAspects::~ProjectConfigurationAspects()
{
    qDeleteAll(base());
}

ProjectConfigurationAspect *ProjectConfigurationAspects::aspect(Utils::Id id) const
{
    return Utils::findOrDefault(base(), Utils::equal(&ProjectConfigurationAspect::id, id));
}

void ProjectConfigurationAspects::fromMap(const QVariantMap &map) const
{
    for (ProjectConfigurationAspect *aspect : *this)
        aspect->fromMap(map);
}

void ProjectConfigurationAspects::toMap(QVariantMap &map) const
{
    for (ProjectConfigurationAspect *aspect : *this)
        aspect->toMap(map);
}


// ProjectConfiguration

ProjectConfiguration::ProjectConfiguration(QObject *parent, Utils::Id id)
    : QObject(parent)
    , m_id(id)
{
    QTC_CHECK(parent);
    QTC_CHECK(id.isValid());
    setObjectName(id.toString());

    for (QObject *obj = this; obj; obj = obj->parent()) {
        m_target = qobject_cast<Target *>(obj);
        if (m_target)
            break;
    }
    QTC_CHECK(m_target);
}

ProjectConfiguration::~ProjectConfiguration() = default;

Project *ProjectConfiguration::project() const
{
    return m_target->project();
}

Utils::Id ProjectConfiguration::id() const
{
    return m_id;
}

QString ProjectConfiguration::settingsIdKey()
{
    return QString(CONFIGURATION_ID_KEY);
}

void ProjectConfiguration::setDisplayName(const QString &name)
{
    if (m_displayName.setValue(name))
        emit displayNameChanged();
}

void ProjectConfiguration::setDefaultDisplayName(const QString &name)
{
    if (m_displayName.setDefaultValue(name))
        emit displayNameChanged();
}

void ProjectConfiguration::setToolTip(const QString &text)
{
    if (text == m_toolTip)
        return;
    m_toolTip = text;
    emit toolTipChanged();
}

QString ProjectConfiguration::toolTip() const
{
    return m_toolTip;
}

QVariantMap ProjectConfiguration::toMap() const
{
    QTC_CHECK(m_id.isValid());
    QVariantMap map;
    map.insert(QLatin1String(CONFIGURATION_ID_KEY), m_id.toSetting());
    m_displayName.toMap(map, DISPLAY_NAME_KEY);
    m_aspects.toMap(map);
    return map;
}

Target *ProjectConfiguration::target() const
{
    return m_target;
}

bool ProjectConfiguration::fromMap(const QVariantMap &map)
{
    Utils::Id id = Utils::Id::fromSetting(map.value(QLatin1String(CONFIGURATION_ID_KEY)));
    // Note: This is only "startsWith", not ==, as RunConfigurations currently still
    // mangle in their build keys.
    QTC_ASSERT(id.toString().startsWith(m_id.toString()), return false);

    m_displayName.fromMap(map, DISPLAY_NAME_KEY);
    m_aspects.fromMap(map);
    return true;
}

ProjectConfigurationAspect *ProjectConfiguration::aspect(Utils::Id id) const
{
    return m_aspects.aspect(id);
}

void ProjectConfiguration::acquaintAspects()
{
    for (ProjectConfigurationAspect *aspect : m_aspects)
        aspect->acquaintSiblings(m_aspects);
}

Utils::Id ProjectExplorer::idFromMap(const QVariantMap &map)
{
    return Utils::Id::fromSetting(map.value(QLatin1String(CONFIGURATION_ID_KEY)));
}
