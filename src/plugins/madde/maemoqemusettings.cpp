/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "maemoqemusettings.h"

#include <coreplugin/icore.h>

#include <QSettings>

namespace Madde {
namespace Internal {
namespace {
const QString SettingsGroup = QLatin1String("Maemo Qemu Settings");
const QString OpenGlModeKey = QLatin1String("OpenGl Mode");
} // anonymous namespace

MaemoQemuSettings::OpenGlMode MaemoQemuSettings::openGlMode()
{
    if (!m_initialized) {
        QSettings *settings = Core::ICore::settings();
        settings->beginGroup(SettingsGroup);
        m_openGlMode = static_cast<OpenGlMode>(settings->value(OpenGlModeKey, AutoDetect).toInt());
        settings->endGroup();
        m_initialized = true;
    }
    return m_openGlMode;
}

void MaemoQemuSettings::setOpenGlMode(OpenGlMode openGlMode)
{
    Q_ASSERT(m_initialized);
    m_openGlMode = openGlMode;
    QSettings *settings = Core::ICore::settings();
    settings->beginGroup(SettingsGroup);
    settings->setValue(OpenGlModeKey, m_openGlMode);
    settings->endGroup();
}

bool MaemoQemuSettings::m_initialized = false;
MaemoQemuSettings::OpenGlMode MaemoQemuSettings::m_openGlMode;

} // namespace Internal
} // namespace Madde
