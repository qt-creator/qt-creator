/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
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

#include "qmljsinspectorsettings.h"
#include "qmljsinspectorconstants.h"
#include <QtCore/QSettings>

namespace QmlJSInspector {
namespace Internal {

InspectorSettings::InspectorSettings(QObject *parent)
    : QObject(parent),
      m_showLivePreviewWarning(true)
{
}

void InspectorSettings::restoreSettings(QSettings *settings)
{
    settings->beginGroup(QLatin1String(QmlJSInspector::Constants::S_QML_INSPECTOR));
    m_showLivePreviewWarning = settings->value(QLatin1String(QmlJSInspector::Constants::S_LIVE_PREVIEW_WARNING_KEY), true).toBool();
    settings->endGroup();
}

void InspectorSettings::saveSettings(QSettings *settings) const
{
    settings->beginGroup(QLatin1String(QmlJSInspector::Constants::S_QML_INSPECTOR));
    settings->setValue(QLatin1String(QmlJSInspector::Constants::S_LIVE_PREVIEW_WARNING_KEY), m_showLivePreviewWarning);
    settings->endGroup();
}

bool InspectorSettings::showLivePreviewWarning() const
{
    return m_showLivePreviewWarning;
}

void InspectorSettings::setShowLivePreviewWarning(bool value)
{
    m_showLivePreviewWarning = value;
}

} // namespace Internal
} // namespace QmlJSInspector
