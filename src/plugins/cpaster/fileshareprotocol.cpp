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

#include "fileshareprotocol.h"
#include "fileshareprotocolsettingspage.h"

#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>
#include <coreplugin/messageoutputwindow.h>

#include <utils/qtcassert.h>
#include <utils/fileutils.h>

#include <QXmlStreamReader>
#include <QXmlStreamAttribute>
#include <QTemporaryFile>
#include <QDir>
#include <QFileInfo>
#include <QDebug>

enum { debug = 0 };

static const char tempPatternC[] = "pasterXXXXXX.xml";
static const char tempGlobPatternC[] = "paster*.xml";
static const char pasterElementC[] = "paster";
static const char userElementC[] = "user";
static const char descriptionElementC[] = "description";
static const char textElementC[] = "text";

namespace CodePaster {

FileShareProtocol::FileShareProtocol() :
    m_settings(new FileShareProtocolSettings),
    m_settingsPage(new FileShareProtocolSettingsPage(m_settings))
{
    m_settings->fromSettings(Core::ICore::settings());
}

FileShareProtocol::~FileShareProtocol()
{
}

QString FileShareProtocol::name() const
{
    return m_settingsPage->displayName();
}

unsigned FileShareProtocol::capabilities() const
{
    return ListCapability|PostDescriptionCapability;
}

bool FileShareProtocol::hasSettings() const
{
    return true;
}

Core::IOptionsPage *FileShareProtocol::settingsPage() const
{
    return m_settingsPage;
}

static bool parse(const QString &fileName,
                  QString *errorMessage,
                  QString *user = 0, QString *description = 0, QString *text = 0)
{
    unsigned elementCount = 0;

    errorMessage->clear();
    if (user)
        user->clear();
    if (description)
        description->clear();
    if (text)
        text->clear();

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly|QIODevice::Text)) {
        *errorMessage = FileShareProtocol::tr("Cannot open %1: %2").arg(fileName, file.errorString());
        return false;
    }
    QXmlStreamReader reader(&file);
    while (!reader.atEnd()) {
        if (reader.readNext() == QXmlStreamReader::StartElement) {
            const QStringRef elementName = reader.name();
            // Check start element
            if (elementCount == 0 && elementName != QLatin1String(pasterElementC)) {
                *errorMessage = FileShareProtocol::tr("%1 does not appear to be a paster file.").arg(fileName);
                return false;
            }
            // Parse elements
            elementCount++;
            if (user && elementName == QLatin1String(userElementC))
                *user = reader.readElementText();
            else if (description && elementName == QLatin1String(descriptionElementC))
                *description = reader.readElementText();
            else if (text && elementName == QLatin1String(textElementC))
                *text = reader.readElementText();
        }
    }
    if (reader.hasError()) {
        *errorMessage = FileShareProtocol::tr("Error in %1 at %2: %3")
                        .arg(fileName).arg(reader.lineNumber()).arg(reader.errorString());
        return false;
    }
    return true;
}

bool FileShareProtocol::checkConfiguration(QString *errorMessage)
{
    if (m_settings->path.isEmpty()) {
        if (errorMessage)
            *errorMessage = tr("Please configure a path.");
        return false;
    }
    return true;
}

void FileShareProtocol::fetch(const QString &id)
{
    // Absolute or relative path name.
    QFileInfo fi(id);
    if (fi.isRelative())
        fi = QFileInfo(m_settings->path + QLatin1Char('/') + id);
    QString errorMessage;
    QString text;
    if (parse(fi.absoluteFilePath(), &errorMessage, 0, 0, &text))
        emit fetchDone(id, text, false);
    else
        emit fetchDone(id, errorMessage, true);
}

void FileShareProtocol::list()
{
    // Read out directory, display by date (latest first)
    QDir dir(m_settings->path, QLatin1String(tempGlobPatternC),
             QDir::Time, QDir::Files|QDir::NoDotAndDotDot|QDir::Readable);
    QStringList entries;
    QString user;
    QString description;
    QString errorMessage;
    const QChar blank = QLatin1Char(' ');
    const QFileInfoList entryInfoList = dir.entryInfoList();
    const int count = qMin(m_settings->displayCount, entryInfoList.size());
    for (int i = 0; i < count; i++) {
        const QFileInfo& entryFi = entryInfoList.at(i);
        if (parse(entryFi.absoluteFilePath(), &errorMessage, &user, &description)) {
            QString entry = entryFi.fileName();
            entry += blank;
            entry += user;
            entry += blank;
            entry += description;
            entries.push_back(entry);
        }
        if (debug)
            qDebug() << entryFi.absoluteFilePath() << errorMessage;
    }
    emit listDone(name(), entries);
}

void FileShareProtocol::paste(const QString &text,
                              ContentType /* ct */,
                              const QString &username,
                              const QString & /* comment */,
                              const QString &description)
{
    // Write out temp XML file
    Utils::TempFileSaver saver(m_settings->path + QLatin1Char('/') + QLatin1String(tempPatternC));
    saver.setAutoRemove(false);
    if (!saver.hasError()) {
        // Flat text sections embedded into pasterElement
        QXmlStreamWriter writer(saver.file());
        writer.writeStartDocument();
        writer.writeStartElement(QLatin1String(pasterElementC));

        writer.writeTextElement(QLatin1String(userElementC), username);
        writer.writeTextElement(QLatin1String(descriptionElementC), description);
        writer.writeTextElement(QLatin1String(textElementC), text);

        writer.writeEndElement();
        writer.writeEndDocument();

        saver.setResult(&writer);
    }
    if (!saver.finalize()) {
        Core::ICore::messageManager()->printToOutputPanePopup(saver.errorString());
        return;
    }

    const QString msg = tr("Pasted: %1").arg(saver.fileName());
    Core::ICore::messageManager()->printToOutputPanePopup(msg);
}
} // namespace CodePaster
