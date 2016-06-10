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

static ModelNode addNodeListChild(const ModelNode &parentNode, const QString &typeName, int major, int minor, const QString &parentProperty)
{
    ModelNode newNode = parentNode.view()->createModelNode(typeName.toUtf8(), major, minor);
    parentNode.nodeListProperty(parentProperty.toUtf8()).reparentHere(newNode);
    return newNode;
}

static ModelNode addNodeChild(const ModelNode &parentNode, const QString &typeName, int major, int minor, const QString &parentProperty)
{
    ModelNode newNode = parentNode.view()->createModelNode(typeName.toUtf8(), major, minor);
    parentNode.nodeProperty(parentProperty.toUtf8()).reparentHere(newNode);
    return newNode;
}

static QString bareTemplate("import Qt 4.6\n"
                            "Item { id: parentItem;"
                            "  %1"
                            "}");
static QString contentsTemplate(bareTemplate.arg("Text { id: textChild; x:10; y: 10; text: \"%1\"; %2 }"));

// TODO: this need to e updated for states
static bool compareProperty(const AbstractProperty &property1, const AbstractProperty &property2)
{
    return (property1.name() == property2.name());
//            && (property1.value().type() == property2.value().type());
//            && (property1.value() == property2.value()));
}

// TODO: this need to e updated for states
static bool compareTree(const ModelNode &node1, const ModelNode &node2)
{
    if (!node1.isValid() || !node2.isValid()) {
        return false;
    }

    if (node1.type() != node2.type()) {
        return false;
    }

    // Compare properties
    {
        const QList<AbstractProperty> propList1 = node1.properties();
        const QList<AbstractProperty> propList2 = node2.properties();

        QList<AbstractProperty>::const_iterator iter1 = propList1.constBegin();
        QList<AbstractProperty>::const_iterator iter2 = propList2.constBegin();
        for (;
             iter1 != propList1.constEnd() && iter2 != propList2.constEnd();
             iter1++, iter2++) {
            if (!compareProperty(*iter1, *iter2))
                return false;
        }

        if (iter1 != propList1.constEnd() || iter2 != propList2.constEnd())
            return false;
    }

    // Compare list of children
    {
        const QList<ModelNode> childList1 = node1.directSubModelNodes();
        const QList<ModelNode> childList2 = node2.directSubModelNodes();

        QList<ModelNode>::const_iterator iter1;
        QList<ModelNode>::const_iterator iter2;
        for (iter1 = childList1.constBegin(), iter2 = childList2.constBegin();
             iter1 != childList1.constEnd() && iter2 != childList2.constEnd();
             iter1++, iter2++) {
            if (!compareTree((*iter1), (*iter2)))
                return false;
        }

        if (iter1 != childList1.constEnd() || iter2 != childList2.constEnd())
            return false;
    }
    return true;
}

//void load(const QString &data, Model *&model, ByteArrayModifier *&modifier)
//{
//    model = 0;
//    QByteArray bytes = data.toLatin1();
//    QBuffer file(&bytes, 0);
//    QVERIFY(file.open(QIODevice::ReadOnly));
//    QList<QDeclarativeError> errors;
//    QString fileText(file.readAll());
//    modifier = ByteArrayModifier::create(QString(fileText));
//    model = Model::create(modifier, QUrl(), &errors);
//
//    if (!errors.isEmpty()) {
//        printErrors(errors, "<inline>");
//    }
//
//    file.close();
//}

//void reload(const QString &data, ByteArrayModifier *modifier)
//{
//    modifier->setText(data);
//}

//static Model* create(const QString& document)
//{
//    ByteArrayModifier* modifier = 0;
//    Model *model = 0;
//
//    load(document, model, modifier);
//
//    if (modifier && model)
//        modifier->setParent(model);
//
//    return model;
//}
