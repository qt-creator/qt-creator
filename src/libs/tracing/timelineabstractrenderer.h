// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "timelinezoomcontrol.h"
#include "timelinemodel.h"
#include "timelinenotesmodel.h"
#include "timelinerenderpass.h"

#include <QPointer>
#include <QSGTransformNode>
#include <QQuickItem>

namespace Timeline {

class TRACING_EXPORT TimelineAbstractRenderer : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(Timeline::TimelineModel *model READ model WRITE setModel NOTIFY modelChanged)
    Q_PROPERTY(Timeline::TimelineNotesModel *notes READ notes WRITE setNotes NOTIFY notesChanged)
    Q_PROPERTY(Timeline::TimelineZoomControl *zoomer READ zoomer WRITE setZoomer NOTIFY zoomerChanged)
    Q_PROPERTY(bool selectionLocked READ selectionLocked WRITE setSelectionLocked NOTIFY selectionLockedChanged)
    Q_PROPERTY(int selectedItem READ selectedItem WRITE setSelectedItem NOTIFY selectedItemChanged)

public:
    explicit TimelineAbstractRenderer(QQuickItem *parent = nullptr);

    bool selectionLocked() const;
    int selectedItem() const;

    TimelineModel *model() const;
    void setModel(TimelineModel *model);

    TimelineNotesModel *notes() const;
    void setNotes(TimelineNotesModel *notes);

    TimelineZoomControl *zoomer() const;
    void setZoomer(TimelineZoomControl *zoomer);

    bool modelDirty() const;
    bool notesDirty() const;
    bool rowHeightsDirty() const;

    void setSelectedItem(int itemIndex);
    void setSelectionLocked(bool locked);

    void setModelDirty();
    void setNotesDirty();
    void setRowHeightsDirty();

signals:
    void modelChanged(TimelineModel *model);
    void notesChanged(TimelineNotesModel *notes);
    void zoomerChanged(TimelineZoomControl *zoomer);
    void selectionLockedChanged(bool locked);
    void selectedItemChanged(int itemIndex);

protected:
    QSGNode *updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *updatePaintNodeData) override;

    int m_selectedItem = -1;
    bool m_selectionLocked = true;
    QPointer<TimelineModel> m_model;
    QPointer<TimelineNotesModel> m_notes;
    QPointer<TimelineZoomControl> m_zoomer;
    bool m_modelDirty = false;
    bool m_rowHeightsDirty = false;
    bool m_notesDirty = false;
    QList<const TimelineRenderPass *> m_renderPasses;
};

} // namespace Timeline
