// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmlrefactoring.h"
#include "../rewriter/rewritertracing.h"

#include <QDebug>

#include "addarraymembervisitor.h"
#include "addobjectvisitor.h"
#include "addpropertyvisitor.h"
#include "changeimportsvisitor.h"
#include "changeobjecttypevisitor.h"
#include "changepropertyvisitor.h"
#include "moveobjectvisitor.h"
#include "moveobjectbeforeobjectvisitor.h"
#include "removepropertyvisitor.h"
#include "removeuiobjectmembervisitor.h"

namespace QmlDesigner {

using NanotraceHR::keyValue;
using RewriterTracing::category;

QmlRefactoring::QmlRefactoring(const QmlJS::Document::Ptr &doc,
                               TextModifier &modifier,
                               Utils::span<const PropertyNameView> propertyOrder)
    : qmlDocument(doc)
    , textModifier(&modifier)
    , m_propertyOrder(propertyOrder)
{
    NanotraceHR::Tracer tracer{"qml refactoring constructor", category()};
}

bool QmlRefactoring::reparseDocument()
{
    NanotraceHR::Tracer tracer{"qml refactoring reparse document", category()};

    const QString newSource = textModifier->text();

    //    qDebug() << "QmlRefactoring::reparseDocument() new QML source:" << newSource;

    QmlJS::Document::MutablePtr tmpDocument(
        QmlJS::Document::create("<ModelToTextMerger>", QmlJS::Dialect::Qml));
    tmpDocument->setSource(newSource);

    if (tmpDocument->parseQml()) {
        qmlDocument = tmpDocument;
        return true;
    } else {
        qWarning() << "*** Possible problem: QML file wasn't parsed correctly.";
        qDebug() << "*** QML text:" << textModifier->text();
        QString errorMessage = QStringLiteral("Parsing Error");
        if (!tmpDocument->diagnosticMessages().isEmpty())
            errorMessage = tmpDocument->diagnosticMessages().constFirst().message;

        qDebug() <<  "*** " << errorMessage;
        return false;
    }
}

bool QmlRefactoring::addImport(const Import &import)
{
    NanotraceHR::Tracer tracer{"qml refactoring add import",
                               category(),
                               keyValue("import", import.toString())};

    Internal::ChangeImportsVisitor visitor(*textModifier, qmlDocument->source());
    return visitor.add(qmlDocument->qmlProgram(), import);
}

bool QmlRefactoring::removeImport(const Import &import)
{
    NanotraceHR::Tracer tracer{"qml refactoring remove import", category(), keyValue("import", import)};

    Internal::ChangeImportsVisitor visitor(*textModifier, qmlDocument->source());
    return visitor.remove(qmlDocument->qmlProgram(), import);
}

bool QmlRefactoring::addToArrayMemberList(int parentLocation,
                                          PropertyNameView propertyName,
                                          const QString &content)
{
    NanotraceHR::Tracer tracer{"qml refactoring add to array member list",
                               category(),
                               keyValue("parent location", parentLocation),
                               keyValue("property name", propertyName),
                               keyValue("content", content)};

    if (parentLocation < 0)
        return false;

    Internal::AddArrayMemberVisitor visit(*textModifier,
                                          parentLocation,
                                          QString::fromUtf8(propertyName),
                                          content);
    visit.setConvertObjectBindingIntoArrayBinding(true);
    return visit(qmlDocument->qmlProgram());
}

bool QmlRefactoring::addToObjectMemberList(int parentLocation,
                                           std::optional<int> nodeLocation,
                                           const QString &content)
{
    NanotraceHR::Tracer tracer{"qml refactoring add to object member list",
                               category(),
                               keyValue("parent location", parentLocation),
                               keyValue("node location", nodeLocation),
                               keyValue("content", content)};
    if (parentLocation < 0)
        return false;

    Internal::AddObjectVisitor visit(*textModifier, parentLocation, nodeLocation, content, m_propertyOrder);
    return visit(qmlDocument->qmlProgram());
}

bool QmlRefactoring::addProperty(int parentLocation,
                                 PropertyNameView name,
                                 const QString &value,
                                 PropertyType propertyType,
                                 const TypeName &dynamicTypeName)
{
    NanotraceHR::Tracer tracer{"qml refactoring add property",
                               category(),
                               keyValue("parent location", parentLocation),
                               keyValue("name", name),
                               keyValue("value", value),
                               keyValue("property type", int(propertyType)),
                               keyValue("dynamic type name", dynamicTypeName)};

    if (parentLocation < 0)
        return true; /* Node is not in hierarchy, yet and operation can be ignored. */

    Internal::AddPropertyVisitor visit(
        *textModifier, parentLocation, name, value, propertyType, m_propertyOrder, dynamicTypeName);
    return visit(qmlDocument->qmlProgram());
}

bool QmlRefactoring::changeProperty(int parentLocation,
                                    PropertyNameView name,
                                    const QString &value,
                                    PropertyType propertyType)
{
    NanotraceHR::Tracer tracer{"qml refactoring change property",
                               category(),
                               keyValue("parent location", parentLocation),
                               keyValue("name", name),
                               keyValue("value", value),
                               keyValue("property type", int(propertyType))};

    if (parentLocation < 0)
        return false;

    Internal::ChangePropertyVisitor visit(*textModifier,
                                          parentLocation,
                                          QString::fromUtf8(name),
                                          value,
                                          propertyType);
    return visit(qmlDocument->qmlProgram());
}

bool QmlRefactoring::changeObjectType(int nodeLocation, const QString &newType)
{
    NanotraceHR::Tracer tracer{"qml refactoring change object type",
                               category(),
                               keyValue("node location", nodeLocation),
                               keyValue("new type", newType)};

    if (nodeLocation < 0 || newType.isEmpty())
        return false;

    Internal::ChangeObjectTypeVisitor visit(*textModifier, nodeLocation, newType);
    return visit(qmlDocument->qmlProgram());
}

bool QmlRefactoring::moveObject(int objectLocation,
                                PropertyNameView targetPropertyName,
                                bool targetIsArrayBinding,
                                int targetParentObjectLocation)
{
    NanotraceHR::Tracer tracer{"qml refactoring move object",
                               category(),
                               keyValue("object location", objectLocation),
                               keyValue("target property name", targetPropertyName),
                               keyValue("target is array binding", targetIsArrayBinding),
                               keyValue("target parent object location", targetParentObjectLocation)};

    if (objectLocation < 0 || targetParentObjectLocation < 0)
        return false;

    Internal::MoveObjectVisitor visit(*textModifier,
                                      objectLocation,
                                      targetPropertyName,
                                      targetIsArrayBinding,
                                      (quint32) targetParentObjectLocation,
                                      m_propertyOrder);
    return visit(qmlDocument->qmlProgram());
}

bool QmlRefactoring::moveObjectBeforeObject(int movingObjectLocation, int beforeObjectLocation, bool inDefaultProperty)
{
    NanotraceHR::Tracer tracer{"qml refactoring move object before object",
                               category(),
                               keyValue("moving object location", movingObjectLocation),
                               keyValue("before object location", beforeObjectLocation),
                               keyValue("in default property", inDefaultProperty)};

    if (movingObjectLocation < 0 || beforeObjectLocation < -1)
        return false;

    if (beforeObjectLocation == -1) {
        Internal::MoveObjectBeforeObjectVisitor visit(*textModifier,
                                                      movingObjectLocation,
                                                      inDefaultProperty);
        return visit(qmlDocument->qmlProgram());
    } else {
        Internal::MoveObjectBeforeObjectVisitor visit(*textModifier,
                                                      movingObjectLocation,
                                                      beforeObjectLocation,
                                                      inDefaultProperty);
        return visit(qmlDocument->qmlProgram());
    }
    return false;
}

bool QmlRefactoring::removeObject(int nodeLocation)
{
    NanotraceHR::Tracer tracer{"qml refactoring remove object",
                               category(),
                               keyValue("node location", nodeLocation)};

    if (nodeLocation < 0)
        return false;

    Internal::RemoveUIObjectMemberVisitor visit(*textModifier, nodeLocation);
    return visit(qmlDocument->qmlProgram());
}

bool QmlRefactoring::removeProperty(int parentLocation, PropertyNameView name)
{
    NanotraceHR::Tracer tracer{"qml refactoring remove property",
                               category(),
                               keyValue("parent location", parentLocation),
                               keyValue("name", name)};

    if (parentLocation < 0 || name.isEmpty())
        return false;

    Internal::RemovePropertyVisitor visit(*textModifier, parentLocation, QString::fromUtf8(name));
    return visit(qmlDocument->qmlProgram());
}
} // namespace QmlDesigner
