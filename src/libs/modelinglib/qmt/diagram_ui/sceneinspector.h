// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>
#include "qmt/tasks/isceneinspector.h"
#include "qmt/infrastructure/qmt_global.h"

namespace qmt {

class DiagramsManager;

class QMT_EXPORT SceneInspector : public QObject, public ISceneInspector
{
public:
    explicit SceneInspector(QObject *parent = nullptr);
    ~SceneInspector() override;

    void setDiagramsManager(DiagramsManager *diagramsManager);

    QSizeF rasterSize() const override;
    QSizeF minimalSize(const DElement *element, const MDiagram *diagram) const override;
    IMoveable *moveable(const DElement *element, const MDiagram *diagram) const override;
    IResizable *resizable(const DElement *element, const MDiagram *diagram) const override;

private:
    DiagramsManager *m_diagramsManager = nullptr;
};

} // namespace qmt
