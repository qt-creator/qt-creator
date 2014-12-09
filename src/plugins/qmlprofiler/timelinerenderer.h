/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
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
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef TIMELINERENDERER_H
#define TIMELINERENDERER_H

#include <QSGTransformNode>
#include <QQuickItem>
#include "timelinezoomcontrol.h"
#include "timelinemodel.h"
#include "timelinenotesmodel.h"
#include "timelinerenderpass.h"

namespace Timeline {

class TimelineRenderPass;
class TimelineRenderState;

class TimelineRenderer : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(Timeline::TimelineModel *model READ model WRITE setModel NOTIFY modelChanged)
    Q_PROPERTY(Timeline::TimelineZoomControl *zoomer READ zoomer WRITE setZoomer NOTIFY zoomerChanged)
    Q_PROPERTY(Timeline::TimelineNotesModel *notes READ notes WRITE setNotes NOTIFY notesChanged)
    Q_PROPERTY(bool selectionLocked READ selectionLocked WRITE setSelectionLocked NOTIFY selectionLockedChanged)
    Q_PROPERTY(int selectedItem READ selectedItem WRITE setSelectedItem NOTIFY selectedItemChanged)

public:

    explicit TimelineRenderer(QQuickItem *parent = 0);

    bool selectionLocked() const
    {
        return m_selectionLocked;
    }

    int selectedItem() const
    {
        return m_selectedItem;
    }

    TimelineModel *model() const { return m_model; }
    void setModel(TimelineModel *model);

    TimelineZoomControl *zoomer() const { return m_zoomer; }
    void setZoomer(TimelineZoomControl *zoomer);

    TimelineNotesModel *notes() const { return m_notes; }
    void setNotes(TimelineNotesModel *notes);

    bool modelDirty() const;
    bool notesDirty() const;
    bool rowHeightsDirty() const;

    Q_INVOKABLE void selectNextFromSelectionId(int selectionId);
    Q_INVOKABLE void selectPrevFromSelectionId(int selectionId);

    // TODO: We could add some Q_INVOKABLE functions to enable or disable render passes when the the
    // need arises.

signals:
    void modelChanged(const TimelineModel *model);
    void zoomerChanged(TimelineZoomControl *zoomer);
    void notesChanged(TimelineNotesModel *notes);

    void selectionLockedChanged(bool locked);
    void selectedItemChanged(int itemIndex);
    void itemPressed(int pressedItem);

public slots:
    void clearData();

    void setSelectedItem(int itemIndex)
    {
        if (m_selectedItem != itemIndex) {
            m_selectedItem = itemIndex;
            update();
            emit selectedItemChanged(itemIndex);
        }
    }

    void setSelectionLocked(bool locked)
    {
        if (m_selectionLocked != locked) {
            m_selectionLocked = locked;
            update();
            emit selectionLockedChanged(locked);
        }
    }

private slots:
    void setModelDirty();
    void setRowHeightsDirty();
    void setNotesDirty();
    void setRowCountsDirty();

protected:
    virtual QSGNode *updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *updatePaintNodeData);
    virtual void mousePressEvent(QMouseEvent *event);
    virtual void mouseReleaseEvent(QMouseEvent *event);
    virtual void mouseMoveEvent(QMouseEvent *event);
    virtual void hoverMoveEvent(QHoverEvent *event);

private:
    int rowFromPosition(int y);

    void manageClicked();
    void manageHovered(int mouseX, int mouseY);

    static const int SafeFloatMax = 1 << 12;

    void resetCurrentSelection();

    TimelineRenderState *findRenderState();

    TimelineModel *m_model;
    TimelineZoomControl *m_zoomer;
    TimelineNotesModel *m_notes;

    struct {
        qint64 startTime;
        qint64 endTime;
        int row;
        int eventIndex;
    } m_currentSelection;

    int m_selectedItem;
    bool m_selectionLocked;
    bool m_modelDirty;
    bool m_rowHeightsDirty;
    bool m_notesDirty;
    bool m_rowCountsDirty;

    QList<const TimelineRenderPass *> m_renderPasses;
    QVector<QVector<TimelineRenderState *> > m_renderStates;
    TimelineRenderState *m_lastState;
};

} // namespace Timeline

QML_DECLARE_TYPE(Timeline::TimelineRenderer)

#endif // TIMELINERENDERER_H
