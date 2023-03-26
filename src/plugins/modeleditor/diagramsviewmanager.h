// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>
#include "qmt/diagram_ui/diagramsviewinterface.h"

#include <QHash>

namespace qmt {
class MDiagram;
}

namespace ModelEditor {
namespace Internal {

class DiagramsViewManager :
        public QObject,
        public qmt::DiagramsViewInterface
{
    Q_OBJECT

public:
    explicit DiagramsViewManager(QObject *parent = nullptr);
    ~DiagramsViewManager() = default;

signals:
    void openNewDiagram(qmt::MDiagram *diagram);
    void closeOpenDiagram(const qmt::MDiagram *diagram);
    void closeAllOpenDiagrams();
    void diagramRenamed(const qmt::MDiagram *diagram);

public:
    void openDiagram(qmt::MDiagram *diagram) override;
    void closeDiagram(const qmt::MDiagram *diagram) override;
    void closeAllDiagrams() override;
    void onDiagramRenamed(const qmt::MDiagram *diagram) override;
};

} // namespace Internal
} // namespace ModelEditor
