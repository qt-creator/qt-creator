/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

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

    bool isValid(ErrorHierarchy *error) const override
    { return check<QString>(error, idKey) && check<QString>(error, methodKey); }
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

    bool isValid(ErrorHierarchy *error) const override
    { return checkArray<Registration>(error, registrationsKey); }
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

    bool isValid(ErrorHierarchy *error) const override
    { return check<QString>(error, idKey) && check<QString>(error, methodKey); }
};

class UnregistrationParams : public JsonObject
{
public:
    using JsonObject::JsonObject;

    QList<Unregistration> unregistrations() const
    { return array<Unregistration>(unregistrationsKey); }
    void setUnregistrations(const QList<Unregistration> &unregistrations)
    { insertArray(unregistrationsKey, unregistrations); }

    bool isValid(ErrorHierarchy *error) const override
    { return checkArray<Unregistration>(error, unregistrationsKey); }
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
