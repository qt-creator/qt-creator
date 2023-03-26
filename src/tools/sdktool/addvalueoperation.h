// Copyright (C) 2019 The Qt Company Ltd.
// Copyright (C) 2019 Christoph Schlosser, B/S/H/ <christoph.schlosser@bshg.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "operation.h"

class AddValueData
{
public:
    bool appendListToMap(QVariantMap &map) const;

    QString m_key;
    QVariantList m_values;
};

class AddValueOperation : public Operation, public AddValueData
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
