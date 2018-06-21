/****************************************************************************
**
** Copyright (C) 2016 Jochen Becher
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

#include <QObject>

#include "qmt/infrastructure/uid.h"

#include <QHash>
#include <QPointer>

QT_BEGIN_NAMESPACE
class QModelIndex;
QT_END_NAMESPACE

namespace qmt {

class MDiagram;
class DiagramController;
class DiagramSceneController;
class StyleController;
class StereotypeController;
class DSelection;
class DElement;

class TreeModel;
class DiagramsViewInterface;
class DiagramSceneModel;

class QMT_EXPORT DiagramsManager : public QObject
{
    Q_OBJECT
    class ManagedDiagram;

public:
    explicit DiagramsManager(QObject *parent = nullptr);
    ~DiagramsManager() override;

signals:
    void someDiagramOpened(bool);
    void diagramActivated(const MDiagram *diagram);
    void diagramSelectionChanged(const MDiagram *diagram);

public:
    void setModel(TreeModel *model);
    void setDiagramsView(DiagramsViewInterface *diagramsView);
    void setDiagramController(DiagramController *diagramController);
    void setDiagramSceneController(DiagramSceneController *diagramSceneController);
    void setStyleController(StyleController *styleController);
    void setStereotypeController(StereotypeController *stereotypeController);

    DiagramSceneModel *bindDiagramSceneModel(MDiagram *diagram);
    DiagramSceneModel *diagramSceneModel(const MDiagram *diagram) const;
    void unbindDiagramSceneModel(const MDiagram *diagram);

    void openDiagram(MDiagram *diagram);
    void removeDiagram(const MDiagram *diagram);
    void removeAllDiagrams();

private:
    void onDataChanged(const QModelIndex &topleft, const QModelIndex &bottomright);

    QPointer<TreeModel> m_model;
    DiagramsViewInterface *m_diagramsView = nullptr;
    DiagramController *m_diagramController = nullptr;
    DiagramSceneController *m_diagramSceneController = nullptr;
    StyleController *m_styleController = nullptr;
    StereotypeController *m_stereotypeController = nullptr;
    QHash<Uid, ManagedDiagram *> m_diagramUidToManagedDiagramMap;
};

} // namespace qmt
