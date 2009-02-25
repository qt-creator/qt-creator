/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#ifndef FETCHER_H
#define FETCHER_H

#include <QHttp>
#include <QHttpResponseHeader>
#include <QString>

class Fetcher : public QHttp
{
    Q_OBJECT
public:
    Fetcher(const QString &host);

    int fetch(const QString &url);
    int fetch(int pasteID);

    QByteArray &body() { return m_body; }

    int status()        { return m_status; }
    bool hadError()     { return m_hadError; }

private slots:
    void gotRequestFinished(int id, bool error);
    void gotReadyRead(const QHttpResponseHeader &resp);

private:
    QString m_host;
    int m_status;
    bool m_hadError;
    QByteArray m_body;
};

#endif // FETCHER_H
