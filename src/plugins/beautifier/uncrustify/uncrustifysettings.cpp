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
}

UncrustifySettings::UncrustifySettings() :
    AbstractSettings(QLatin1String(Constants::Uncrustify::SETTINGS_NAME), QLatin1String(".cfg"))
{
    setCommand(QLatin1String("uncrustify"));
    m_settings.insert(kUseOtherFiles, QVariant(true));
    m_settings.insert(kUseHomeFile, QVariant(false));
    m_settings.insert(kUseCustomStyle, QVariant(false));
    m_settings.insert(kCustomStyle, QVariant());
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
