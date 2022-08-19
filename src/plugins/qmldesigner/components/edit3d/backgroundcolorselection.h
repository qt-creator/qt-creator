// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <QByteArray>
#include <view3dactioncommand.h>

QT_FORWARD_DECLARE_CLASS(QColorDialog)

namespace QmlDesigner {

class BackgroundColorSelection : public QObject
{
    Q_OBJECT

public:
    explicit BackgroundColorSelection(QObject *parent = nullptr)
        : QObject{parent}
    {}

    static void showBackgroundColorSelectionWidget(QWidget *parent, const QByteArray &key,
                                                   View3DActionCommand::Type cmdType);

private:
    static QColorDialog *createColorDialog(QWidget *parent, const QByteArray &key,
                                           View3DActionCommand::Type cmdType);

    inline static QColorDialog *m_dialog = nullptr;
};

} // namespace QmlDesigner
