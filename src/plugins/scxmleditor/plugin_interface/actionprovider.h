// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>

QT_FORWARD_DECLARE_CLASS(QMenu)

namespace ScxmlEditor {

namespace PluginInterface {

class ScxmlTag;

class ActionProvider : public QObject
{
    Q_OBJECT

public:
    explicit ActionProvider(QObject *parent = nullptr)
        : QObject(parent)
    {
    }

    virtual void initStateMenu(const ScxmlTag *tag, QMenu *menu) = 0;
};

} // namespace PluginInterface
} // namespace ScxmlEditor
