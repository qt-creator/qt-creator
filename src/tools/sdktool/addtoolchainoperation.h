// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "operation.h"

class AddToolChainData
{
public:
    QVariantMap addToolChain(const QVariantMap &map) const;

    static QVariantMap initializeToolChains();

    static bool exists(const QString &id);
    static bool exists(const QVariantMap &map, const QString &id);

    QString m_id;
    QString m_languageId;
    QString m_displayName;
    QString m_path;
    QString m_targetAbi;
    QString m_supportedAbis;
    KeyValuePairList m_extra;
};

class AddToolChainOperation : public Operation, public AddToolChainData
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
