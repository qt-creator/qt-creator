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

#ifndef TIMELINEABSTRACTRENDERER_H
#define TIMELINEABSTRACTRENDERER_H

#include "timelinezoomcontrol.h"
#include "timelinemodel.h"
#include "timelinenotesmodel.h"
#include "timelinerenderpass.h"

#include <QSGTransformNode>
#include <QQuickItem>

namespace Timeline {

class TimelineRenderPass;
class TimelineRenderState;

class TIMELINE_EXPORT TimelineAbstractRenderer : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(Timeline::TimelineModel *model READ model WRITE setModel NOTIFY modelChanged)
    Q_PROPERTY(Timeline::TimelineNotesModel *notes READ notes WRITE setNotes NOTIFY notesChanged)
    Q_PROPERTY(Timeline::TimelineZoomControl *zoomer READ zoomer WRITE setZoomer NOTIFY zoomerChanged)
    Q_PROPERTY(bool selectionLocked READ selectionLocked WRITE setSelectionLocked NOTIFY selectionLockedChanged)
    Q_PROPERTY(int selectedItem READ selectedItem WRITE setSelectedItem NOTIFY selectedItemChanged)

public:
    TimelineAbstractRenderer(QQuickItem *parent = 0);
    ~TimelineAbstractRenderer();

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

signals:
    void modelChanged(TimelineModel *model);
    void notesChanged(TimelineNotesModel *notes);
    void zoomerChanged(TimelineZoomControl *zoomer);
    void selectionLockedChanged(bool locked);
    void selectedItemChanged(int itemIndex);

public slots:
    void setSelectedItem(int itemIndex);
    void setSelectionLocked(bool locked);

    void setModelDirty();
    void setNotesDirty();
    void setRowHeightsDirty();

protected:
    QSGNode *updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *updatePaintNodeData);

    class TimelineAbstractRendererPrivate;
    TimelineAbstractRenderer(TimelineAbstractRendererPrivate &dd, QQuickItem *parent = 0);
    TimelineAbstractRendererPrivate *d_ptr;
    Q_DECLARE_PRIVATE(TimelineAbstractRenderer)
};

} // namespace Timeline


#endif // TIMELINEABSTRACTRENDERER_H
