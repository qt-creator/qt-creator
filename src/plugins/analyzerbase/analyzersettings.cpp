/**************************************************************************
**
** This file is part of Qt Creator Instrumentation Tools
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Author: Milian Wolff, KDAB (milian.wolff@kdab.com)
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#include "analyzersettings.h"

#include "analyzermanager.h"
#include "ianalyzertool.h"
#include "analyzerplugin.h"
#include "analyzeroptionspage.h"

#include <coreplugin/icore.h>
#include <utils/qtcassert.h>

#include <QtCore/QSettings>

using namespace Analyzer::Internal;

static const char groupC[] = "Analyzer";

namespace Analyzer {

AnalyzerGlobalSettings *AnalyzerGlobalSettings::m_instance = 0;

AnalyzerSettings::AnalyzerSettings(QObject *parent)
    : QObject(parent)
{
}

bool AnalyzerSettings::fromMap(const QVariantMap &map)
{
    bool ret = true;
    foreach (AbstractAnalyzerSubConfig *config, subConfigs()) {
        ret = ret && config->fromMap(map);
    }
    return ret;
}

QVariantMap AnalyzerSettings::defaults() const
{
    QVariantMap map;
    foreach (AbstractAnalyzerSubConfig *config, subConfigs()) {
        map.unite(config->defaults());
    }
    return map;
}

QVariantMap AnalyzerSettings::toMap() const
{
    QVariantMap map;
    foreach (AbstractAnalyzerSubConfig *config, subConfigs()) {
        map.unite(config->toMap());
    }
    return map;
}

AnalyzerGlobalSettings::AnalyzerGlobalSettings(QObject *parent)
: AnalyzerSettings(parent)
{
    QTC_ASSERT(!m_instance, return);
    m_instance = this;
}

AnalyzerGlobalSettings *AnalyzerGlobalSettings::instance()
{
    if (!m_instance)
        m_instance = new AnalyzerGlobalSettings(AnalyzerPlugin::instance());

    return m_instance;
}

AnalyzerGlobalSettings::~AnalyzerGlobalSettings()
{
    m_instance = 0;
}

void AnalyzerGlobalSettings::readSettings()
{
    QSettings *settings = Core::ICore::instance()->settings();

    QVariantMap map;

    settings->beginGroup(QLatin1String(groupC));
    // read the values from config, using the keys from the defaults value map
    const QVariantMap def = defaults();
    for (QVariantMap::ConstIterator it = def.constBegin(); it != def.constEnd(); ++it)
        map.insert(it.key(), settings->value(it.key(), it.value()));
    settings->endGroup();

    // apply the values to our member variables
    fromMap(map);
}

void AnalyzerGlobalSettings::writeSettings() const
{
    QSettings *settings = Core::ICore::instance()->settings();
    settings->beginGroup(QLatin1String(groupC));
    const QVariantMap map = toMap();
    for (QVariantMap::ConstIterator it = map.begin(); it != map.end(); ++it)
        settings->setValue(it.key(), it.value());
    settings->endGroup();
}

void AnalyzerGlobalSettings::registerSubConfigs
    (AnalyzerSubConfigFactory globalCreator, AnalyzerSubConfigFactory projectCreator)
{
    m_projectSubConfigs.append(projectCreator);

    AbstractAnalyzerSubConfig *config = globalCreator();
    config->setParent(this);
    AnalyzerPlugin::instance()->addAutoReleasedObject(new AnalyzerOptionsPage(config));

    readSettings();
}

QList<AnalyzerSubConfigFactory> AnalyzerGlobalSettings::projectSubConfigs() const
{
    return m_projectSubConfigs;
}

AnalyzerProjectSettings::AnalyzerProjectSettings(QObject *parent)
    : AnalyzerSettings(parent)
{
    // add sub configs
    foreach (AnalyzerSubConfigFactory factory, AnalyzerGlobalSettings::instance()->projectSubConfigs())
        factory()->setParent(this);

    // take defaults from global settings
    AnalyzerGlobalSettings *gs = AnalyzerGlobalSettings::instance();
    fromMap(gs->toMap());
}

QString AnalyzerProjectSettings::displayName() const
{
    return tr("Analyzer Settings");
}

bool AnalyzerProjectSettings::fromMap(const QVariantMap &map)
{
    return AnalyzerSettings::fromMap(map);
}

QVariantMap AnalyzerProjectSettings::toMap() const
{
    return AnalyzerSettings::toMap();
}

} // namespace Analyzer
