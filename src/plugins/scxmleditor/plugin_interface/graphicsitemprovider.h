// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>

namespace ScxmlEditor {

namespace PluginInterface {

class BaseItem;
class WarningItem;

class GraphicsItemProvider : public QObject
{
    Q_OBJECT

public:
    explicit GraphicsItemProvider(QObject *parent = nullptr)
        : QObject(parent)
    {
    }

    virtual WarningItem *createWarningItem(const QString &key, BaseItem *parentItem) const = 0;
};

} // namespace PluginInterface
} // namespace ScxmlEditor
