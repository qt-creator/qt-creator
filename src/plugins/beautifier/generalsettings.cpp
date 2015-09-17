/****************************************************************************
**
** Copyright (C) 2016 Lorenz Haas
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

#include "generalsettings.h"

#include "beautifierconstants.h"

#include <coreplugin/icore.h>
#include <utils/mimetypes/mimedatabase.h>

namespace Beautifier {
namespace Internal {

namespace {
const char GROUP[]                            = "General";
const char AUTO_FORMAT_ON_SAVE[]              = "autoFormatOnSave";
const char AUTO_FORMAT_TOOL[]                 = "autoFormatTool";
const char AUTO_FORMAT_MIME[]                 = "autoFormatMime";
const char AUTO_FORMAT_ONLY_CURRENT_PROJECT[] = "autoFormatOnlyCurrentProject";
}

GeneralSettings::GeneralSettings()
{
    read();
}

void GeneralSettings::read()
{
    QSettings *s = Core::ICore::settings();
    s->beginGroup(Constants::SETTINGS_GROUP);
    s->beginGroup(GROUP);
    m_autoFormatOnSave = s->value(AUTO_FORMAT_ON_SAVE, false).toBool();
    m_autoFormatTool = s->value(AUTO_FORMAT_TOOL, QString()).toString();
    setAutoFormatMime(s->value(AUTO_FORMAT_MIME, "text/x-c++src;text/x-c++hdr").toString());
    m_autoFormatOnlyCurrentProject = s->value(AUTO_FORMAT_ONLY_CURRENT_PROJECT, true).toBool();
    s->endGroup();
    s->endGroup();
}

void GeneralSettings::save()
{
    QSettings *s = Core::ICore::settings();
    s->beginGroup(Constants::SETTINGS_GROUP);
    s->beginGroup(GROUP);
    s->setValue(AUTO_FORMAT_ON_SAVE, m_autoFormatOnSave);
    s->setValue(AUTO_FORMAT_TOOL, m_autoFormatTool);
    s->setValue(AUTO_FORMAT_MIME, autoFormatMimeAsString());
    s->setValue(AUTO_FORMAT_ONLY_CURRENT_PROJECT, m_autoFormatOnlyCurrentProject);
    s->endGroup();
    s->endGroup();
}

bool GeneralSettings::autoFormatOnSave() const
{
    return m_autoFormatOnSave;
}

void GeneralSettings::setAutoFormatOnSave(bool autoFormatOnSave)
{
    m_autoFormatOnSave = autoFormatOnSave;
}

QString GeneralSettings::autoFormatTool() const
{
    return m_autoFormatTool;
}

void GeneralSettings::setAutoFormatTool(const QString &autoFormatTool)
{
    m_autoFormatTool = autoFormatTool;
}

QList<Utils::MimeType> GeneralSettings::autoFormatMime() const
{
    return m_autoFormatMime;
}

QString GeneralSettings::autoFormatMimeAsString() const
{
    QStringList types;
    types.reserve(m_autoFormatMime.count());
    for (auto t : m_autoFormatMime)
        types << t.name();
    return types.join("; ");
}

void GeneralSettings::setAutoFormatMime(const QList<Utils::MimeType> &autoFormatMime)
{
    m_autoFormatMime = autoFormatMime;
}

void GeneralSettings::setAutoFormatMime(const QString &mimeList)
{
    const QStringList stringTypes = mimeList.split(';');
    QList<Utils::MimeType> types;
    types.reserve(stringTypes.count());
    const Utils::MimeDatabase mdb;
    for (QString t : stringTypes) {
        t = t.trimmed();
        const Utils::MimeType mime = mdb.mimeTypeForName(t);
        if (mime.isValid())
            types << mime;
    }
    setAutoFormatMime(types);
}

bool GeneralSettings::autoFormatOnlyCurrentProject() const
{
    return m_autoFormatOnlyCurrentProject;
}

void GeneralSettings::setAutoFormatOnlyCurrentProject(bool autoFormatOnlyCurrentProject)
{
    m_autoFormatOnlyCurrentProject = autoFormatOnlyCurrentProject;
}

} // namespace Internal
} // namespace Beautifier
