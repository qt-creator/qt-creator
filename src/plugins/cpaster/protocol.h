// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>

QT_BEGIN_NAMESPACE
class QNetworkReply;
class QWidget;
QT_END_NAMESPACE

namespace Core { class IOptionsPage; }

namespace CodePaster {

enum class Capability
{
    None            = 0,
    List            = 1 << 0,
    PostComment     = 1 << 1,
    PostDescription = 1 << 2,
    PostUserName    = 1 << 3
};

Q_DECLARE_FLAGS(Capabilities, Capability)
Q_DECLARE_OPERATORS_FOR_FLAGS(Capabilities)

class ProtocolData
{
public:
    QString name;
    Capabilities capabilities = Capability::None;
};

class Protocol : public QObject
{
    Q_OBJECT

public:
    enum ContentType {
        Text, C, Cpp, JavaScript, Diff, Xml
    };

    ~Protocol() override;

    QString name() const { return m_protocolData.name; }
    Capabilities capabilities() const { return m_protocolData.capabilities; };

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
    Protocol(const ProtocolData &data);
    static QString textFromHtml(QString data);
    static QString fixNewLines(QString in);

private:
    ProtocolData m_protocolData;
};

/* Network-based protocol helpers: Provides access with delayed
 * initialization to a QNetworkAccessManager and conveniences
 * for HTTP-requests. */
QNetworkReply *httpGet(const QString &url, bool handleCookies = false);

QNetworkReply *httpPost(const QString &link, const QByteArray &data, bool handleCookies = false);

} //namespace CodePaster
