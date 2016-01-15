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
#include <rewriterview.h>
#include <basetexteditmodifier.h>

#include <QApplication>
#include <QPlainTextEdit>
#include <QClipboard>
#include <QMimeData>
#include <QDebug>

namespace QmlDesigner {

DesignDocumentView::DesignDocumentView(QObject *parent)
    : AbstractView(parent), m_modelMerger(this)
{
}

DesignDocumentView::~DesignDocumentView()
{
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

    QMimeData *data = new QMimeData;

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

    QScopedPointer<RewriterView> rewriterView(new RewriterView(RewriterView::Amend, 0));
    rewriterView->setCheckSemanticErrors(false);
    rewriterView->setTextModifier(&modifier);
    outputModel->setRewriterView(rewriterView.data());

    ModelMerger merger(rewriterView.data());

    merger.replaceModel(rootModelNode());

    ModelNode rewriterNode(rewriterView->rootModelNode());

    //get the text of the root item without imports
    return rewriterView->extractText(QList<ModelNode>() << rewriterNode).value(rewriterNode);
}

void DesignDocumentView::fromText(QString text)
{
    QScopedPointer<Model> inputModel(Model::create("QtQuick.Rectangle", 1, 0, model()));
    inputModel->setFileUrl(model()->fileUrl());
    QPlainTextEdit textEdit;
    QString imports;
    foreach (const Import &import, model()->imports())
        imports += QStringLiteral("import ") + import.toString(true) + QLatin1Char(';') + QLatin1Char('\n');

    textEdit.setPlainText(imports + text);
    NotIndentingTextEditModifier modifier(&textEdit);

    QScopedPointer<RewriterView> rewriterView(new RewriterView(RewriterView::Amend, 0));
    rewriterView->setCheckSemanticErrors(false);
    rewriterView->setTextModifier(&modifier);
    inputModel->setRewriterView(rewriterView.data());

    if (rewriterView->errors().isEmpty() && rewriterView->rootModelNode().isValid()) {
        ModelMerger merger(this);
        merger.replaceModel(rewriterView->rootModelNode());
    }
}

}// namespace QmlDesigner
