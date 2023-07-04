// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "designdocumentview.h"

#include <exception.h>
#include <rewriterview.h>
#include <basetexteditmodifier.h>
#include <modelmerger.h>

#include "designdocument.h"
#include <qmldesignerconstants.h>
#include <qmldesignerplugin.h>
#include <nodelistproperty.h>
#include <nodemetainfo.h>

#include <QApplication>
#include <QPlainTextEdit>
#include <QClipboard>
#include <QMimeData>
#include <QDebug>

#include <utils/algorithm.h>
#include <utils/qtcassert.h>

namespace QmlDesigner {

DesignDocumentView::DesignDocumentView(ExternalDependenciesInterface &externalDependencies)
    : AbstractView{externalDependencies}
    , m_modelMerger(new ModelMerger(this))
{
}

DesignDocumentView::~DesignDocumentView() = default;

ModelNode DesignDocumentView::insertModel(const ModelNode &modelNode)
{
    return m_modelMerger->insertModel(modelNode);
}

void DesignDocumentView::replaceModel(const ModelNode &modelNode)
{
    m_modelMerger->replaceModel(modelNode);
}

static QStringList arrayToStringList(const QByteArray &byteArray)
{
    QString str(QString::fromUtf8(byteArray));
    return str.split(QLatin1Char('\n'));
}

static QByteArray stringListToArray(const QStringList &stringList)
{
    QString str;
    for (const QString &subString : stringList)
        str += subString + QLatin1Char('\n');
    return str.toUtf8();
}

void DesignDocumentView::toClipboard() const
{
    QClipboard *clipboard = QApplication::clipboard();

    auto data = new QMimeData;

    data->setText(toText());
    QStringList imports;
    for (const Import &import : model()->imports())
        imports.append(import.toImportString());

    data->setData(QLatin1String("QmlDesigner::imports"), stringListToArray(imports));
    clipboard->setMimeData(data);
}

void DesignDocumentView::fromClipboard()
{
    QClipboard *clipboard = QApplication::clipboard();
    fromText(clipboard->text());
    QStringList imports = arrayToStringList(clipboard->mimeData()->data(QLatin1String("QmlDesigner::imports")));
//    for (const QString &importString, imports) {
//        Import import(Import::createLibraryImport();
//        model()->addImport(import); //### imports
//    }
}

static bool hasOnly3DNodes(const ModelNode &node)
{
    if (node.id() == "__multi__selection__") {
        bool hasNon3DNode = Utils::anyOf(node.directSubModelNodes(), [](const ModelNode &node) {
            return !node.metaInfo().isQtQuick3DNode();
        });

        return !hasNon3DNode;
    }
    return node.metaInfo().isQtQuick3DNode();
}

QString DesignDocumentView::toText() const
{
    auto outputModel = Model::create("QtQuick.Rectangle", 1, 0, model());
    outputModel->setFileUrl(model()->fileUrl());
    QPlainTextEdit textEdit;

    QString imports;
    for (const Import &import : model()->imports()) {
        if (import.isFileImport())
            imports += QStringLiteral("import ") + QStringLiteral("\"") + import.file() + QStringLiteral("\"")+ QStringLiteral(";\n");
        else
            imports += QStringLiteral("import ") + import.url() + QStringLiteral(" ") + import.version() + QStringLiteral(";\n");
    }

    textEdit.setPlainText(imports +  QStringLiteral("Item {\n}\n"));
    NotIndentingTextEditModifier modifier(&textEdit);

    QScopedPointer<RewriterView> rewriterView(
        new RewriterView(externalDependencies(), RewriterView::Amend));
    rewriterView->setCheckSemanticErrors(false);
    rewriterView->setPossibleImportsEnabled(false);
    rewriterView->setTextModifier(&modifier);
    outputModel->setRewriterView(rewriterView.data());

    ModelMerger merger(rewriterView.data());

    merger.replaceModel(rootModelNode());

    ModelNode rewriterNode(rewriterView->rootModelNode());

    rewriterView->writeAuxiliaryData();

    QString paste3DHeader = hasOnly3DNodes(rewriterNode) ? QString(Constants::HEADER_3DPASTE_CONTENT) : QString();
    return paste3DHeader
            + rewriterView->extractText({rewriterNode}).value(rewriterNode)
            + rewriterView->getRawAuxiliaryData();
    //get the text of the root item without imports
}

void DesignDocumentView::fromText(const QString &text)
{
    auto inputModel = Model::create("QtQuick.Rectangle", 1, 0, model());
    inputModel->setFileUrl(model()->fileUrl());
    QPlainTextEdit textEdit;
    QString imports;
    const auto modelImports = model()->imports();
    for (const Import &import : modelImports)
        imports += "import " + import.toString(true) + QLatin1Char(';') + QLatin1Char('\n');

    textEdit.setPlainText(imports + text);
    NotIndentingTextEditModifier modifier(&textEdit);

    RewriterView rewriterView{externalDependencies()};
    rewriterView.setCheckSemanticErrors(false);
    rewriterView.setPossibleImportsEnabled(false);
    rewriterView.setTextModifier(&modifier);
    inputModel->setRewriterView(&rewriterView);

    rewriterView.restoreAuxiliaryData();

    if (rewriterView.errors().isEmpty() && rewriterView.rootModelNode().isValid()) {
        try {
            replaceModel(rewriterView.rootModelNode());
        } catch(Exception &/*e*/) {
            /* e.showException(); Do not show any error if the clipboard contains invalid QML */
        }
    }
}

static Model *currentModel()
{
    DesignDocument *document = QmlDesignerPlugin::instance()->viewManager().currentDesignDocument();
    if (document)
        return document->currentModel();

    return nullptr;
}

ModelPointer DesignDocumentView::pasteToModel(ExternalDependenciesInterface &externalDependencies)
{
    Model *parentModel = currentModel();

    QTC_ASSERT(parentModel, return nullptr);

    auto pasteModel = Model::create("empty", 1, 0, parentModel);

    Q_ASSERT(pasteModel);

    if (!pasteModel)
        return nullptr;

    pasteModel->setFileUrl(parentModel->fileUrl());
    pasteModel->changeImports(parentModel->imports(), {});

    DesignDocumentView view{externalDependencies};
    pasteModel->attachView(&view);

    view.fromClipboard();

    return pasteModel;
}

void DesignDocumentView::copyModelNodes(const QList<ModelNode> &nodesToCopy,
                                        ExternalDependenciesInterface &externalDependencies)
{
    Model *parentModel = currentModel();

    QTC_ASSERT(parentModel, return);

    auto copyModel = Model::create("QtQuick.Rectangle", 1, 0, parentModel);

    copyModel->setFileUrl(parentModel->fileUrl());
    copyModel->changeImports(parentModel->imports(), {});

    Q_ASSERT(copyModel);

    QList<ModelNode> selectedNodes = nodesToCopy;

    if (selectedNodes.isEmpty())
        return;

    QList<ModelNode> nodeList = selectedNodes;
    for (int end = nodeList.length(), i = 0; i < end; ++i) {
        for (int j = 0; j < end; ++j) {
            if (nodeList.at(i).isAncestorOf(nodeList.at(j)))
                selectedNodes.removeAll(nodeList.at(j));
        }
    }

    DesignDocumentView view{externalDependencies};
    copyModel->attachView(&view);

    if (selectedNodes.size() == 1) {
        const ModelNode &selectedNode = selectedNodes.constFirst();

        if (!selectedNode.isValid())
            return;

        view.replaceModel(selectedNode);

        Q_ASSERT(view.rootModelNode().isValid());
        Q_ASSERT(view.rootModelNode().type() != "empty");

        view.toClipboard();
    } else { // multi items selected
        for (ModelNode node : view.rootModelNode().directSubModelNodes()) {
            node.destroy();
        }
        view.changeRootNodeType("QtQuick.Rectangle", 2, 0);
        view.rootModelNode().setIdWithRefactoring("__multi__selection__");

        for (const ModelNode &selectedNode : std::as_const(selectedNodes)) {
            ModelNode newNode(view.insertModel(selectedNode));
            view.rootModelNode().nodeListProperty("data").reparentHere(newNode);
        }

        view.toClipboard();
    }
}

}// namespace QmlDesigner
