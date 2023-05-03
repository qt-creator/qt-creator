// Copyright (C) 2016 Lorenz Haas
// Copyright (C) 2022 Xavier BESSON
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cmakeformattersettings.h"
#include "cmakeprojectconstants.h"

#include <coreplugin/icore.h>
#include <coreplugin/idocument.h>
#include <utils/algorithm.h>
#include <utils/genericconstants.h>
#include <utils/mimeutils.h>
#include <utils/process.h>

namespace CMakeProjectManager {
namespace Internal {

namespace {
const char AUTO_FORMAT_COMMAND[] = "autoFormatCommand";
const char AUTO_FORMAT_MIME[] = "autoFormatMime";
const char AUTO_FORMAT_ONLY_CURRENT_PROJECT[] = "autoFormatOnlyCurrentProject";
const char AUTO_FORMAT_ON_SAVE[] = "autoFormatOnSave";
}

CMakeFormatterSettings::CMakeFormatterSettings(QObject* parent)
    : QObject(parent)
{
    read();
}

CMakeFormatterSettings *CMakeFormatterSettings::instance()
{
    static CMakeFormatterSettings m_instance;
    return &m_instance;
}

void CMakeFormatterSettings::read()
{
    QSettings *s = Core::ICore::settings();
    s->beginGroup(Constants::CMAKEFORMATTER_SETTINGS_GROUP);
    s->beginGroup(Constants::CMAKEFORMATTER_GENERAL_GROUP);
    setCommand(s->value(AUTO_FORMAT_COMMAND, QString("cmake-format")).toString());
    m_autoFormatOnSave = s->value(AUTO_FORMAT_ON_SAVE, false).toBool();
    setAutoFormatMime(s->value(AUTO_FORMAT_MIME, QString("text/x-cmake")).toString());
    m_autoFormatOnlyCurrentProject = s->value(AUTO_FORMAT_ONLY_CURRENT_PROJECT, true).toBool();
    s->endGroup();
    s->endGroup();
}

void CMakeFormatterSettings::save()
{
    QSettings *s = Core::ICore::settings();
    s->beginGroup(Constants::CMAKEFORMATTER_SETTINGS_GROUP);
    s->beginGroup(Constants::CMAKEFORMATTER_GENERAL_GROUP);
    Utils::QtcSettings::setValueWithDefault(s, AUTO_FORMAT_COMMAND, m_command, QString("cmake-format"));
    Utils::QtcSettings::setValueWithDefault(s, AUTO_FORMAT_ON_SAVE, m_autoFormatOnSave, false);
    Utils::QtcSettings::setValueWithDefault(s, AUTO_FORMAT_MIME, autoFormatMimeAsString(), QString("text/x-cmake"));
    Utils::QtcSettings::setValueWithDefault(s, AUTO_FORMAT_ONLY_CURRENT_PROJECT, m_autoFormatOnlyCurrentProject, true);
    s->endGroup();
    s->endGroup();
}

Utils::FilePath CMakeFormatterSettings::command() const
{
    return Utils::FilePath::fromString(m_command);
}

void CMakeFormatterSettings::setCommand(const QString &cmd)
{
    if (cmd == m_command)
        return;

    m_command = cmd;
}

bool CMakeFormatterSettings::autoFormatOnSave() const
{
    return m_autoFormatOnSave;
}

void CMakeFormatterSettings::setAutoFormatOnSave(bool autoFormatOnSave)
{
    m_autoFormatOnSave = autoFormatOnSave;
}

QStringList CMakeFormatterSettings::autoFormatMime() const
{
    return m_autoFormatMime;
}

QString CMakeFormatterSettings::autoFormatMimeAsString() const
{
    return m_autoFormatMime.join("; ");
}

void CMakeFormatterSettings::setAutoFormatMime(const QStringList &autoFormatMime)
{
    if (m_autoFormatMime == autoFormatMime)
        return;

    m_autoFormatMime = autoFormatMime;
    emit supportedMimeTypesChanged();
}

void CMakeFormatterSettings::setAutoFormatMime(const QString &mimeList)
{
    setAutoFormatMime(mimeList.split(';'));
}

bool CMakeFormatterSettings::autoFormatOnlyCurrentProject() const
{
    return m_autoFormatOnlyCurrentProject;
}

void CMakeFormatterSettings::setAutoFormatOnlyCurrentProject(bool autoFormatOnlyCurrentProject)
{
    m_autoFormatOnlyCurrentProject = autoFormatOnlyCurrentProject;
}

bool CMakeFormatterSettings::isApplicable(const Core::IDocument *document) const
{
    if (!document)
        return false;

    if (m_autoFormatMime.isEmpty())
        return true;

    const Utils::MimeType documentMimeType = Utils::mimeTypeForName(document->mimeType());
    return Utils::anyOf(m_autoFormatMime, [&documentMimeType](const QString &mime) {
        return documentMimeType.inherits(mime);
    });
}

} // namespace Internal
} // namespace CMakeProjectManager
