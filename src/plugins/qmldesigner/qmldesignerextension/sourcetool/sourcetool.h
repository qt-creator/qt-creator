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
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
****************************************************************************/

#ifndef SOURCETOOL_H
#define SOURCETOOL_H

#include <abstractcustomtool.h>
#include "selectionindicator.h"

#include <QHash>
#include <QPointer>
#include <QFileDialog>

namespace QmlDesigner {

class SelectionContext;

class SourceTool : public QObject, public AbstractCustomTool
{
    Q_OBJECT
public:
    SourceTool();
    ~SourceTool();

    void mousePressEvent(const QList<QGraphicsItem*> &itemList,
                         QGraphicsSceneMouseEvent *event) override;
    void mouseMoveEvent(const QList<QGraphicsItem*> &itemList,
                        QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(const QList<QGraphicsItem*> &itemList,
                           QGraphicsSceneMouseEvent *event) override;
    void mouseDoubleClickEvent(const QList<QGraphicsItem*> &itemList,
                               QGraphicsSceneMouseEvent *event) override;
    void hoverMoveEvent(const QList<QGraphicsItem*> &itemList,
                        QGraphicsSceneMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *keyEvent) override;

    void dragLeaveEvent(const QList<QGraphicsItem*> &itemList,
                        QGraphicsSceneDragDropEvent * event) override;
    void dragMoveEvent(const QList<QGraphicsItem*> &itemList,
                       QGraphicsSceneDragDropEvent * event) override;

    void itemsAboutToRemoved(const QList<FormEditorItem*> &itemList) override;

    void selectedItemsChanged(const QList<FormEditorItem*> &itemList) override;

    void instancesCompleted(const QList<FormEditorItem*> &itemList) override;
    void instancesParentChanged(const QList<FormEditorItem *> &itemList) override;
    void instancePropertyChange(const QList<QPair<ModelNode, PropertyName> > &propertyList) override;

    void clear() override;

    void formEditorItemsChanged(const QList<FormEditorItem*> &itemList) override;

    int wantHandleItem(const ModelNode &modelNode) const override;

    QString name() const override;

private slots:
    void changeToSourceTool();

private:
    /* private methods */
    void fileSelected(const QString &fileName);


    /* members */
    QPointer<FormEditorItem> m_formEditorItem;
    QString m_oldFileName;
};

}
#endif // SOURCETOOL_H
