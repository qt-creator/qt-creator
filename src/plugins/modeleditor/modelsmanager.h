// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>

namespace ProjectExplorer {
class Project;
class Node;
}

namespace qmt {
class Uid;
class MDiagram;
class DiagramsViewInterface;
class MContainer;
class MReferences;
class DContainer;
class DReferences;
}

namespace ModelEditor {
namespace Internal {

class ExtDocumentController;
class DiagramsViewManager;
class ModelDocument;

class ModelsManager :
        public QObject
{
    Q_OBJECT
    class ManagedModel;
    class ModelsManagerPrivate;

public:
    explicit ModelsManager(QObject *parent = nullptr);
    ~ModelsManager();

signals:
    void modelClipboardChanged(bool isEmpty);
    void diagramClipboardChanged(bool isEmpty);

public:
    ExtDocumentController *createModel(ModelDocument *modelDocument);
    void releaseModel(ExtDocumentController *documentController);
    void openDiagram(const qmt::Uid &modelUid, const qmt::Uid &diagramUid);

    bool isModelClipboardEmpty() const;
    ExtDocumentController *modelClipboardDocumentController() const;
    qmt::MReferences modelClipboard() const;
    void setModelClipboard(ExtDocumentController *documentController, const qmt::MContainer &container);

    bool isDiagramClipboardEmpty() const;
    ExtDocumentController *diagramClipboardDocumentController() const;
    qmt::DReferences diagramClipboard() const;
    void setDiagramClipboard(ExtDocumentController *documentController, const qmt::DContainer &dcontainer,
                             const qmt::MContainer &mcontainer);

private:
    void onAboutToShowContextMenu(ProjectExplorer::Node *node);
    void onOpenDiagramFromProjectExplorer();
    void onOpenDefaultModel(const qmt::Uid &modelUid);

    void openDiagram(ExtDocumentController *documentController, qmt::MDiagram *diagram);

private:
    ModelsManagerPrivate *d;
};

} // namespace Internal
} // namespace ModelEditor
