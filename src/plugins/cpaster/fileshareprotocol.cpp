// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "fileshareprotocol.h"

#include "cpastertr.h"
#include "fileshareprotocolsettingspage.h"

#include <coreplugin/messagemanager.h>

#include <utils/fileutils.h>

#include <QXmlStreamReader>
#include <QXmlStreamAttribute>
#include <QTemporaryFile>
#include <QDir>
#include <QFileInfo>
#include <QDebug>

enum { debug = 0 };

const char tempPatternC[] = "pasterXXXXXX.xml";
const char tempGlobPatternC[] = "paster*.xml";
const char pasterElementC[] = "paster";
const char userElementC[] = "user";
const char descriptionElementC[] = "description";
const char textElementC[] = "text";

namespace CodePaster {

FileShareProtocol::FileShareProtocol() = default;

FileShareProtocol::~FileShareProtocol() = default;

QString FileShareProtocol::name() const
{
    return fileShareSettingsPage().displayName();
}

unsigned FileShareProtocol::capabilities() const
{
    return ListCapability | PostDescriptionCapability | PostUserNameCapability;
}

bool FileShareProtocol::hasSettings() const
{
    return true;
}

const Core::IOptionsPage *FileShareProtocol::settingsPage() const
{
    return &fileShareSettingsPage();
}

static bool parse(const QString &fileName,
                  QString *errorMessage,
                  QString *user = nullptr, QString *description = nullptr, QString *text = nullptr)
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
        *errorMessage = Tr::tr("Cannot open %1: %2").arg(fileName, file.errorString());
        return false;
    }
    QXmlStreamReader reader(&file);
    while (!reader.atEnd()) {
        if (reader.readNext() == QXmlStreamReader::StartElement) {
            const auto elementName = reader.name();
            // Check start element
            if (elementCount == 0 && elementName != QLatin1String(pasterElementC)) {
                *errorMessage = Tr::tr("%1 does not appear to be a paster file.").arg(fileName);
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
        *errorMessage = Tr::tr("Error in %1 at %2: %3")
                        .arg(fileName).arg(reader.lineNumber()).arg(reader.errorString());
        return false;
    }
    return true;
}

bool FileShareProtocol::checkConfiguration(QString *errorMessage)
{
    if (fileShareSettings().path().isEmpty()) {
        if (errorMessage)
            *errorMessage = Tr::tr("Please configure a path.");
        return false;
    }
    return true;
}

void FileShareProtocol::fetch(const QString &id)
{
    // Absolute or relative path name.
    QFileInfo fi(id);
    if (fi.isRelative())
        fi = fileShareSettings().path().pathAppended(id).toFileInfo();
    QString errorMessage;
    QString text;
    if (parse(fi.absoluteFilePath(), &errorMessage, nullptr, nullptr, &text))
        emit fetchDone(id, text, false);
    else
        emit fetchDone(id, errorMessage, true);
}

void FileShareProtocol::list()
{
    // Read out directory, display by date (latest first)
    QDir dir(fileShareSettings().path().toFSPathString(), tempGlobPatternC,
             QDir::Time, QDir::Files|QDir::NoDotAndDotDot|QDir::Readable);
    QStringList entries;
    QString user;
    QString description;
    QString errorMessage;
    const QChar blank = QLatin1Char(' ');
    const QFileInfoList entryInfoList = dir.entryInfoList();
    const int count = qMin(int(fileShareSettings().displayCount()), entryInfoList.size());
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

void FileShareProtocol::paste(
        const QString &text,
        ContentType /* ct */,
        int /* expiryDays */,
        const QString &username,
        const QString & /* comment */,
        const QString &description
        )
{
    // Write out temp XML file
    Utils::TempFileSaver saver(fileShareSettings().path().pathAppended(tempPatternC).toFSPathString());
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
        Core::MessageManager::writeDisrupting(saver.errorString());
        return;
    }

    emit pasteDone(saver.filePath().toUserOutput());
}
} // namespace CodePaster
