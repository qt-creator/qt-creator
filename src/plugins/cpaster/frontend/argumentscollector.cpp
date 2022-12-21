// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "argumentscollector.h"

#include <utils/filepath.h>

#include <QCoreApplication>

static QString pasteRequestString() { return QLatin1String("paste"); }
static QString listProtocolsRequestString() { return QLatin1String("list-protocols"); }
static QString helpRequestString() { return QLatin1String("help"); }
static QString pasteFileOptionString() { return QLatin1String("-file"); }
static QString pasteProtocolOptionString() { return QLatin1String("-protocol"); }

namespace {
struct ArgumentErrorException
{
    ArgumentErrorException(const QString &error) : error(error) {}
    const QString error;
};
}

ArgumentsCollector::ArgumentsCollector(const QStringList &availableProtocols)
    : m_availableProtocols(availableProtocols)
{
}

bool ArgumentsCollector::collect(const QStringList &args)
{
    m_arguments = args;
    m_errorString.clear();
    m_inputFilePath.clear();
    m_protocol.clear();
    try {
        setRequest();
        if (m_requestType == RequestTypePaste)
            setPasteOptions();
        return true;
    } catch (const ArgumentErrorException &ex) {
        m_errorString = ex.error;
        return false;
    }
}

QString ArgumentsCollector::usageString() const
{
    QString usage = QString::fromLatin1("Usage:\n\t%1 <request> [ <request options>]\n\t")
            .arg(Utils::FilePath::fromString(QCoreApplication::applicationFilePath()).fileName());
    usage += QString::fromLatin1("Possible requests: \"%1\", \"%2\", \"%3\"\n\t")
            .arg(pasteRequestString(), listProtocolsRequestString(), helpRequestString());
    usage += QString::fromLatin1("Possible options for request \"%1\": \"%2 <file>\" (default: stdin), "
                "\"%3 <protocol>\"\n")
            .arg(pasteRequestString(), pasteFileOptionString(), pasteProtocolOptionString());
    return usage;
}

void ArgumentsCollector::setRequest()
{
    if (m_arguments.isEmpty())
        throw ArgumentErrorException(QLatin1String("No request given"));
    const QString requestString = m_arguments.takeFirst();
    if (requestString == pasteRequestString())
        m_requestType = RequestTypePaste;
    else if (requestString == listProtocolsRequestString())
        m_requestType = RequestTypeListProtocols;
    else if (requestString == helpRequestString())
        m_requestType = RequestTypeHelp;
    else
        throw ArgumentErrorException(QString::fromLatin1("Unknown request \"%1\"").arg(requestString));
}

void ArgumentsCollector::setPasteOptions()
{
    while (!m_arguments.isEmpty()) {
        if (checkAndSetOption(pasteFileOptionString(), m_inputFilePath))
            continue;
        if (checkAndSetOption(pasteProtocolOptionString(), m_protocol)) {
            if (!m_availableProtocols.contains(m_protocol))
                throw ArgumentErrorException(QString::fromLatin1("Unknown protocol \"%1\"").arg(m_protocol));
            continue;
        }
        throw ArgumentErrorException(QString::fromLatin1("Invalid option \"%1\" for request \"%2\"")
                .arg(m_arguments.first(), pasteRequestString()));
    }

    if (m_protocol.isEmpty())
        throw ArgumentErrorException(QLatin1String("No protocol given"));
}

bool ArgumentsCollector::checkAndSetOption(const QString &optionString, QString &optionValue)
{
    if (m_arguments.first() != optionString)
        return false;

    if (!optionValue.isEmpty())
        throw ArgumentErrorException(QString::fromLatin1("option \"%1\" was given twice").arg(optionString));
    m_arguments.removeFirst();
    if (m_arguments.isEmpty()) {
        throw ArgumentErrorException(QString::fromLatin1("Option \"%1\" requires an argument")
                .arg(optionString));
    }
    optionValue = m_arguments.takeFirst();
    return true;
}
