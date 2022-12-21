// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>

namespace ScxmlEditor {

namespace OutputPane {
class WarningModel;
} // namespace OutputPane

namespace PluginInterface {

class ScxmlTag;

class WarningProvider : public QObject
{
    Q_OBJECT

public:
    explicit WarningProvider(QObject *parent = nullptr)
        : QObject(parent)
    {
    }

    virtual void init(OutputPane::WarningModel *model) = 0;
};

} // namespace PluginInterface
} // namespace ScxmlEditor
