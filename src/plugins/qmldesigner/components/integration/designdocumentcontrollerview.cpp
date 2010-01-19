/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "designdocumentcontrollerview.h"
#include <rewriterview.h>
#include <plaintexteditmodifier.h>
#include <metainfo.h>

#include <QApplication>
#include <QPlainTextEdit>
#include <QClipboard>
#include <QDebug>

namespace QmlDesigner {

void DesignDocumentControllerView::nodeCreated(const ModelNode & /*createdNode*/) {};
void DesignDocumentControllerView::nodeAboutToBeRemoved(const ModelNode & /*removedNode*/) {};
void DesignDocumentControllerView::nodeRemoved(const ModelNode & /*removedNode*/, const NodeAbstractProperty & /*parentProperty*/, AbstractView::PropertyChangeFlags /*propertyChange*/) {};
void DesignDocumentControllerView::nodeReparented(const ModelNode & /*node*/, const NodeAbstractProperty & /*newPropertyParent*/, const NodeAbstractProperty & /*oldPropertyParent*/, AbstractView::PropertyChangeFlags /*propertyChange*/) {};
void DesignDocumentControllerView::nodeIdChanged(const ModelNode& /*node*/, const QString& /*newId*/, const QString& /*oldId*/) {};
void DesignDocumentControllerView::propertiesAboutToBeRemoved(const QList<AbstractProperty>& /*propertyList*/) {};
void DesignDocumentControllerView::propertiesRemoved(const QList<AbstractProperty>& /*propertyList*/) {};
void DesignDocumentControllerView::variantPropertiesChanged(const QList<VariantProperty>& /*propertyList*/, AbstractView::PropertyChangeFlags /*propertyChange*/) {};
void DesignDocumentControllerView::bindingPropertiesChanged(const QList<BindingProperty>& /*propertyList*/, AbstractView::PropertyChangeFlags /*propertyChange*/) {};
void DesignDocumentControllerView::nodeTypeChanged(const ModelNode & /*node*/,const QString & /*type*/, int /*majorVersion*/, int /*minorVersion*/) {};

void DesignDocumentControllerView::selectedNodesChanged(const QList<ModelNode> & /*selectedNodeList*/,
                          const QList<ModelNode> & /*lastSelectedNodeList*/) {};

void DesignDocumentControllerView::nodeOrderChanged(const NodeListProperty & /*listProperty*/, const ModelNode & /*movedNode*/, int /*oldIndex*/) {};

static QStringList arrayToStringList(const QByteArray &byteArray)
{
    QString str(QString::fromLatin1(byteArray));
    return str.split("\n");
}

static QByteArray stringListToArray(const QStringList &stringList)
{
    QString str;
    foreach (const QString &subString, stringList)
        str += subString + "\n";
    return str.toLatin1();
}

void DesignDocumentControllerView::toClipboard() const
{
    QClipboard *clipboard = QApplication::clipboard();

    QMimeData *data = new QMimeData;

    data->setText(toText());
    QStringList imports;
    foreach (const Import &import, model()->imports())
        imports.append(import.toString());

    data->setData("QmlDesigner::imports", stringListToArray(imports));
    clipboard->setMimeData(data);
}

void DesignDocumentControllerView::fromClipboard()
{
    QClipboard *clipboard = QApplication::clipboard();
    fromText(clipboard->text());
    QStringList imports = arrayToStringList(clipboard->mimeData()->data("QmlDesigner::imports"));
//    foreach (const QString &importString, imports) {
//        Import import(Import::createLibraryImport();
//        model()->addImport(import); //### imports
//    }
}


QString DesignDocumentControllerView::toText() const
{
    QScopedPointer<Model> outputModel(Model::create("Qt/Rectangle"));
    outputModel->setMetaInfo(model()->metaInfo());
    QPlainTextEdit textEdit;
    textEdit.setPlainText("import Qt 4.6; Item {}");
    PlainTextEditModifier modifier(&textEdit);

    QScopedPointer<RewriterView> rewriterView(new RewriterView(RewriterView::Amend, 0));
    rewriterView->setTextModifier(&modifier);
    outputModel->attachView(rewriterView.data());

    ModelMerger merger(rewriterView.data());

    merger.replaceModel(rootModelNode());

    ModelNode rewriterNode(rewriterView->rootModelNode());

    //get the text of the root item without imports
    return rewriterView->extractText(QList<ModelNode>() << rewriterNode).value(rewriterNode);
}

void DesignDocumentControllerView::fromText(QString text)
{
    QScopedPointer<Model> inputModel(Model::create("Qt/Rectangle"));
    inputModel->setMetaInfo(model()->metaInfo());
    inputModel->setFileUrl(model()->fileUrl());
    QPlainTextEdit textEdit;
    QString imports("import Qt 4.6;\n");
    textEdit.setPlainText(imports + text);
    PlainTextEditModifier modifier(&textEdit);

    QScopedPointer<RewriterView> rewriterView(new RewriterView(RewriterView::Amend, 0));
    rewriterView->setTextModifier(&modifier);
    inputModel->attachView(rewriterView.data());

    if (rewriterView->errors().isEmpty() && rewriterView->rootModelNode().isValid()) {
        ModelMerger merger(this);
        merger.replaceModel(rewriterView->rootModelNode());
    }
}

}// namespace QmlDesigner
