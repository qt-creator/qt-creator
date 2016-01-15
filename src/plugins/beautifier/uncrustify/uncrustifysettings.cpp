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

#include "uncrustifysettings.h"

#include "uncrustifyconstants.h"

#include "../beautifierconstants.h"

#include <coreplugin/icore.h>

#include <QDateTime>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QXmlStreamWriter>

namespace Beautifier {
namespace Internal {
namespace Uncrustify {

namespace {
const QString kUseOtherFiles = QLatin1String("useOtherFiles");
const QString kUseHomeFile = QLatin1String("useHomeFile");
const QString kUseCustomStyle = QLatin1String("useCustomStyle");
const QString kCustomStyle = QLatin1String("customStyle");
const QString kFormatEntireFileFallback = QLatin1String("formatEntireFileFallback");
}

UncrustifySettings::UncrustifySettings() :
    AbstractSettings(QLatin1String(Constants::Uncrustify::SETTINGS_NAME), QLatin1String(".cfg"))
{
    setCommand(QLatin1String("uncrustify"));
    m_settings.insert(kUseOtherFiles, QVariant(true));
    m_settings.insert(kUseHomeFile, QVariant(false));
    m_settings.insert(kUseCustomStyle, QVariant(false));
    m_settings.insert(kCustomStyle, QVariant());
    m_settings.insert(kFormatEntireFileFallback, QVariant(true));
    read();
}

bool UncrustifySettings::useOtherFiles() const
{
    return m_settings.value(kUseOtherFiles).toBool();
}

void UncrustifySettings::setUseOtherFiles(bool useOtherFiles)
{
    m_settings.insert(kUseOtherFiles, QVariant(useOtherFiles));
}

bool UncrustifySettings::useHomeFile() const
{
    return m_settings.value(kUseHomeFile).toBool();
}

void UncrustifySettings::setUseHomeFile(bool useHomeFile)
{
    m_settings.insert(kUseHomeFile, QVariant(useHomeFile));
}

bool UncrustifySettings::useCustomStyle() const
{
    return m_settings.value(kUseCustomStyle).toBool();
}

void UncrustifySettings::setUseCustomStyle(bool useCustomStyle)
{
    m_settings.insert(kUseCustomStyle, QVariant(useCustomStyle));
}

QString UncrustifySettings::customStyle() const
{
    return m_settings.value(kCustomStyle).toString();
}

void UncrustifySettings::setCustomStyle(const QString &customStyle)
{
    m_settings.insert(kCustomStyle, QVariant(customStyle));
}

bool UncrustifySettings::formatEntireFileFallback() const
{
    return m_settings.value(kFormatEntireFileFallback).toBool();
}

void UncrustifySettings::setFormatEntireFileFallback(bool formatEntireFileFallback)
{
    m_settings.insert(kFormatEntireFileFallback, QVariant(formatEntireFileFallback));
}

QString UncrustifySettings::documentationFilePath() const
{
    return Core::ICore::userResourcePath() + QLatin1Char('/')
            + QLatin1String(Beautifier::Constants::SETTINGS_DIRNAME) + QLatin1Char('/')
            + QLatin1String(Beautifier::Constants::DOCUMENTATION_DIRNAME) + QLatin1Char('/')
            + QLatin1String(Constants::Uncrustify::SETTINGS_NAME) + QLatin1String(".xml");
}

void UncrustifySettings::createDocumentationFile() const
{
    QProcess process;
    process.start(command(), QStringList() << QLatin1String("--show-config"));
    process.waitForFinished(2000); // show config should be really fast.
    if (process.error() != QProcess::UnknownError)
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
    stream.writeStartDocument(QLatin1String("1.0"), true);
    stream.writeComment(QLatin1String("Created ")
                        + QDateTime::currentDateTime().toString(Qt::ISODate));
    stream.writeStartElement(QLatin1String(Constants::DOCUMENTATION_XMLROOT));

    const QStringList lines = QString::fromUtf8(process.readAll()).split(QLatin1Char('\n'));
    const int totalLines = lines.count();
    for (int i = 0; i < totalLines; ++i) {
        const QString &line = lines.at(i);
        if (line.startsWith(QLatin1Char('#')) || line.trimmed().isEmpty())
            continue;

        const int firstSpace = line.indexOf(QLatin1Char(' '));
        const QString keyword = line.left(firstSpace);
        const QString options = line.right(line.size() - firstSpace).trimmed();
        QStringList docu;
        while (++i < totalLines) {
            const QString &subline = lines.at(i);
            if (line.startsWith(QLatin1Char('#')) || subline.trimmed().isEmpty()) {
                const QString text = QLatin1String("<p><span class=\"option\">") + keyword
                        + QLatin1String("</span> <span class=\"param\">") + options
                        + QLatin1String("</span></p><p>")
                        + (docu.join(QLatin1Char(' ')).toHtmlEscaped())
                        + QLatin1String("</p>");
                stream.writeStartElement(QLatin1String(Constants::DOCUMENTATION_XMLENTRY));
                stream.writeTextElement(QLatin1String(Constants::DOCUMENTATION_XMLKEY), keyword);
                stream.writeTextElement(QLatin1String(Constants::DOCUMENTATION_XMLDOC), text);
                stream.writeEndElement();
                contextWritten = true;
                break;
            } else {
                docu << subline;
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

} // namespace Uncrustify
} // namespace Internal
} // namespace Beautifier
