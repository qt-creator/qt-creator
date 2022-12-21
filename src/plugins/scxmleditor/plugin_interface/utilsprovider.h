// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QGraphicsItem>
#include <QList>
#include <QObject>

namespace ScxmlEditor {

namespace PluginInterface {

class ScxmlTag;

class UtilsProvider : public QObject
{
    Q_OBJECT

public:
    explicit UtilsProvider(QObject *parent = nullptr);

    virtual void checkInitialState(const QList<QGraphicsItem*> &items, ScxmlTag *tag) = 0;
};

} // namespace PluginInterface
} // namespace ScxmlEditor
