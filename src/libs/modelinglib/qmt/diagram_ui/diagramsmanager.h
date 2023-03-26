// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
    void diagramActivated(const qmt::MDiagram *diagram);
    void diagramSelectionChanged(const qmt::MDiagram *diagram);

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
