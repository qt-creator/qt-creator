// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QGraphicsView>

QT_BEGIN_NAMESPACE
class QAbstractItemModel;
QT_END_NAMESPACE

namespace Valgrind::Callgrind { class Function; }

namespace Valgrind::Internal {

class Visualization : public QGraphicsView
{
    Q_OBJECT

public:
    explicit Visualization(QWidget *parent = nullptr);
    ~Visualization() override;

    void setModel(QAbstractItemModel *model);

    const Valgrind::Callgrind::Function *functionForItem(QGraphicsItem *item) const;
    QGraphicsItem *itemForFunction(const Valgrind::Callgrind::Function *function) const;

    void setFunction(const Valgrind::Callgrind::Function *function);
    const Valgrind::Callgrind::Function *function() const;

    void setMinimumInclusiveCostRatio(double ratio);

    void setText(const QString &message);

signals:
    void functionActivated(const Valgrind::Callgrind::Function *);
    void functionSelected(const Valgrind::Callgrind::Function *);

protected:
    void populateScene();
    void mousePressEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    class Private;
    Private *d;
};

} // namespace Valgrind::Internal
