// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QWidget>

QT_FORWARD_DECLARE_CLASS(QLabel)
QT_FORWARD_DECLARE_CLASS(QToolButton)

namespace ScxmlEditor {

namespace PluginInterface { class ShapeProvider; }

namespace Common {

class ShapeGroupWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ShapeGroupWidget(PluginInterface::ShapeProvider *shapeProvider, int groupIndex, QWidget *parent = nullptr);

private:
    void createUi();

    QLabel *m_title = nullptr;
    QToolButton *m_closeButton = nullptr;
    QWidget *m_content = nullptr;
};

} // namespace Common
} // namespace ScxmlEditor
