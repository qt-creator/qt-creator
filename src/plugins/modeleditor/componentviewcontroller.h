// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>

namespace ProjectExplorer { class FolderNode; }

namespace qmt {
class MPackage;
class MDiagram;
class DiagramSceneController;
}

namespace ModelEditor {
namespace Internal {

class ModelUtilities;
class PackageViewController;
class PxNodeUtilities;

class ComponentViewController :
        public QObject
{
    Q_OBJECT
    class ComponentViewControllerPrivate;

public:
    explicit ComponentViewController(QObject *parent = nullptr);
    ~ComponentViewController();

    void setModelUtilities(ModelUtilities *modelUtilities);
    void setPackageViewController(PackageViewController *packageViewController);
    void setPxNodeUtilties(PxNodeUtilities *pxnodeUtilities);
    void setDiagramSceneController(qmt::DiagramSceneController *diagramSceneController);

    void createComponentModel(const QString &filePath,
                              qmt::MDiagram *diagram, const QString &anchorFolder);
    void updateIncludeDependencies(qmt::MPackage *rootPackage);

private:
    void doCreateComponentModel(const QString &filePath, qmt::MDiagram *diagram,
                                const QString &anchorFolder, bool scanHeaders);

    ComponentViewControllerPrivate *d;
};

} // namespace Internal
} // namespace ModelEditor
