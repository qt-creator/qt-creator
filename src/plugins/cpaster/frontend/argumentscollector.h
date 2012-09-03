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

#ifndef ARGUMENTSCOLLECTOR_H
#define ARGUMENTSCOLLECTOR_H

#include <QCoreApplication>
#include <QStringList>

class ArgumentsCollector
{
    Q_DECLARE_TR_FUNCTIONS(ArgumentsCollector)
public:
    ArgumentsCollector(const QStringList &availableProtocols);
    bool collect(const QStringList &args); // Application is already removed.

    enum RequestType { RequestTypeHelp, RequestTypeListProtocols, RequestTypePaste };
    RequestType requestType() const { return m_requestType; }

    QString errorString() const { return m_errorString; }
    QString usageString() const;

    // These are valid <=> requestType() == RequestTypePaste
    QString inputFilePath() const { return m_inputFilePath; }
    QString protocol() const { return m_protocol; }

private:
    void setRequest();
    void setPasteOptions();
    bool checkAndSetOption(const QString &optionString, QString &optionValue);

    const QStringList m_availableProtocols;
    QStringList m_arguments;
    RequestType m_requestType;
    QString m_inputFilePath;
    QString m_protocol;
    QString m_errorString;
};

#endif // ARGUMENTSCOLLECTOR_H
