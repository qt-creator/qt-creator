// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QtWidgets/qwidget.h>
#include <QtGui/qpixmap.h>

#include <memory>

namespace QmlDesigner {
namespace Ui {
class PreviewImageTooltip;
}

class PreviewImageTooltip : public QWidget
{
    Q_OBJECT

public:
    explicit PreviewImageTooltip(QWidget *parent = {});
    ~PreviewImageTooltip();

    void setName(const QString &name);
    void setPath(const QString &path);
    void setInfo(const QString &info);
    void setImage(const QImage &image, bool scale = true);

private:
    std::unique_ptr<Ui::PreviewImageTooltip> m_ui;
};
}
