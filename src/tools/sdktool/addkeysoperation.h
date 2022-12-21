// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "operation.h"

class AddKeysData
{
public:
    QVariantMap addKeys(const QVariantMap &map) const;

    QList<KeyValuePair> m_data;
};

class AddKeysOperation : public Operation, public AddKeysData
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

private:
    QString m_file;
};
