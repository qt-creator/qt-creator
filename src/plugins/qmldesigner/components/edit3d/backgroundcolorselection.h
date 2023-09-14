// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <auxiliarydata.h>

#include <QByteArray>
#include <QObject>

QT_FORWARD_DECLARE_CLASS(QColorDialog)

namespace QmlDesigner {
class AbstractView;

inline constexpr AuxiliaryDataKeyView edit3dGridColorProperty{AuxiliaryDataType::NodeInstanceAuxiliary,
                                                              "edit3dGridColor"};
inline constexpr AuxiliaryDataKeyView edit3dBgColorProperty{AuxiliaryDataType::NodeInstanceAuxiliary,
                                                            "edit3dBgColor"};

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
                                                   const AuxiliaryDataKeyView &auxProp,
                                                   const std::function<void()> &colorSelected = {});

private:
    static QColorDialog *createColorDialog(QWidget *parent,
                                           const QByteArray &key,
                                           AbstractView *view,
                                           const AuxiliaryDataKeyView &auxProp,
                                           const std::function<void ()> &colorSelected);

    inline static QColorDialog *m_dialog = nullptr;
};

} // namespace QmlDesigner
