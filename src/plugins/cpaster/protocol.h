// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

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

    ~Protocol() override;

    virtual QString name() const = 0;

    virtual unsigned capabilities() const = 0;
    virtual bool hasSettings() const;
    virtual const Core::IOptionsPage *settingsPage() const;

    virtual bool checkConfiguration(QString *errorMessage = nullptr);
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
                                       QWidget *parent = nullptr,
                                       bool showConfig = true);
    // Ensure configuration is correct
    static bool ensureConfiguration(Protocol *p,
                                    QWidget *parent = nullptr);

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
public:
    NetworkProtocol() = default;

    ~NetworkProtocol() override;

protected:
    QNetworkReply *httpGet(const QString &url, bool handleCookies = false);

    QNetworkReply *httpPost(const QString &link, const QByteArray &data,
                            bool handleCookies = false);

    // Check connectivity of host, displaying a message box.
    bool httpStatus(QString url, QString *errorMessage, bool useHttps = false);
};

} //namespace CodePaster
