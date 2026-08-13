// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "tracing_global.h"
#include "timelinemodel.h"

#include <QPointer>
#include <QWidget>

QT_BEGIN_NAMESPACE
class QLabel;
class QStandardItemModel;
class QTreeView;
QT_END_NAMESPACE

namespace Timeline {

class TRACING_EXPORT RangeDetailsWidget : public QWidget
{
    Q_OBJECT
public:
    explicit RangeDetailsWidget(QWidget *parent = nullptr);

    // Several views share one panel: in the profiler every backend's timeline and
    // flame graph write into the same one. Each fill names the view it came from,
    // so a view can recognize its own content in provider() before acting on the
    // panel's signals, and so one view dropping its selection cannot wipe the
    // content another view put there.
    void setData(QObject *provider, const QString &title,
                 const QList<QPair<QString, QString>> &content);
    // Drops the content only if `provider` is the view that put it there.
    void clear(QObject *provider);
    // Drops the content whoever put it there, for when the trace itself goes away.
    void reset();
    QObject *provider() const;

signals:
    void recenterOnItem();
    void rowDoubleClicked(int row);

protected:
    void mousePressEvent(QMouseEvent *) override;
    void mouseReleaseEvent(QMouseEvent *) override;

private:
    void clearContent();
    void rebuildRows(const QList<QPair<QString, QString>> &content);

    QWidget *m_titleBar = nullptr;
    QLabel *m_titleLabel = nullptr;
    QTreeView *m_treeView = nullptr;
    QStandardItemModel *m_model = nullptr;
    QPointer<QObject> m_provider; // The view whose content is currently shown.

    bool m_hasData = false;
};

} // namespace Timeline
