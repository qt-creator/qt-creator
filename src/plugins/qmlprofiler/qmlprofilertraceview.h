/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef QMLPROFILERTRACEVIEW_H
#define QMLPROFILERTRACEVIEW_H

#include "qmlprofilermodelmanager.h"

#include <coreplugin/find/ifindsupport.h>

#include <QTimer>
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
    explicit QmlProfilerTraceView(QWidget *parent, QmlProfilerTool *profilerTool,
                                  QmlProfilerViewManager *container,
                                  QmlProfilerModelManager *modelManager);
    ~QmlProfilerTraceView();

    bool hasValidSelection() const;
    qint64 selectionStart() const;
    qint64 selectionEnd() const;
    void showContextMenu(QPoint position);

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

#endif // QMLPROFILERTRACEVIEW_H

