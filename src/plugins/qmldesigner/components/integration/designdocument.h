// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <basetexteditmodifier.h>
#include <componenttextmodifier.h>
#include <coreplugin/icontext.h>
#include <model.h>
#include <projectstorage/projectstoragefwd.h>
#include <rewriterview.h>
#include <qmldesignercomponents_global.h>

#include <QObject>
#include <QString>
#include <QStackedWidget>

#include <memory>

QT_BEGIN_NAMESPACE
class QPlainTextEdit;
QT_END_NAMESPACE

namespace ProjectExplorer {
class Target;
}

namespace QmlDesigner {

class ModelNode;
class TextModifier;
class QmlObjectNode;
class CrumbleBarInfo;
class ViewManager;
class AbstractView;

class QMLDESIGNERCOMPONENTS_EXPORT DesignDocument : public QObject
{
    Q_OBJECT

public:
    DesignDocument(const QUrl &filePath,
                   ProjectStorageDependencies projectStorageDependencies,
                   ExternalDependenciesInterface &externalDependencies,
                   ModulesStorage &modulesStorage);
    ~DesignDocument() override;

    QString displayName() const;

    void loadDocument(Utils::PlainTextEdit *edit);
    void attachRewriterToModel();
    void close();
    bool isUndoAvailable() const;
    bool isRedoAvailable() const;
    void clearUndoRedoStacks() const;

    Model *currentModel() const;
    Model *documentModel() const;
    bool inFileComponentModelActive() const;

    void contextHelp(const Core::IContext::HelpCallback &callback) const;
    QList<DocumentMessage> qmlParseWarnings() const;
    bool hasQmlParseWarnings() const;
    QList<DocumentMessage> qmlParseErrors() const;
    bool hasQmlParseErrors() const;

    RewriterView *rewriterView() const;

    void setEditor(Core::IEditor *editor);
    Core::IEditor *editor() const;

    TextEditor::BaseTextEditor *textEditor() const;
    Utils::PlainTextEdit *plainTextEdit() const;
    Utils::FilePath fileName() const;
    ProjectExplorer::Target *currentTarget() const;
    bool isDocumentLoaded() const;

    void resetToDocumentModel();

    void changeToDocumentModel();

    bool isQtForMCUsProject() const;
    [[nodiscard]] QString defaultFontFamilyMCU() const;

    Utils::FilePath projectFolder() const;
    bool hasProject() const;

    void setModified();

signals:
    void displayNameChanged(const QString &newFileName);
    void dirtyStateChanged(bool newState);

    void undoAvailable(bool isAvailable);
    void redoAvailable(bool isAvailable);
    void designDocumentClosed();
    void qmlErrorsChanged(const QList<DocumentMessage> &errors);

public:
    void deleteSelected();
    void copySelected();
    void cutSelected();
    void duplicateSelected();
    void paste();
    void pasteToPosition(const std::optional<QVector3D> &position);
    void selectAll();
    void undo();
    void redo();
    void updateActiveTarget();
    void changeToSubComponent(const ModelNode &componentNode);
    void changeToMaster();

private: // functions
    void updateFileName(const Utils::FilePath &oldFileName, const Utils::FilePath &newFileName);

    void changeToInFileComponentModel(ComponentTextModifier *textModifer);

    void updateQrcFiles();

    QWidget *centralWidget() const;
    QString pathToQt() const;

    const ViewManager &viewManager() const;
    ViewManager &viewManager();

    ModelNode rootModelNode() const;

    bool loadInFileComponent(const ModelNode &componentNode);

    const AbstractView *view() const;

    ModelPointer createInFileComponentModel();

    bool pasteSVG();
    void moveNodesToPosition(const QList<ModelNode> &nodes, const std::optional<QVector3D> &position);

private: // variables
    ModelPointer m_documentModel;
    ModelPointer m_inFileComponentModel;
    QPointer<Core::IEditor> m_textEditor;
    std::unique_ptr<BaseTextEditModifier> m_documentTextModifier;
    std::unique_ptr<ComponentTextModifier> m_inFileComponentTextModifier;
    std::unique_ptr<RewriterView> m_rewriterView;
    bool m_documentLoaded;
    ProjectExplorer::Target *m_currentTarget;
    ProjectStorageDependencies m_projectStorageDependencies;
    ExternalDependenciesInterface &m_externalDependencies;
    ModulesStorage &m_modulesStorage;
};

} // namespace QmlDesigner
