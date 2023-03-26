// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <nodeinstanceglobal.h>

#include <QByteArray>
#include <QObject>

QT_FORWARD_DECLARE_CLASS(QColorDialog)

namespace QmlDesigner {
class AbstractView;

class BackgroundColorSelection : public QObject
{
    Q_OBJECT

public:
    explicit BackgroundColorSelection(QObject *parent = nullptr)
        : QObject{parent}
    {}

    static void showBackgroundColorSelectionWidget(QWidget *parent,
                                                   const QByteArray &key,
                                                   AbstractView *view,
                                                   View3DActionType actionType,
                                                   const std::function<void()> &colorSelected = {});

private:
    static QColorDialog *createColorDialog(QWidget *parent,
                                           const QByteArray &key,
                                           AbstractView *view,
                                           View3DActionType actionType,
                                           const std::function<void ()> &colorSelected);

    inline static QColorDialog *m_dialog = nullptr;
};

} // namespace QmlDesigner
