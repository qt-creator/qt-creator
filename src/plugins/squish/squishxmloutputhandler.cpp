// Copyright (C) 2022 The Qt Company Ltd
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "squishxmloutputhandler.h"

#include "squishoutputpane.h"
#include "squishresultmodel.h"
#include "squishtr.h"

#include <utils/filepath.h>
#include <utils/qtcassert.h>

#include <QDateTime>
#include <QDebug>
#include <QFile>
#include <QXmlStreamWriter>

namespace Squish {
namespace Internal {

SquishXmlOutputHandler::SquishXmlOutputHandler(QObject *parent)
    : QObject(parent)
{
    connect(this,
            &SquishXmlOutputHandler::resultItemCreated,
            SquishOutputPane::instance(),
            &SquishOutputPane::addResultItem,
            Qt::QueuedConnection);
}

void SquishXmlOutputHandler::clearForNextRun()
{
    m_xmlReader.clear();
}

void SquishXmlOutputHandler::mergeResultFiles(const Utils::FilePaths &reportFiles,
                                              const Utils::FilePath &resultsDirectory,
                                              const QString &suiteName,
                                              QString *error)
{
    QFile resultsXML(resultsDirectory.pathAppended("results.xml").toString());
    if (resultsXML.exists()) {
        if (error)
            *error = Tr::tr("Could not merge results into single results.xml.\n"
                            "Destination file \"%1\" already exists.")
                         .arg(resultsXML.fileName());
        return;
    }

    if (!resultsXML.open(QFile::WriteOnly)) {
        if (error)
            *error = Tr::tr("Could not merge results into single results.xml.\n"
                            "Failed to open file \"%1\".")
                         .arg(resultsXML.fileName());
        return;
    }

    QXmlStreamWriter xmlWriter(&resultsXML);
    xmlWriter.writeStartDocument("1.0");
    bool isFirstReport = true;
    bool isFirstTest = true;
    QString lastEpilogTime;
    for (const Utils::FilePath &caseResult : reportFiles) {
        QFile currentResultsFile(caseResult.toString());
        if (!currentResultsFile.exists())
            continue;
        if (!currentResultsFile.open(QFile::ReadOnly))
            continue;
        QXmlStreamReader reader(&currentResultsFile);
        while (!reader.atEnd()) {
            QXmlStreamReader::TokenType type = reader.readNext();
            switch (type) {
            case QXmlStreamReader::StartElement: {
                const QString tagName = reader.name().toString();
                // SquishReport of the first results.xml will be taken as is and as this is a
                // merged results.xml we add another test tag holding the suite's name
                if (tagName == "SquishReport") {
                    if (isFirstReport) {
                        xmlWriter.writeStartElement(tagName);
                        xmlWriter.writeAttributes(reader.attributes());
                        xmlWriter.writeStartElement("test");
                        xmlWriter.writeAttribute("name", suiteName);
                        isFirstReport = false;
                    }
                    break;
                }
                if (isFirstTest && tagName == "test") {
                    // the prolog tag of the first results.xml holds the start time of the suite
                    // we already wrote the test tag for the suite, but haven't added the start
                    // time as we didn't know about it, so store information of the current test
                    // tag (case name), read ahead (prolog tag), write prolog (suite's test tag)
                    // and finally write test tag (case name) - the prolog tag (for test case)
                    // will be written outside the if
                    const QXmlStreamAttributes testAttributes = reader.attributes();
                    QXmlStreamReader::TokenType token = QXmlStreamReader::NoToken;
                    while (!reader.atEnd()) {
                        token = reader.readNext();
                        if (token != QXmlStreamReader::Characters)
                            break;
                    }
                    const QString prolog = reader.name().toString();
                    QTC_ASSERT(token == QXmlStreamReader::StartElement
                                   && prolog == "prolog",
                               if (error) *error = Tr::tr("Error while parsing first test result.");
                               return );
                    xmlWriter.writeStartElement(prolog);
                    xmlWriter.writeAttributes(reader.attributes());
                    xmlWriter.writeEndElement();
                    xmlWriter.writeStartElement("test");
                    xmlWriter.writeAttributes(testAttributes);
                    isFirstTest = false;
                } else if (tagName == "epilog") {
                    lastEpilogTime = reader.attributes().value("time").toString();
                }
                xmlWriter.writeCurrentToken(reader);
                break;
            }
            case QXmlStreamReader::EndElement:
                if (reader.name() != QLatin1String("SquishReport"))
                    xmlWriter.writeCurrentToken(reader);
                break;
            case QXmlStreamReader::Characters:
                xmlWriter.writeCurrentToken(reader);
                break;
            // ignore the rest
            default:
                break;
            }
        }
        currentResultsFile.close();
    }
    if (!lastEpilogTime.isEmpty()) {
        xmlWriter.writeStartElement("epilog");
        xmlWriter.writeAttribute("time", lastEpilogTime);
        xmlWriter.writeEndElement();
    }
    xmlWriter.writeEndDocument();
}

Result::Type resultFromString(const QString &type)
{
    if (type == "DETAILED")
        return Result::Detail;
    if (type == "LOG")
        return Result::Log;
    if (type == "PASS")
        return Result::Pass;
    if (type == "FAIL")
        return Result::Fail;
    if (type == "WARNING")
        return Result::Warn;
    if (type == "XFAIL")
        return Result::ExpectedFail;
    if (type == "XPASS")
        return Result::UnexpectedPass;
    if (type == "FATAL")
        return Result::Fatal;
    if (type == "ERROR")
        return Result::Error;
    return Result::Log;
}

// this method uses the XML reader to parse output of the Squish results.xml and put it into an
// item that can be used to display inside the test results pane
// TODO: support Squish report 3.x as well
void SquishXmlOutputHandler::outputAvailable(const QByteArray &output)
{
    static SquishResultItem *testCaseRootItem;
    static QString name;
    static QString details;
    static QString logDetails;
    static QStringList logDetailsList;
    static QString time;
    static QString file;
    static Result::Type type;
    static int line = 0;
    static bool prepend = false;

    m_xmlReader.addData(output);

    while (!m_xmlReader.atEnd()) {
        QXmlStreamReader::TokenType tokenType = m_xmlReader.readNext();
        switch (tokenType) {
        case QXmlStreamReader::StartDocument:
        case QXmlStreamReader::EndDocument:
            break;
        case QXmlStreamReader::StartElement: {
            const QString currentName = m_xmlReader.name().toString();
            // tags verification, message, epilog and test will start a new entry, so, reset values
            if (currentName == "verification"
                || currentName == "message"
                || currentName == "epilog"
                || currentName == "test") {
                name = currentName;
                details.clear();
                logDetails.clear();
                logDetailsList.clear();
                time.clear();
                file.clear();
                line = 0;
                type = Result::Log;
                if (currentName == "test")
                    testCaseRootItem = nullptr;
            } else if (currentName == "result") {
                // result tag won't add another entry, but gives more information on enclosing tag
                name = currentName;
            }

            // description tag could provide information that must be prepended to the former entry
            if (currentName == "description") {
                prepend = (name == "result" && m_xmlReader.attributes().isEmpty());
            } else {
                const QXmlStreamAttributes attributes = m_xmlReader.attributes();
                for (const QXmlStreamAttribute &att : attributes) {
                    const QString attributeName = att.name().toString();
                    if (attributeName == "time")
                        time = QDateTime::fromString(att.value().toString(), Qt::ISODate)
                                   .toString("MMM dd, yyyy h:mm:ss AP");
                    else if (attributeName == "file")
                        file = att.value().toString();
                    else if (attributeName == "line")
                        line = att.value().toInt();
                    else if (attributeName == "type")
                        type = resultFromString(att.value().toString());
                    else if (attributeName == "name")
                        logDetails = att.value().toString();
                }
            }
            // handle prolog (test) elements already within the start tag
            if (currentName == "prolog") {
                TestResult result(Result::Start, logDetails, time);
                result.setFile(file);
                result.setLine(line);
                testCaseRootItem = new SquishResultItem(result);
                emit resultItemCreated(testCaseRootItem);
                emit updateStatus(result.text());
            }
            break;
        }
        case QXmlStreamReader::EndElement: {
            const QString currentName = m_xmlReader.name().toString();
            // description and result tags are handled differently, test tags are handled by
            // prolog tag (which is handled in QXmlStreamReader::StartElement already),
            // SquishReport tags will be ignored completely
            if (currentName == "epilog") {
                QTC_ASSERT(testCaseRootItem, break);
                const TestResult result(Result::End, QString(), time);
                SquishResultItem *item = new SquishResultItem(result);
                testCaseRootItem->appendChild(item);
                emit updateStatus(result.text());
            } else if (currentName == "description") {
                if (!prepend && !details.trimmed().isEmpty()) {
                    logDetailsList.append(details.trimmed());
                    details.clear();
                }
            } else if (currentName != "prolog"
                       && currentName != "test"
                       && currentName != "result"
                       && currentName != "SquishReport") {
                QTC_ASSERT(testCaseRootItem, break);
                TestResult result(type, logDetails, time);
                if (logDetails.isEmpty() && !logDetailsList.isEmpty())
                    result.setText(logDetailsList.takeFirst());
                result.setFile(file);
                result.setLine(line);
                SquishResultItem *item = new SquishResultItem(result);
                emit updateStatus(result.text());

                switch (result.type()) {
                case Result::Pass:
                case Result::ExpectedFail:
                    emit increasePassCounter();
                    break;
                case Result::Error:
                case Result::Fail:
                case Result::Fatal:
                case Result::UnexpectedPass:
                    emit increaseFailCounter();
                    break;
                default:
                    break;
                }

                if (!logDetailsList.isEmpty()) {
                    for (const QString &detail : std::as_const(logDetailsList)) {
                        const TestResult childResult(Result::Detail, detail);
                        SquishResultItem *childItem = new SquishResultItem(childResult);
                        item->appendChild(childItem);
                        emit updateStatus(childResult.text());
                    }
                }
                testCaseRootItem->appendChild(item);
            }
            break;
        }
        case QXmlStreamReader::Characters: {
            QString text = m_xmlReader.text().toString();
            if (m_xmlReader.isCDATA() || !text.trimmed().isEmpty()) {
                if (!m_xmlReader.isCDATA())
                    text = text.trimmed();
                if (prepend) {
                    if (!logDetails.isEmpty() && (text == "Verified" || text == "Not Verified"))
                        logDetails.prepend(text + ": ");
                    else
                        logDetails = text;
                } else {
                    details.append(text).append('\n');
                }
            }
            break;
        }
        default:
            break;
        }
    }

    if (m_xmlReader.hasError()) {
        // kind of expected as we get the output piece by piece
        if (m_xmlReader.error() != QXmlStreamReader::PrematureEndOfDocumentError)
            qWarning() << m_xmlReader.error() << m_xmlReader.errorString();
    }
}

} // namespace Internal
} // namespace Squish
