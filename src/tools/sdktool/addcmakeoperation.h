// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "operation.h"

class AddCMakeData
{
public:
    QVariantMap addCMake(const QVariantMap &map) const;

    static QVariantMap initializeCMake();

    static bool exists(const QString &id);
    static bool exists(const QVariantMap &map, const QString &id);

    QString m_id;
    QString m_displayName;
    QString m_path;
    KeyValuePairList m_extra;
};

class AddCMakeOperation : public Operation, public AddCMakeData
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
