// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QWidget>
#include <QIcon>

namespace ModelEditor {
namespace Internal {

class DragTool :
        public QWidget
{
    Q_OBJECT
    class DragToolPrivate;

public:
    DragTool(const QIcon &icon, const QString &title, const QString &newElementName,
             const QString &newElementId, const QString &stereotype, QWidget *parent = nullptr);
    ~DragTool();

    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;

private:
    DragToolPrivate *d;
};

} // namespace Internal
} // namespace ModelEditor
