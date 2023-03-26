// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QTabWidget>

#include "qmt/infrastructure/uid.h"
#include "qmt/diagram_ui/diagramsviewinterface.h"

#include <QPointer>
#include <QHash>

namespace qmt {

class MDiagram;

class DiagramView;
class DiagramSceneModel;
class DiagramsManager;

class QMT_EXPORT DiagramsView : public QTabWidget, public DiagramsViewInterface
{
    Q_OBJECT

public:
    explicit DiagramsView(QWidget *parent = nullptr);
    ~DiagramsView() override;

signals:
    void currentDiagramChanged(const qmt::MDiagram *diagram);
    void diagramCloseRequested(const qmt::MDiagram *diagram);
    void someDiagramOpened(bool);

public:
    void setDiagramsManager(DiagramsManager *diagramsManager);

    void openDiagram(MDiagram *diagram) override;
    void closeDiagram(const MDiagram *diagram) override;
    void closeAllDiagrams() override;
    void onDiagramRenamed(const MDiagram *diagram) override;

private:
    void onCurrentChanged(int tabIndex);
    void onTabCloseRequested(int tabIndex);

    MDiagram *diagram(int tabIndex) const;
    MDiagram *diagram(DiagramView * diagramView) const;

    DiagramsManager *m_diagramsManager = nullptr;
    QHash<Uid, DiagramView *> m_diagramViews;
};

} // namespace qmt
