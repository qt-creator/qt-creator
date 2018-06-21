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
    void currentDiagramChanged(const MDiagram *diagram);
    void diagramCloseRequested(const MDiagram *diagram);
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
