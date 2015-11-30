/**************************************************************************
**
** Copyright (C) 2015 Lorenz Haas
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "artisticstylesettings.h"

#include "artisticstyleconstants.h"

#include "../beautifierconstants.h"

#include <coreplugin/icore.h>

#include <utils/QtConcurrentTools>

#include <QDateTime>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QXmlStreamWriter>

namespace Beautifier {
namespace Internal {
namespace ArtisticStyle {

namespace {
const char kUseOtherFiles[] = "useOtherFiles";
const char kUseHomeFile[] = "useHomeFile";
const char kUseCustomStyle[] = "useCustomStyle";
const char kCustomStyle[] = "customStyle";
}

ArtisticStyleSettings::ArtisticStyleSettings() :
    AbstractSettings(QLatin1String(Constants::ArtisticStyle::SETTINGS_NAME),
                     QLatin1String(".astyle"))
{
    connect(&m_versionWatcher, &QFutureWatcherBase::finished,
            this, &ArtisticStyleSettings::helperSetVersion);

    setCommand(QLatin1String("astyle"));
    m_settings.insert(QLatin1String(kUseOtherFiles), QVariant(true));
    m_settings.insert(QLatin1String(kUseHomeFile), QVariant(false));
    m_settings.insert(QLatin1String(kUseCustomStyle), QVariant(false));
    m_settings.insert(QLatin1String(kCustomStyle), QVariant());
    read();
}

void ArtisticStyleSettings::updateVersion()
{
    if (m_versionFuture.isRunning())
        m_versionFuture.cancel();

    m_versionFuture = QtConcurrent::run(&ArtisticStyleSettings::helperUpdateVersion, this);
    m_versionWatcher.setFuture(m_versionFuture);
}

void ArtisticStyleSettings::helperUpdateVersion(QFutureInterface<int> &future)
{
    QProcess process;
    process.start(command(), QStringList() << QLatin1String("--version"));
    if (!process.waitForFinished()) {
        process.kill();
        future.reportResult(0);
        return;
    }

    // The version in Artistic Style is printed like "Artistic Style Version 2.04"
    const QString version = QString::fromUtf8(process.readAllStandardError()).trimmed();
    const QRegExp rx(QLatin1String("([2-9]{1})\\.([0-9]{2})(\\.[1-9]{1})?$"));
    if (rx.indexIn(version) != -1) {
        const int major = rx.cap(1).toInt() * 100;
        const int minor = rx.cap(2).toInt();
        future.reportResult(major + minor);
        return;
    }
    future.reportResult(0);
    return;
}

void ArtisticStyleSettings::helperSetVersion()
{
    m_version = m_versionWatcher.result();
}

bool ArtisticStyleSettings::useOtherFiles() const
{
    return m_settings.value(QLatin1String(kUseOtherFiles)).toBool();
}

void ArtisticStyleSettings::setUseOtherFiles(bool useOtherFiles)
{
    m_settings.insert(QLatin1String(kUseOtherFiles), QVariant(useOtherFiles));
}

bool ArtisticStyleSettings::useHomeFile() const
{
    return m_settings.value(QLatin1String(kUseHomeFile)).toBool();
}

void ArtisticStyleSettings::setUseHomeFile(bool useHomeFile)
{
    m_settings.insert(QLatin1String(kUseHomeFile), QVariant(useHomeFile));
}

bool ArtisticStyleSettings::useCustomStyle() const
{
    return m_settings.value(QLatin1String(kUseCustomStyle)).toBool();
}

void ArtisticStyleSettings::setUseCustomStyle(bool useCustomStyle)
{
    m_settings.insert(QLatin1String(kUseCustomStyle), QVariant(useCustomStyle));
}

QString ArtisticStyleSettings::customStyle() const
{
    return m_settings.value(QLatin1String(kCustomStyle)).toString();
}

void ArtisticStyleSettings::setCustomStyle(const QString &customStyle)
{
    m_settings.insert(QLatin1String(kCustomStyle), QVariant(customStyle));
}

QString ArtisticStyleSettings::documentationFilePath() const
{
    return Core::ICore::userResourcePath() + QLatin1Char('/')
            + QLatin1String(Beautifier::Constants::SETTINGS_DIRNAME) + QLatin1Char('/')
            + QLatin1String(Beautifier::Constants::DOCUMENTATION_DIRNAME) + QLatin1Char('/')
            + QLatin1String(Constants::ArtisticStyle::SETTINGS_NAME) + QLatin1String(".xml");
}

void ArtisticStyleSettings::createDocumentationFile() const
{
    QProcess process;
    process.start(command(), QStringList() << QLatin1String("-h"));
    process.waitForFinished(2000); // show help should be really fast.
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

    // astyle writes its output to 'error'...
    const QStringList lines = QString::fromUtf8(process.readAllStandardError())
            .split(QLatin1Char('\n'));
    QStringList keys;
    QStringList docu;
    foreach (QString line, lines) {
        line = line.trimmed();
        if ((line.startsWith(QLatin1String("--")) && !line.startsWith(QLatin1String("---")))
                || line.startsWith(QLatin1String("OR "))) {
            QStringList rawKeys = line.split(QLatin1String(" OR "), QString::SkipEmptyParts);
            foreach (QString k, rawKeys) {
                k = k.trimmed();
                k.remove(QLatin1Char('#'));
                keys << k;
                if (k.startsWith(QLatin1String("--")))
                    keys << k.right(k.size() - 2);
            }
        } else {
            if (line.isEmpty()) {
                if (!keys.isEmpty()) {
                    // Write entry
                    stream.writeStartElement(QLatin1String(Constants::DOCUMENTATION_XMLENTRY));
                    stream.writeStartElement(QLatin1String(Constants::DOCUMENTATION_XMLKEYS));
                    foreach (const QString &key, keys)
                        stream.writeTextElement(QLatin1String(Constants::DOCUMENTATION_XMLKEY), key);
                    stream.writeEndElement();
                    const QString text = QLatin1String("<p><span class=\"option\">")
                            + keys.filter(QRegExp(QLatin1String("^\\-"))).join(QLatin1String(", "))
                            + QLatin1String("</span></p><p>")
                            + (docu.join(QLatin1Char(' ')).toHtmlEscaped())
                            + QLatin1String("</p>");
                    stream.writeTextElement(QLatin1String(Constants::DOCUMENTATION_XMLDOC), text);
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
