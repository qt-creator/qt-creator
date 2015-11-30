/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <QObject>
#include <QScopedPointer>
#include <QSharedPointer>

QT_BEGIN_NAMESPACE
class QNetworkReply;
class QWidget;
QT_END_NAMESPACE

namespace Core { class IOptionsPage; }

namespace CodePaster {

class Protocol : public QObject
{
    Q_OBJECT

public:
    enum ContentType {
        Text, C, Cpp, JavaScript, Diff, Xml
    };

    enum Capabilities  {
        ListCapability = 0x1,
        PostCommentCapability = 0x2,
        PostDescriptionCapability = 0x4,
        PostUserNameCapability = 0x8
    };

    virtual ~Protocol();

    virtual QString name() const = 0;

    virtual unsigned capabilities() const = 0;
    virtual bool hasSettings() const;
    virtual Core::IOptionsPage *settingsPage() const;

    virtual bool checkConfiguration(QString *errorMessage = 0);
    virtual void fetch(const QString &id) = 0;
    virtual void list();
    virtual void paste(const QString &text,
                       ContentType ct = Text,
                       int expiryDays = 1,
                       const QString &username = QString(),
                       const QString &comment = QString(),
                       const QString &description = QString()) = 0;

    // Convenience to determine content type from mime type
    static ContentType contentType(const QString &mimeType);

    // Show a configuration error and point user to settings.
    // Return true when settings changed.
    static bool showConfigurationError(const Protocol *p,
                                       const QString &message,
                                       QWidget *parent = 0,
                                       bool showConfig = true);
    // Ensure configuration is correct
    static bool ensureConfiguration(Protocol *p,
                                    QWidget *parent = 0);

signals:
    void pasteDone(const QString &link);
    void fetchDone(const QString &titleDescription,
                   const QString &content,
                   bool error);
    void listDone(const QString &name, const QStringList &result);

protected:
    Protocol();
    static QString textFromHtml(QString data);
    static QString fixNewLines(QString in);
};

/* Network-based protocol: Provides access with delayed
 * initialization to a QNetworkAccessManager and conveniences
 * for HTTP-requests. */

class NetworkProtocol : public Protocol
{
    Q_OBJECT

public:
    virtual ~NetworkProtocol();

protected:
    QNetworkReply *httpGet(const QString &url);

    QNetworkReply *httpPost(const QString &link, const QByteArray &data);

    // Check connectivity of host, displaying a message box.
    bool httpStatus(QString url, QString *errorMessage, bool useHttps = false);
};

} //namespace CodePaster

#endif // PROTOCOL_H
