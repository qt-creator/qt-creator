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

#include "artisticstylesettings.h"

#include "artisticstyleconstants.h"

#include "../beautifierconstants.h"

#include <coreplugin/icore.h>

#include <utils/runextensions.h>
#include <utils/synchronousprocess.h>

#include <QDateTime>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QXmlStreamWriter>

namespace Beautifier {
namespace Internal {
namespace ArtisticStyle {

namespace {
const char USE_OTHER_FILES[]          = "useOtherFiles";
const char USE_SPECIFIC_CONFIG_FILE[] = "useSpecificConfigFile";
const char SPECIFIC_CONFIG_FILE[]     = "specificConfigFile";
const char USE_HOME_FILE[]            = "useHomeFile";
const char USE_CUSTOM_STYLE[]         = "useCustomStyle";
const char CUSTOM_STYLE[]             = "customStyle";
}

ArtisticStyleSettings::ArtisticStyleSettings() :
    AbstractSettings(Constants::ArtisticStyle::SETTINGS_NAME, ".astyle")
{
    connect(&m_versionWatcher, &QFutureWatcherBase::finished,
            this, &ArtisticStyleSettings::helperSetVersion);

    setCommand("astyle");
    m_settings.insert(USE_OTHER_FILES, QVariant(true));
    m_settings.insert(USE_SPECIFIC_CONFIG_FILE, QVariant(false));
    m_settings.insert(SPECIFIC_CONFIG_FILE, QVariant());
    m_settings.insert(USE_HOME_FILE, QVariant(false));
    m_settings.insert(USE_CUSTOM_STYLE, QVariant(false));
    m_settings.insert(CUSTOM_STYLE, QVariant());
    read();
}

static int parseVersion(const QString &text)
{
    // The version in Artistic Style is printed like "Artistic Style Version 2.04"
    const QRegularExpression rx("([2-9]{1})\\.([0-9]{2})(\\.[1-9]{1})?$");
    const QRegularExpressionMatch match = rx.match(text);
    if (match.hasMatch()) {
        const int major = match.capturedRef(1).toInt() * 100;
        const int minor = match.capturedRef(2).toInt();
        return major + minor;
    }
    return 0;
}

static int updateVersionHelper(const QString &command)
{
    Utils::SynchronousProcess process;
    Utils::SynchronousProcessResponse response
            = process.runBlocking(command, QStringList("--version"));
    if (response.result != Utils::SynchronousProcessResponse::Finished)
        return 0;

    // Astyle prints the version on stdout or stderr, depending on platform
    const int version = parseVersion(response.stdOut().trimmed());
    if (version != 0)
        return version;
    return parseVersion(response.stdErr().trimmed());
}

void ArtisticStyleSettings::updateVersion()
{
    if (m_versionFuture.isRunning())
        m_versionFuture.cancel();

    m_versionFuture = Utils::runAsync(updateVersionHelper, command());
    m_versionWatcher.setFuture(m_versionFuture);
}

void ArtisticStyleSettings::helperSetVersion()
{
    m_version = m_versionWatcher.result();
}

bool ArtisticStyleSettings::useOtherFiles() const
{
    return m_settings.value(USE_OTHER_FILES).toBool();
}

void ArtisticStyleSettings::setUseOtherFiles(bool useOtherFiles)
{
    m_settings.insert(USE_OTHER_FILES, QVariant(useOtherFiles));
}

bool ArtisticStyleSettings::useSpecificConfigFile() const
{
    return m_settings.value(USE_SPECIFIC_CONFIG_FILE).toBool();
}

void ArtisticStyleSettings::setUseSpecificConfigFile(bool useSpecificConfigFile)
{
    m_settings.insert(USE_SPECIFIC_CONFIG_FILE, QVariant(useSpecificConfigFile));
}

Utils::FileName ArtisticStyleSettings::specificConfigFile() const
{
    return Utils::FileName::fromString(m_settings.value(SPECIFIC_CONFIG_FILE).toString());
}

void ArtisticStyleSettings::setSpecificConfigFile(const Utils::FileName &specificConfigFile)
{
    m_settings.insert(SPECIFIC_CONFIG_FILE, QVariant(specificConfigFile.toString()));
}

bool ArtisticStyleSettings::useHomeFile() const
{
    return m_settings.value(USE_HOME_FILE).toBool();
}

void ArtisticStyleSettings::setUseHomeFile(bool useHomeFile)
{
    m_settings.insert(USE_HOME_FILE, QVariant(useHomeFile));
}

bool ArtisticStyleSettings::useCustomStyle() const
{
    return m_settings.value(USE_CUSTOM_STYLE).toBool();
}

void ArtisticStyleSettings::setUseCustomStyle(bool useCustomStyle)
{
    m_settings.insert(USE_CUSTOM_STYLE, QVariant(useCustomStyle));
}

QString ArtisticStyleSettings::customStyle() const
{
    return m_settings.value(CUSTOM_STYLE).toString();
}

void ArtisticStyleSettings::setCustomStyle(const QString &customStyle)
{
    m_settings.insert(CUSTOM_STYLE, QVariant(customStyle));
}

QString ArtisticStyleSettings::documentationFilePath() const
{
    return Core::ICore::userResourcePath() + '/' + Beautifier::Constants::SETTINGS_DIRNAME + '/'
            + Beautifier::Constants::DOCUMENTATION_DIRNAME + '/'
            + Constants::ArtisticStyle::SETTINGS_NAME + ".xml";
}

void ArtisticStyleSettings::createDocumentationFile() const
{
    Utils::SynchronousProcess process;
    process.setTimeoutS(2);
    Utils::SynchronousProcessResponse response
            = process.runBlocking(command(), QStringList("-h"));
    if (response.result != Utils::SynchronousProcessResponse::Finished)
        return;

    QFile file(documentationFilePath());
    const QFileInfo fi(file);
    if (!fi.exists())
        fi.dir().mkpath(fi.absolutePath());
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text))
        return;

    bool contextWritten = false;
    QXmlStreamWriter stream(&file);
    stream.setAutoFormatting(true);
    stream.writeStartDocument("1.0", true);
    stream.writeComment("Created " + QDateTime::currentDateTime().toString(Qt::ISODate));
    stream.writeStartElement(Constants::DOCUMENTATION_XMLROOT);

    // astyle writes its output to 'error'...
    const QStringList lines = response.stdErr().split(QLatin1Char('\n'));
    QStringList keys;
    QStringList docu;
    for (QString line : lines) {
        line = line.trimmed();
        if ((line.startsWith("--") && !line.startsWith("---")) || line.startsWith("OR ")) {
            const QStringList rawKeys = line.split(" OR ", QString::SkipEmptyParts);
            for (QString k : rawKeys) {
                k = k.trimmed();
                k.remove('#');
                keys << k;
                if (k.startsWith("--"))
                    keys << k.right(k.size() - 2);
            }
        } else {
            if (line.isEmpty()) {
                if (!keys.isEmpty()) {
                    // Write entry
                    stream.writeStartElement(Constants::DOCUMENTATION_XMLENTRY);
                    stream.writeStartElement(Constants::DOCUMENTATION_XMLKEYS);
                    for (const QString &key : keys)
                        stream.writeTextElement(Constants::DOCUMENTATION_XMLKEY, key);
                    stream.writeEndElement();
                    const QString text = "<p><span class=\"option\">"
                            + keys.filter(QRegularExpression("^\\-")).join(", ") + "</span></p><p>"
                            + (docu.join(' ').toHtmlEscaped()) + "</p>";
                    stream.writeTextElement(Constants::DOCUMENTATION_XMLDOC, text);
                    stream.writeEndElement();
                    contextWritten = true;
                }
                keys.clear();
                docu.clear();
            } else if (!keys.isEmpty()) {
                docu << line;
            }
        }
    }

    stream.writeEndElement();
    stream.writeEndDocument();

    // An empty file causes error messages and a contextless file preventing this function to run
    // again in order to generate the documentation successfully. Thus delete the file.
    if (!contextWritten) {
        file.close();
        file.remove();
    }
}

} // namespace ArtisticStyle
} // namespace Internal
} // namespace Beautifier
