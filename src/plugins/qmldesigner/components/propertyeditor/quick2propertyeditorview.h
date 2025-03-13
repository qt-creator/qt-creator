// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QQuickWidget>

namespace QmlDesigner {

class InstanceImageProvider;

class Quick2PropertyEditorView : public QQuickWidget
{
    Q_OBJECT

public:
    explicit Quick2PropertyEditorView(class AsynchronousImageCache &imageCache);

    InstanceImageProvider *instanceImageProvider() const;

    static void registerQmlTypes();

private:
    InstanceImageProvider *m_instanceImageProvider = nullptr;
};

} //QmlDesigner
