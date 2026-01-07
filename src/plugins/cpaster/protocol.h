// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/result.h>

#include <QObject>
#include <QtTaskTree/QTaskTree>

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
    PostDescription = 1 << 1,
    PostUserName    = 1 << 2
};

Q_DECLARE_FLAGS(Capabilities, Capability)
Q_DECLARE_OPERATORS_FOR_FLAGS(Capabilities)

using ConfigChecker = std::function<Utils::Result<>()>;

class ProtocolData
{
public:
    QString name;
    Capabilities capabilities = Capability::None;
    ConfigChecker configChecker = {};
    Core::IOptionsPage *settingsPage = nullptr;
};

enum ContentType {
    Text, C, Cpp, JavaScript, Diff, Xml
};

class PasteInputData
{
public:
    QString text;
    ContentType ct = Text;
    int expiryDays = 1;
    QString username = {};
    QString description = {};
};

using FetchHandler = std::function<void(const QString &, const QString &)>;
using ListHandler = std::function<void(const QStringList &)>;

class Protocol : public QObject
{
    Q_OBJECT

public:
    ~Protocol() override;

    QString name() const { return m_protocolData.name; }
    Capabilities capabilities() const { return m_protocolData.capabilities; };
    Utils::Result<> checkConfiguration() const {
        return m_protocolData.configChecker ? m_protocolData.configChecker() : Utils::ResultOk;
    }
    const Core::IOptionsPage *settingsPage() const { return m_protocolData.settingsPage; }

    virtual QtTaskTree::ExecutableItem fetchRecipe(const QString &id,
                                                   const FetchHandler &handler) const;
    virtual QtTaskTree::ExecutableItem listRecipe(const ListHandler &handler) const;
    virtual void paste(const PasteInputData &inputData) = 0;

    // Ensure configuration is correct
    static bool ensureConfiguration(Protocol *p,
                                    QWidget *parent = nullptr);

signals:
    void pasteDone(const QString &link);

protected:
    Protocol(const ProtocolData &data);
    void reportError(const QString &message) const;
    static QString fixNewLines(QString in);

private:
    ProtocolData m_protocolData;
};

QNetworkReply *httpPost(const QString &link, const QByteArray &data);

} //namespace CodePaster
