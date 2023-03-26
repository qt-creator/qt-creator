// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "jsonrpcmessages.h"

namespace LanguageServerProtocol {

class Registration : public JsonObject
{
public:
    Registration() : Registration(QString()) {}
    explicit Registration(const QString &method)
    {
        setId(QUuid::createUuid().toString());
        setMethod(method);
    }
    using JsonObject::JsonObject;

    QString id() const { return typedValue<QString>(idKey); }
    void setId(const QString &id) { insert(idKey, id); }

    QString method() const { return typedValue<QString>(methodKey); }
    void setMethod(const QString &method) { insert(methodKey, method); }

    QJsonValue registerOptions() const { return value(registerOptionsKey); }
    void setRegisterOptions(const QJsonValue &registerOptions)
    { insert(registerOptionsKey, registerOptions); }

    bool isValid() const override { return contains(idKey) && contains(methodKey); }
};

class RegistrationParams : public JsonObject
{
public:
    RegistrationParams() : RegistrationParams(QList<Registration>()) {}
    explicit RegistrationParams(const QList<Registration> &registrations)
    { setRegistrations(registrations); }
    using JsonObject::JsonObject;

    QList<Registration> registrations() const { return array<Registration>(registrationsKey); }
    void setRegistrations(const QList<Registration> &registrations)
    { insertArray(registrationsKey, registrations); }

    bool isValid() const override { return contains(registrationsKey); }
};

class LANGUAGESERVERPROTOCOL_EXPORT RegisterCapabilityRequest : public Request<
        std::nullptr_t, std::nullptr_t, RegistrationParams>
{
public:
    explicit RegisterCapabilityRequest(const RegistrationParams &params);
    using Request::Request;
    constexpr static const char methodName[] = "client/registerCapability";
};

class Unregistration : public JsonObject
{
public:
    using JsonObject::JsonObject;

    QString id() const { return typedValue<QString>(idKey); }
    void setId(const QString &id) { insert(idKey, id); }

    QString method() const { return typedValue<QString>(methodKey); }
    void setMethod(const QString &method) { insert(methodKey, method); }

    bool isValid() const override { return contains(idKey) && contains(methodKey); }
};

class UnregistrationParams : public JsonObject
{
public:
    using JsonObject::JsonObject;

    QList<Unregistration> unregistrations() const
    { return array<Unregistration>(unregistrationsKey); }
    void setUnregistrations(const QList<Unregistration> &unregistrations)
    { insertArray(unregistrationsKey, unregistrations); }

    bool isValid() const override { return contains(unregistrationsKey); }
};

class LANGUAGESERVERPROTOCOL_EXPORT UnregisterCapabilityRequest : public Request<
        std::nullptr_t, std::nullptr_t, UnregistrationParams>
{
public:
    explicit UnregisterCapabilityRequest(const UnregistrationParams &params);
    using Request::Request;
    constexpr static const char methodName[] = "client/unregisterCapability";
};

} // namespace LanguageClient
