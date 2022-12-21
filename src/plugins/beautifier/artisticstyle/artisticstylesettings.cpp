// Copyright (C) 2016 Lorenz Haas
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "artisticstylesettings.h"

#include "artisticstyleconstants.h"

#include "../beautifierconstants.h"

#include <coreplugin/icore.h>

#include <utils/runextensions.h>
#include <utils/stringutils.h>
#include <utils/qtcprocess.h>

#include <QDateTime>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QXmlStreamWriter>

using namespace Utils;

namespace Beautifier {
namespace Internal {

const char USE_OTHER_FILES[]          = "useOtherFiles";
const char USE_SPECIFIC_CONFIG_FILE[] = "useSpecificConfigFile";
const char SPECIFIC_CONFIG_FILE[]     = "specificConfigFile";
const char USE_HOME_FILE[]            = "useHomeFile";
const char USE_CUSTOM_STYLE[]         = "useCustomStyle";
const char CUSTOM_STYLE[]             = "customStyle";
const char SETTINGS_NAME[]            = "artisticstyle";

ArtisticStyleSettings::ArtisticStyleSettings() :
    AbstractSettings(SETTINGS_NAME, ".astyle")
{
    setVersionRegExp(QRegularExpression("([2-9]{1})\\.([0-9]{1,2})(\\.[1-9]{1})?$"));
    setCommand("astyle");
    m_settings.insert(USE_OTHER_FILES, QVariant(true));
    m_settings.insert(USE_SPECIFIC_CONFIG_FILE, QVariant(false));
    m_settings.insert(SPECIFIC_CONFIG_FILE, QVariant());
    m_settings.insert(USE_HOME_FILE, QVariant(false));
    m_settings.insert(USE_CUSTOM_STYLE, QVariant(false));
    m_settings.insert(CUSTOM_STYLE, QVariant());
    read();
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

FilePath ArtisticStyleSettings::specificConfigFile() const
{
    return FilePath::fromString(m_settings.value(SPECIFIC_CONFIG_FILE).toString());
}

void ArtisticStyleSettings::setSpecificConfigFile(const FilePath &specificConfigFile)
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
    return (Core::ICore::userResourcePath(Beautifier::Constants::SETTINGS_DIRNAME)
                / Beautifier::Constants::DOCUMENTATION_DIRNAME / SETTINGS_NAME)
            .stringAppended(".xml")
        .toString();
}

void ArtisticStyleSettings::createDocumentationFile() const
{
    QtcProcess process;
    process.setTimeoutS(2);
    process.setCommand({command(), {"-h"}});
    process.runBlocking();
    if (process.result() != ProcessResult::FinishedWithSuccess)
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
    const QStringList lines = process.cleanedStdErr().split(QLatin1Char('\n'));
    QStringList keys;
    QStringList docu;
    for (QString line : lines) {
        line = line.trimmed();
        if ((line.startsWith("--") && !line.startsWith("---")) || line.startsWith("OR ")) {
            const QStringList rawKeys = line.split(" OR ", Qt::SkipEmptyParts);
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
                    for (const QString &key : std::as_const(keys))
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

} // namespace Internal
} // namespace Beautifier
