/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/
#include "argumentscollector.h"

#include <QFileInfo>

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
    QString usage = tr("Usage:");
    usage += QLatin1String("\n\t");
    usage += tr("%1 <request> [ <request options>]")
            .arg(QFileInfo(QCoreApplication::applicationFilePath()).fileName());
    usage += QLatin1String("\n\t");
    usage += tr("Possible requests: \"%1\", \"%2\", \"%3\"")
            .arg(pasteRequestString(), listProtocolsRequestString(), helpRequestString());
    usage += QLatin1String("\n\t");
    usage += tr("Possible options for request \"%1\": \"%2 <file>\" (default: stdin), "
                "\"%3 <protocol>\"")
            .arg(pasteRequestString(), pasteFileOptionString(), pasteProtocolOptionString());
    usage += QLatin1Char('\n');
    return usage;
}

void ArgumentsCollector::setRequest()
{
    if (m_arguments.isEmpty())
        throw ArgumentErrorException(tr("No request given"));
    const QString requestString = m_arguments.takeFirst();
    if (requestString == pasteRequestString())
        m_requestType = RequestTypePaste;
    else if (requestString == listProtocolsRequestString())
        m_requestType = RequestTypeListProtocols;
    else if (requestString == helpRequestString())
        m_requestType = RequestTypeHelp;
    else
        throw ArgumentErrorException(tr("Unknown request \"%1\"").arg(requestString));
}

void ArgumentsCollector::setPasteOptions()
{
    while (!m_arguments.isEmpty()) {
        if (checkAndSetOption(pasteFileOptionString(), m_inputFilePath))
            continue;
        if (checkAndSetOption(pasteProtocolOptionString(), m_protocol)) {
            if (!m_availableProtocols.contains(m_protocol))
                throw ArgumentErrorException(tr("Unknown protocol \"%1\"").arg(m_protocol));
            continue;
        }
        throw ArgumentErrorException(tr("Invalid option \"%1\" for request \"%2\"")
                .arg(m_arguments.first(), pasteRequestString()));
    }

    if (m_protocol.isEmpty())
        throw ArgumentErrorException(tr("No protocol given"));
}

bool ArgumentsCollector::checkAndSetOption(const QString &optionString, QString &optionValue)
{
    if (m_arguments.first() != optionString)
        return false;

    if (!optionValue.isEmpty())
        throw ArgumentErrorException(tr("option \"%1\" was given twice").arg(optionString));
    m_arguments.removeFirst();
    if (m_arguments.isEmpty()) {
        throw ArgumentErrorException(tr("Option \"%1\" requires an argument")
                .arg(optionString));
    }
    optionValue = m_arguments.takeFirst();
    return true;
}
