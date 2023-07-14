// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmt/infrastructure/qmt_global.h"

#include <QObject>

namespace qmt {

class TreeModel;
class ModelTreeViewInterface;
class MObject;
class MPackage;
class MSelection;

class QMT_EXPORT TreeModelManager : public QObject
{
public:
    explicit TreeModelManager(QObject *parent = nullptr);
    ~TreeModelManager() override;

    TreeModel *treeModel() const { return m_treeModel; }
    void setTreeModel(TreeModel *treeModel);
    ModelTreeViewInterface *modelTreeView() const { return m_modelTreeView; }
    void setModelTreeView(ModelTreeViewInterface *modelTreeView);

    bool isRootPackageSelected() const;
    MObject *selectedObject() const;
    MPackage *selectedPackage() const;
    MSelection selectedObjects() const;

private:
    TreeModel *m_treeModel = nullptr;
    ModelTreeViewInterface *m_modelTreeView = nullptr;
};

} // namespace qmt
