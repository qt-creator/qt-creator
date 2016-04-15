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

#include "abstractsettings.h"

#include "beautifierconstants.h"
#include "beautifierplugin.h"

#include <coreplugin/icore.h>
#include <utils/fileutils.h>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QXmlStreamReader>

namespace Beautifier {
namespace Internal {

AbstractSettings::AbstractSettings(const QString &name, const QString &ending)
    : m_version(0)
    , m_ending(ending)
    , m_styleDir(Core::ICore::userResourcePath() + QLatin1Char('/')
                 + QLatin1String(Beautifier::Constants::SETTINGS_DIRNAME) + QLatin1Char('/')
                 + name)
    , m_name(name)
{
}

AbstractSettings::~AbstractSettings()
{
}

QStringList AbstractSettings::completerWords()
{
    return QStringList();
}

QStringList AbstractSettings::styles() const
{
    QStringList list = m_styles.keys();
    list.sort(Qt::CaseInsensitive);
    return list;
}

QString AbstractSettings::style(const QString &key) const
{
    return m_styles.value(key);
}

bool AbstractSettings::styleExists(const QString &key) const
{
    return m_styles.contains(key);
}

bool AbstractSettings::styleIsReadOnly(const QString &key)
{
    const QFileInfo fi(m_styleDir.absoluteFilePath(key + m_ending));
    if (!fi.exists()) {
        // newly added style which was not saved yet., thus it is not read only.
        //TODO In a later version when we have predefined styles in Core::ICore::resourcePath()
        //     we need to check if it is a read only global config file...
        return false;
    }

    return !fi.isWritable();
}

void AbstractSettings::setStyle(const QString &key, const QString &value)
{
    m_styles.insert(key, value);
    m_changedStyles.insert(key);
}

void AbstractSettings::removeStyle(const QString &key)
{
    m_styles.remove(key);
    m_stylesToRemove << key;
}

void AbstractSettings::replaceStyle(const QString &oldKey, const QString &newKey,
                                    const QString &value)
{
    // Set value regardles if keys are equal
    m_styles.insert(newKey, value);

    if (oldKey != newKey)
        removeStyle(oldKey);

    m_changedStyles.insert(newKey);
}

QString AbstractSettings::styleFileName(const QString &key) const
{
    return m_styleDir.absoluteFilePath(key + m_ending);
}

QString AbstractSettings::command() const
{
    return m_command;
}

void AbstractSettings::setCommand(const QString &command)
{
    if (command == m_command)
        return;

    m_command = command;
    updateVersion();
}

int AbstractSettings::version() const
{
    return m_version;
}

void AbstractSettings::updateVersion()
{
    // If a beautifier needs to know the current tool's version, reimplement and store the version
    // in m_version.
}

QStringList AbstractSettings::options()
{
    if (m_options.isEmpty())
        readDocumentation();

    return m_options.keys();
}

QString AbstractSettings::documentation(const QString &option) const
{
    const int index = m_options.value(option, -1);
    if (index != -1)
        return m_docu.at(index);
    else
        return QString();
}

void AbstractSettings::save()
{
    // Save settings, except styles
    QSettings *s = Core::ICore::settings();
    s->beginGroup(QLatin1String(Constants::SETTINGS_GROUP));
    s->beginGroup(m_name);
    QMap<QString, QVariant>::const_iterator iSettings = m_settings.constBegin();
    while (iSettings != m_settings.constEnd()) {
        s->setValue(iSettings.key(), iSettings.value());
        ++iSettings;
    }
    s->setValue(QLatin1String("command"), m_command);
    s->endGroup();
    s->endGroup();

    // Save styles
    if (m_stylesToRemove.isEmpty() && m_styles.isEmpty())
        return;

    // remove old files and possible subfolder
    foreach (const QString &key, m_stylesToRemove) {
        const QFileInfo fi(styleFileName(key));
        QFile::remove(fi.absoluteFilePath());
        if (fi.absoluteDir() != m_styleDir)
            m_styleDir.rmdir(fi.absolutePath());
    }
    m_stylesToRemove.clear();

    QMap<QString, QString>::const_iterator iStyles = m_styles.constBegin();
    while (iStyles != m_styles.constEnd()) {
        // Only save changed styles.
        if (!m_changedStyles.contains(iStyles.key())) {
            ++iStyles;
            continue;
        }

        const QFileInfo fi(styleFileName(iStyles.key()));
        if (!(m_styleDir.mkpath(fi.absolutePath()))) {
            BeautifierPlugin::showError(tr("Cannot save styles. %1 does not exist.")
                                        .arg(fi.absolutePath()));
            continue;
        }

        Utils::FileSaver saver(fi.absoluteFilePath());
        if (saver.hasError()) {
            BeautifierPlugin::showError(tr("Cannot open file \"%1\": %2.")
                                        .arg(saver.fileName())
                                        .arg(saver.errorString()));
        } else {
            saver.write(iStyles.value().toLocal8Bit());
            if (!saver.finalize()) {
                BeautifierPlugin::showError(tr("Cannot save file \"%1\": %2.")
                                            .arg(saver.fileName())
                                            .arg(saver.errorString()));
            }
        }
        ++iStyles;
    }

    m_changedStyles.clear();
}

void AbstractSettings::createDocumentationFile() const
{
    // Could be reimplemented to create a documentation file.
}

void AbstractSettings::read()
{
    // Read settings, except styles
    QSettings *s = Core::ICore::settings();
    s->beginGroup(QLatin1String(Constants::SETTINGS_GROUP));
    s->beginGroup(m_name);
    const QStringList keys = s->allKeys();
    foreach (const QString &key, keys) {
        if (key == QLatin1String("command"))
            setCommand(s->value(key).toString());
        else if (m_settings.contains(key))
            m_settings[key] = s->value(key);
        else
            s->remove(key);
    }
    s->endGroup();
    s->endGroup();

    m_styles.clear();
    m_changedStyles.clear();
    m_stylesToRemove.clear();
    readStyles();
}

void AbstractSettings::readDocumentation()
{
    const QString filename = documentationFilePath();
    if (filename.isEmpty()) {
        BeautifierPlugin::showError(tr("No documentation file specified."));
        return;
    }

    QFile file(filename);
    if (!file.exists())
        createDocumentationFile();

    if (!file.open(QIODevice::ReadOnly)) {
        BeautifierPlugin::showError(tr("Cannot open documentation file \"%1\".").arg(filename));
        return;
    }

    QXmlStreamReader xml(&file);
    if (!xml.readNextStartElement())
        return;
    if (xml.name() != QLatin1String(Constants::DOCUMENTATION_XMLROOT)) {
        BeautifierPlugin::showError(tr("The file \"%1\" is not a valid documentation file.")
                                    .arg(filename));
        return;
    }

    // We do not use QHash<QString, QString> since e.g. in Artistic Style there are many different
    // keys with the same (long) documentation text. By splitting the keys from the documentation
    // text we save a huge amount of memory.
    m_options.clear();
    m_docu.clear();
    QStringList keys;
    while (!(xml.atEnd() || xml.hasError())) {
        if (xml.readNext() == QXmlStreamReader::StartElement) {
            const QStringRef &name = xml.name();
            if (name == QLatin1String(Constants::DOCUMENTATION_XMLENTRY)) {
                keys.clear();
            } else if (name == QLatin1String(Constants::DOCUMENTATION_XMLKEY)) {
                if (xml.readNext() == QXmlStreamReader::Characters)
                    keys << xml.text().toString();
            } else if (name == QLatin1String(Constants::DOCUMENTATION_XMLDOC)) {
                if (xml.readNext() == QXmlStreamReader::Characters) {
                    m_docu << xml.text().toString();
                    const int index = m_docu.size() - 1;
                    foreach (const QString &key, keys)
                        m_options.insert(key, index);
                }
            }
        }
    }

    if (xml.hasError()) {
        BeautifierPlugin::showError(tr("Cannot read documentation file \"%1\": %2.")
                                    .arg(filename).arg(xml.errorString()));
    }
}

void AbstractSettings::readStyles()
{
    if (!m_styleDir.exists())
        return;

    const QStringList files
            = m_styleDir.entryList(QStringList() << QLatin1Char('*') + m_ending,
                                   QDir::Files | QDir::Readable | QDir::NoDotAndDotDot);
    foreach (const QString &filename, files) {
        // do not allow empty file names
        if (filename == m_ending)
            continue;

        QFile file(m_styleDir.absoluteFilePath(filename));
        if (file.open(QIODevice::ReadOnly)) {
            m_styles.insert(filename.left(filename.length() - m_ending.length()),
                            QString::fromLocal8Bit(file.readAll()));
        }
    }
}

} // namespace Internal
} // namespace Beautifier
