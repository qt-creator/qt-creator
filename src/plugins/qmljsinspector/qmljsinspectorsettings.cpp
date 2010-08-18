/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
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

InspectorSettings::~InspectorSettings()
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
