// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QMap>
#include <QObject>

namespace ScxmlEditor {

namespace PluginInterface {

class ScxmlNamespace : public QObject
{
public:
    ScxmlNamespace(const QString &prefix, const QString &name, QObject *parent = nullptr);

    QString prefix() const;
    QString name() const;

    bool isTagVisible(const QString &tag) const;
    void setTagVisibility(const QString &tag, bool visible);

private:
    QString m_prefix, m_name;
    QMap<QString, bool> m_tagVisibility;
};

} // namespace PluginInterface
} // namespace ScxmlEditor
