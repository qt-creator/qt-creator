// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "fileshareprotocol.h"

#include "cpastertr.h"
#include "fileshareprotocolsettingspage.h"

#include <coreplugin/messagemanager.h>

#include <utils/fileutils.h>

#include <QXmlStreamReader>
#include <QXmlStreamAttribute>
#include <QDir>
#include <QFileInfo>
#include <QDebug>

using namespace QtTaskTree;
using namespace Utils;

namespace CodePaster {

enum { debug = 0 };

const char tempPatternC[] = "pasterXXXXXX.xml";
const char tempGlobPatternC[] = "paster*.xml";
const char pasterElementC[] = "paster";
const char userElementC[] = "user";
const char descriptionElementC[] = "description";
const char textElementC[] = "text";
const char protocolNameC[] = "Fileshare";

static Result<> checkConfig()
{
    return fileShareSettings().path().isEmpty() ? ResultError(Tr::tr("Please configure a path."))
                                                : ResultOk;
}

FileShareProtocol::FileShareProtocol()
    : Protocol({protocolNameC,
                Capability::List | Capability::PostDescription | Capability::PostUserName,
                checkConfig, &fileShareSettingsPage()})
{}

struct ParseResult
{
    QString user;
    QString description;
    QString text;
};

static Result<ParseResult> parse(const QString &fileName)
{
    unsigned elementCount = 0;

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly|QIODevice::Text))
        return ResultError(Tr::tr("Cannot open %1: %2").arg(fileName, file.errorString()));

    ParseResult result;
    QXmlStreamReader reader(&file);
    while (!reader.atEnd()) {
        if (reader.readNext() == QXmlStreamReader::StartElement) {
            const auto elementName = reader.name();
            // Check start element
            if (elementCount == 0 && elementName != QLatin1String(pasterElementC))
                return ResultError(Tr::tr("%1 does not appear to be a paster file.").arg(fileName));

            // Parse elements
            elementCount++;
            if (elementName == QLatin1String(userElementC))
                result.user = reader.readElementText();
            else if (elementName == QLatin1String(descriptionElementC))
                result.description = reader.readElementText();
            else if (elementName == QLatin1String(textElementC))
                result.text = reader.readElementText();
        }
    }
    if (reader.hasError()) {
        return ResultError(Tr::tr("Error in %1 at %2: %3")
                               .arg(fileName).arg(reader.lineNumber()).arg(reader.errorString()));
    }
    return result;
}

ExecutableItem FileShareProtocol::fetchRecipe(const QString &id,
                                              const FetchHandler &handler) const
{
    return QSyncTask([this, id, handler] {
        // Absolute or relative path name.
        QFileInfo fi(id);
        if (fi.isRelative())
            fi = fileShareSettings().path().pathAppended(id).toFileInfo();
        const auto result = parse(fi.absoluteFilePath());
        if (result) {
            if (handler)
                handler(id, result->text);
            return true;
        }
        reportError(result.error());
        return false;
    });
}

static QFileInfoList fileShareInfoList()
{
    // Read out directory, display by date (latest first)
    QDir dir(fileShareSettings().path().toFSPathString(), tempGlobPatternC,
             QDir::Time, QDir::Files | QDir::NoDotAndDotDot | QDir::Readable);
    const QFileInfoList entryInfoList = dir.entryInfoList();
    return entryInfoList.mid(0, fileShareSettings().displayCount());
}

QtTaskTree::ExecutableItem FileShareProtocol::listRecipe(const ListHandler &handler) const
{
    return QSyncTask([handler] {
        QStringList entries;
        const QFileInfoList entryInfoList = fileShareInfoList();
        for (const QFileInfo &entry : entryInfoList) {
            const auto result = parse(entry.absoluteFilePath());
            if (result)
                entries.append(entry.fileName() + u' ' + result->user + u' ' + result->description);
            else if (debug)
                qDebug() << entry.absoluteFilePath() << result.error();
        }
        if (handler)
            handler(entries);
    });
}

ExecutableItem FileShareProtocol::pasteRecipe(const PasteInputData &inputData,
                                              const PasteHandler &handler) const
{
    return QSyncTask([this, inputData, handler] {
        // Write out temp XML file
        TempFileSaver saver(fileShareSettings().path().pathAppended(tempPatternC).toFSPathString());
        saver.setAutoRemove(false);
        if (!saver.hasError()) {
            // Flat text sections embedded into pasterElement
            QXmlStreamWriter writer(saver.file());
            writer.writeStartDocument();
            writer.writeStartElement(QLatin1String(pasterElementC));

            writer.writeTextElement(QLatin1String(userElementC), inputData.username);
            writer.writeTextElement(QLatin1String(descriptionElementC), inputData.description);
            writer.writeTextElement(QLatin1String(textElementC), inputData.text);

            writer.writeEndElement();
            writer.writeEndDocument();

            saver.setResult(&writer);
        }
        if (const Result<> res = saver.finalize(); !res) {
            reportError(res.error());
            return;
        }

        if (handler)
            handler(saver.filePath().toUserOutput());
    });
}

} // namespace CodePaster
