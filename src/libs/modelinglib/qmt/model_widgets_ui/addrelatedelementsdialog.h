// Copyright (C) 2018 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmt/infrastructure/qmt_global.h"

#include <QDialog>

namespace qmt {

class DSelection;
class DObject;
class MObject;
class MDiagram;
class MRelation;
class DiagramSceneController;
}

namespace qmt {

class QMT_EXPORT AddRelatedElementsDialog : public QDialog
{
    Q_OBJECT
    class Private;

public:
    explicit AddRelatedElementsDialog(QWidget *parent = nullptr);
    ~AddRelatedElementsDialog();

    void setDiagramSceneController(qmt::DiagramSceneController *diagramSceneController);
    void setElements(const qmt::DSelection &selection, qmt::MDiagram *diagram);

private:
    void onAccepted();
    void updateFilter();
    bool filter(qmt::DObject *dobject, qmt::MObject *mobject, qmt::MRelation *relation);
    void updateNumberOfElements();

    Private *d = nullptr;
};

} // namespace qmt
