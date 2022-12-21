// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "operation.h"

class RmKeysOperation : public Operation
{
public:
    QString name() const;
    QString helpText() const;
    QString argumentsHelpText() const;

    bool setArguments(const QStringList &args);

    int execute() const;

#ifdef WITH_TESTS
    static void unittest();
#endif

    static QVariantMap rmKeys(const QVariantMap &map, const QStringList &removals);

private:
    QStringList m_keys;
    QString m_file;
};
