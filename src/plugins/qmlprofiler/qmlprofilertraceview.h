// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmlprofilermodelmanager.h"
#include <coreplugin/find/ifindsupport.h>
#include <QWidget>

namespace QmlProfiler {

class QmlProfilerModelManager;
class QmlProfilerStateManager;

namespace Internal {

class QmlProfilerTool;
class QmlProfilerViewManager;

class QmlProfilerTraceView : public QWidget
{
    Q_OBJECT

public:
    explicit QmlProfilerTraceView(QWidget *parent, QmlProfilerViewManager *container,
                                  QmlProfilerModelManager *modelManager);
    ~QmlProfilerTraceView() override;

    bool hasValidSelection() const;
    qint64 selectionStart() const;
    qint64 selectionEnd() const;
    void showContextMenu(QPoint position);
    bool isUsable() const;
    bool isSuspended() const;

    void clear();
    void selectByTypeId(int typeId);
    void selectByEventIndex(int modelId, int eventIndex);
    void updateCursorPosition();

protected:
    void changeEvent(QEvent *e) override;
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

class TraceViewFindSupport : public Core::IFindSupport
{
    Q_OBJECT

public:
    TraceViewFindSupport(QmlProfilerTraceView *view, QmlProfilerModelManager *manager);

    bool supportsReplace() const override;
    Utils::FindFlags supportedFindFlags() const override;
    void resetIncrementalSearch() override;
    void clearHighlights() override;
    QString currentFindString() const override;
    QString completedFindString() const override;
    Result findIncremental(const QString &txt, Utils::FindFlags findFlags) override;
    Result findStep(const QString &txt, Utils::FindFlags findFlags) override;

private:
    bool find(const QString &txt, Utils::FindFlags findFlags, int start, bool *wrapped);
    bool findOne(const QString &txt, Utils::FindFlags findFlags, int start);

    QmlProfilerTraceView *m_view;
    QmlProfilerModelManager *m_modelManager;
    int m_incrementalStartPos = -1;
    bool m_incrementalWrappedState = false;
    int m_currentPosition = -1;
};

} // namespace Internal
} // namespace QmlProfiler
