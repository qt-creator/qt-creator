// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "operation.h"

class AddAbiFlavorData
{
public:
    QVariantMap addAbiFlavor(const QVariantMap &map) const;

    static QVariantMap initializeAbiFlavors();

    static bool exists(const QString &flavor);
    static bool exists(const QVariantMap &map, const QString &flavor);

    QStringList m_oses;
    QString m_flavor;
};

class AddAbiFlavor : public Operation, public AddAbiFlavorData
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
