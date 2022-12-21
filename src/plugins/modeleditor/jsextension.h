// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>

namespace ModelEditor {
namespace Internal {

class JsExtension : public QObject
{
    Q_OBJECT

public:
    JsExtension() {}

    Q_INVOKABLE QString fileNameToElementName(const QString &file);
    Q_INVOKABLE QString elementNameToFileName(const QString &element);
};

} // namespace Internal
} // namespace ModelEditor
