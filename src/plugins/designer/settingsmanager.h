// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QDesignerSettingsInterface>

namespace Designer {
namespace Internal {

/* Prepends "Designer" to every value stored/retrieved by designer plugins,
   to avoid namespace polution. We cannot use a group because groups cannot be nested,
   and designer uses groups internally. */
class SettingsManager : public QDesignerSettingsInterface
{
public:
    void beginGroup(const QString &prefix) override;
    void endGroup() override;

    bool contains(const QString &key) const override;
    void setValue(const QString &key, const QVariant &value) override;
    QVariant value(const QString &key, const QVariant &defaultValue = QVariant()) const override;
    void remove(const QString &key) override;
};

} // namespace Internal
} // namespace Designer
