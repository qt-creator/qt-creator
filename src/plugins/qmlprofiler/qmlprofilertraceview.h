// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmlprofilermodelmanager.h"

#include <QWidget>

namespace Timeline { class RangeDetailsWidget; }

namespace Profiler::Internal {

class QmlProfilerModelManager;

class QmlProfilerTraceView : public QWidget
{
    Q_OBJECT

public:
    explicit QmlProfilerTraceView(QWidget *parent, QmlProfilerModelManager *modelManager);
    ~QmlProfilerTraceView() override;

    Timeline::RangeDetailsWidget *rangeDetailsWidget() const;

    bool hasValidSelection() const;
    qint64 selectionStart() const;
    qint64 selectionEnd() const;
    void showContextMenu(QPoint position);
    bool isSuspended() const;

    void clear();
    void selectByTypeId(int typeId);
    void selectByEventIndex(int modelId, int eventIndex);
    void updateCursorPosition();

protected:
    void contextMenuEvent(QContextMenuEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

signals:
    void gotoSourceLocation(const QString &fileUrl, int lineNumber, int columNumber);
    void typeSelected(int typeId);

private:
    class QmlProfilerTraceViewPrivate;
    QmlProfilerTraceViewPrivate *d;
};

} // namespace Profiler::Internal
