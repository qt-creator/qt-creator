/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "qdeclarativeoutputparser.h"
#include "qmljsdebugclientconstants.h"

namespace QmlJsDebugClient {

QDeclarativeOutputParser::QDeclarativeOutputParser(QObject *parent)
    : QObject(parent)
{
}

void QDeclarativeOutputParser::setNoOutputText(const QString &text)
{
    m_noOutputText = text;
}

void QDeclarativeOutputParser::processOutput(const QString &output)
{
    m_buffer.append(output);

    while (true) {
        const int nlIndex = m_buffer.indexOf(QLatin1Char('\n'));
        if (nlIndex < 0) // no further complete lines
            break;

        const QString msg = m_buffer.left(nlIndex);
        m_buffer = m_buffer.right(m_buffer.size() - nlIndex - 1);

        static const QString qddserver = QLatin1String("QDeclarativeDebugServer: ");

        const int index = msg.indexOf(qddserver);
        if (index != -1) {
            QString status = msg;
            status.remove(0, index + qddserver.length()); // chop of 'QDeclarativeDebugServer: '

            static QString waitingForConnection = QLatin1String(Constants::STR_WAITING_FOR_CONNECTION);
            static QString unableToListen = QLatin1String(Constants::STR_UNABLE_TO_LISTEN);
            static QString debuggingNotEnabled = QLatin1String(Constants::STR_IGNORING_DEBUGGER);
            static QString debuggingNotEnabled2 = QLatin1String(Constants::STR_IGNORING_DEBUGGER2);
            static QString connectionEstablished = QLatin1String(Constants::STR_CONNECTION_ESTABLISHED);

            if (status.startsWith(waitingForConnection)) {
                emit waitingForConnectionMessage();
            } else if (status.startsWith(unableToListen)) {
                //: Error message shown after 'Could not connect ... debugger:"
                emit errorMessage(tr("The port seems to be in use."));
            } else if (status.startsWith(debuggingNotEnabled) || status.startsWith(debuggingNotEnabled2)) {
                //: Error message shown after 'Could not connect ... debugger:"
                emit errorMessage(tr("The application is not set up for QML/JS debugging."));
            } else if (status.startsWith(connectionEstablished)) {
                emit connectionEstablishedMessage();
            } else {
                emit unknownMessage(status);
            }
        } else if (msg.contains(m_noOutputText)) {
            emit noOutputMessage();
        }


    }
}

} // namespace QmLJsDebugClient
