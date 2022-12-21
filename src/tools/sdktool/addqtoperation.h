// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "operation.h"

class AddQtData
{
public:
    QVariantMap addQt(const QVariantMap &map) const;

    static QVariantMap initializeQtVersions();

    static bool exists(const QString &id);
    static bool exists(const QVariantMap &map, const QString &id);

    QString m_id; // actually this is the autodetectionSource
    QString m_displayName;
    QString m_type;
    QString m_qmake;
    QStringList m_abis;
    KeyValuePairList m_extra;
};

class AddQtOperation : public Operation, public AddQtData
{
private:
    QString name() const final;
    QString helpText() const final;
    QString argumentsHelpText() const final;
    bool setArguments(const QStringList &args) final;
    int execute() const final;

#ifdef WITH_TESTS
public:
    static void unittest();
     // TODO: Remove
#endif
};
