// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "tracing_global.h"

#include <QUrl>
#include <QWidget>

QT_BEGIN_NAMESPACE
class QAbstractItemModel;
QT_END_NAMESPACE

namespace Timeline {

class TRACING_EXPORT FlameGraphWidget : public QWidget
{
    Q_OBJECT
public:
    explicit FlameGraphWidget(QAbstractItemModel *model, const QUrl &qmlUrl,
                              QWidget *parent = nullptr);
    ~FlameGraphWidget() override;

    QAbstractItemModel *model() const;
    void selectByTypeId(int typeId);
    void resetRoot();
    bool isZoomed() const;

signals:
    void typeSelected(int typeId);
    void gotoSourceLocation(const QString &file, int line, int column);

private:
    class FlameGraphWidgetPrivate;
    FlameGraphWidgetPrivate *d;
};

} // namespace Timeline
