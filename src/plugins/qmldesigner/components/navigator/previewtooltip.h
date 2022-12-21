// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QtWidgets/qwidget.h>
#include <QtGui/qpixmap.h>

namespace QmlDesigner {
namespace Ui {
class PreviewToolTip;
}

class PreviewToolTip : public QWidget
{
    Q_OBJECT

public:
    explicit PreviewToolTip(QWidget *parent = nullptr);
    ~PreviewToolTip();

    void setId(const QString &id);
    void setType(const QString &type);
    void setInfo(const QString &info);
    void setPixmap(const QPixmap &pixmap);

    QString id() const;

private:
    Ui::PreviewToolTip *m_ui;
};
}
