// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "operation.h"

class RmCMakeData
{
public:
    QVariantMap rmCMake(const QVariantMap &map) const;

    QString m_id;
};

class RmCMakeOperation : public Operation, public RmCMakeData
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
