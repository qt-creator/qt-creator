/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "designdocumentview.h"

#include <exception.h>
#include <rewriterview.h>
#include <basetexteditmodifier.h>
#include <modelmerger.h>

#include "designdocument.h"
#include <qmldesignerplugin.h>
#include <nodelistproperty.h>

#include <QApplication>
#include <QPlainTextEdit>
#include <QClipboard>
#include <QMimeData>
#include <QDebug>

#include <utils/qtcassert.h>

namespace QmlDesigner {

DesignDocumentView::DesignDocumentView(QObject *parent)
    : AbstractView(parent), m_modelMerger(new ModelMerger(this))
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
    foreach (const QString &subString, stringList)
        str += subString + QLatin1Char('\n');
    return str.toUtf8();
}

void DesignDocumentView::toClipboard() const
{
    QClipboard *clipboard = QApplication::clipboard();

    auto data = new QMimeData;

    data->setText(toText());
    QStringList imports;
    foreach (const Import &import, model()->imports())
        imports.append(import.toImportString());

    data->setData(QLatin1String("QmlDesigner::imports"), stringListToArray(imports));
    clipboard->setMimeData(data);
}

void DesignDocumentView::fromClipboard()
{
    QClipboard *clipboard = QApplication::clipboard();
    fromText(clipboard->text());
    QStringList imports = arrayToStringList(clipboard->mimeData()->data(QLatin1String("QmlDesigner::imports")));
//    foreach (const QString &importString, imports) {
//        Import import(Import::createLibraryImport();
//        model()->addImport(import); //### imports
//    }
}


QString DesignDocumentView::toText() const
{
    QScopedPointer<Model> outputModel(Model::create("QtQuick.Rectangle", 1, 0, model()));
    outputModel->setFileUrl(model()->fileUrl());
    QPlainTextEdit textEdit;

    QString imports;
    foreach (const Import &import, model()->imports()) {
        if (import.isFileImport())
            imports += QStringLiteral("import ") + QStringLiteral("\"") + import.file() + QStringLiteral("\"")+ QStringLiteral(";\n");
        else
            imports += QStringLiteral("import ") + import.url() + QStringLiteral(" ") + import.version() + QStringLiteral(";\n");
    }

    textEdit.setPlainText(imports +  QStringLiteral("Item {\n}\n"));
    NotIndentingTextEditModifier modifier(&textEdit);

    QScopedPointer<RewriterView> rewriterView(new RewriterView(RewriterView::Amend, nullptr));
    rewriterView->setCheckSemanticErrors(false);
    rewriterView->setTextModifier(&modifier);
    outputModel->setRewriterView(rewriterView.data());

    ModelMerger merger(rewriterView.data());

    merger.replaceModel(rootModelNode());

    ModelNode rewriterNode(rewriterView->rootModelNode());

    rewriterView->writeAuxiliaryData();
    return rewriterView->extractText({rewriterNode}).value(rewriterNode) + rewriterView->getRawAuxiliaryData();
    //get the text of the root item without imports
}

void DesignDocumentView::fromText(const QString &text)
{
    QScopedPointer<Model> inputModel(Model::create("QtQuick.Rectangle", 1, 0, model()));
    inputModel->setFileUrl(model()->fileUrl());
    QPlainTextEdit textEdit;
    QString imports;
    foreach (const Import &import, model()->imports())
        imports += QStringLiteral("import ") + import.toString(true) + QLatin1Char(';') + QLatin1Char('\n');

    textEdit.setPlainText(imports + text);
    NotIndentingTextEditModifier modifier(&textEdit);

    QScopedPointer<RewriterView> rewriterView(new RewriterView(RewriterView::Amend, nullptr));
    rewriterView->setCheckSemanticErrors(false);
    rewriterView->setTextModifier(&modifier);
    inputModel->setRewriterView(rewriterView.data());

    rewriterView->restoreAuxiliaryData();

    if (rewriterView->errors().isEmpty() && rewriterView->rootModelNode().isValid()) {
        ModelMerger merger(this);
        try {
            merger.replaceModel(rewriterView->rootModelNode());
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

Model *DesignDocumentView::pasteToModel()
{
    Model *parentModel = currentModel();

    QTC_ASSERT(parentModel, return nullptr);

    Model *pasteModel(Model::create("empty", 1, 0, parentModel));

    Q_ASSERT(pasteModel);

    if (!pasteModel)
        return nullptr;

    pasteModel->setFileUrl(parentModel->fileUrl());
    pasteModel->changeImports(parentModel->imports(), {});

    DesignDocumentView view;
    pasteModel->attachView(&view);

    view.fromClipboard();

    return pasteModel;
}

void DesignDocumentView::copyModelNodes(const QList<ModelNode> &nodesToCopy)
{
    Model *parentModel = currentModel();

    QTC_ASSERT(parentModel, return);

    QScopedPointer<Model> copyModel(Model::create("QtQuick.Rectangle", 1, 0, parentModel));

    copyModel->setFileUrl(parentModel->fileUrl());
    copyModel->changeImports(parentModel->imports(), {});

    Q_ASSERT(copyModel);

    QList<ModelNode> selectedNodes = nodesToCopy;

    if (selectedNodes.isEmpty())
        return;

    foreach (const ModelNode &node, selectedNodes) {
        foreach (const ModelNode &node2, selectedNodes) {
            if (node.isAncestorOf(node2))
                selectedNodes.removeAll(node2);
        }
    }

    DesignDocumentView view;
    copyModel->attachView(&view);

    if (selectedNodes.count() == 1) {
        const ModelNode &selectedNode = selectedNodes.constFirst();

        if (!selectedNode.isValid())
            return;

        view.replaceModel(selectedNode);

        Q_ASSERT(view.rootModelNode().isValid());
        Q_ASSERT(view.rootModelNode().type() != "empty");

        view.toClipboard();
    } else { //multi items selected

        foreach (ModelNode node, view.rootModelNode().directSubModelNodes()) {
            node.destroy();
        }
        view.changeRootNodeType("QtQuick.Rectangle", 2, 0);
        view.rootModelNode().setIdWithRefactoring("designer__Selection");

        foreach (const ModelNode &selectedNode, selectedNodes) {
            ModelNode newNode(view.insertModel(selectedNode));
            view.rootModelNode().nodeListProperty("data").reparentHere(newNode);
        }

        view.toClipboard();
    }

}

}// namespace QmlDesigner
