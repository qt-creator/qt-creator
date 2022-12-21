// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>

namespace ScxmlEditor {

namespace PluginInterface {

class ScxmlTag;

class ImageProvider : public QObject
{
    Q_OBJECT

public:
    explicit ImageProvider(QObject *parent = nullptr);

    virtual QImage *backgroundImage(const ScxmlTag *tag) const = 0;
};

} // namespace PluginInterface
} // namespace ScxmlEditor
