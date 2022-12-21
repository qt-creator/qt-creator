// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "operation.h"

#include <QHash>

class AddKitData
{
public:
    QVariantMap addKit(const QVariantMap &map) const;
    QVariantMap addKit(const QVariantMap &map, const QVariantMap &tcMap,
                       const QVariantMap &qtMap, const QVariantMap &devMap,
                       const QVariantMap &cmakeMap) const;

    static QVariantMap initializeKits();

    QString m_id;
    QString m_displayName;
    QString m_icon;
    QString m_debuggerId;
    quint32 m_debuggerEngine = 0;
    QString m_debugger;
    QString m_deviceType;
    QString m_device;
    QString m_buildDevice;
    QString m_sysRoot;
    QHash<QString, QString> m_tcs;
    QString m_qt;
    QString m_mkspec;
    QString m_cmakeId;
    QString m_cmakeGenerator;
    QString m_cmakeExtraGenerator;
    QString m_cmakeGeneratorToolset;
    QString m_cmakeGeneratorPlatform;
    QStringList m_cmakeConfiguration;
    QStringList m_env;
    KeyValuePairList m_extra;
};

class AddKitOperation : public Operation, public AddKitData
{
public:
    QString name() const final;
    QString helpText() const final;
    QString argumentsHelpText() const final;

    bool setArguments(const QStringList &args) final;
    int execute() const final;

#ifdef WITH_TESTS
    static void unittest();

#endif
};
