// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "operation.h"

class AddDebuggerData
{
public:
    QVariantMap addDebugger(const QVariantMap &map) const;

    static QVariantMap initializeDebuggers();

    QString m_id;
    QString m_displayName;
    int m_engine = 0;
    QString m_binary;
    QStringList m_abis;
    KeyValuePairList m_extra;
};

class AddDebuggerOperation : public Operation, public AddDebuggerData
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
