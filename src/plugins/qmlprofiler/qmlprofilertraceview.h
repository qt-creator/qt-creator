/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

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
    ~QmlProfilerTraceView();

    bool hasValidSelection() const;
    qint64 selectionStart() const;
    qint64 selectionEnd() const;
    void showContextMenu(QPoint position);
    bool isUsable() const;

public slots:
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
    Core::FindFlags supportedFindFlags() const override;
    void resetIncrementalSearch() override;
    void clearHighlights() override;
    QString currentFindString() const override;
    QString completedFindString() const override;
    Result findIncremental(const QString &txt, Core::FindFlags findFlags) override;
    Result findStep(const QString &txt, Core::FindFlags findFlags) override;

private:
    bool find(const QString &txt, Core::FindFlags findFlags, int start, bool *wrapped);
    bool findOne(const QString &txt, Core::FindFlags findFlags, int start);

    QmlProfilerTraceView *m_view;
    QmlProfilerModelManager *m_modelManager;
    int m_incrementalStartPos = -1;
    bool m_incrementalWrappedState = false;
    int m_currentPosition = -1;
};

} // namespace Internal
} // namespace QmlProfiler
