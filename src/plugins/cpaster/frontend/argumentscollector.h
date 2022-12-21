// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QStringList>

class ArgumentsCollector
{
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
