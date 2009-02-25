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

#ifndef POSTER_H
#define POSTER_H

#include <QHttp>
#include <QHttpResponseHeader>
#include <QString>

class Poster : public QHttp
{
    Q_OBJECT
public:
    Poster(const QString &host);

    void post(const QString &description, const QString &comment,
              const QString &text, const QString &user);

    QString pastedUrl() { return m_url; }
    int status()        { return m_status; }
    bool hadError()     { return m_hadError; }

private slots:
    void gotRequestFinished(int id, bool error);
    void gotResponseHeaderReceived(const QHttpResponseHeader &resp);

private:
    QString m_url;
    int m_status;
    bool m_hadError;
};

#endif // POSTER_H
