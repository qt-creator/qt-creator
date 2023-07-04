// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <qmldesignercorelib_global.h>
#include "qmlmodelnodefacade.h"
#include "qmlchangeset.h"

namespace QmlDesigner {

class AbstractViewAbstractVieweGroup;
class QmlObjectNode;
class QmlModelStateGroup;
class Annotation;
class AnnotationEditor;
class StatesEditorView;

namespace Experimental {
class StatesEditorView;
}

class QMLDESIGNERCORE_EXPORT QmlModelState final : public QmlModelNodeFacade
{
    friend StatesEditorView;

public:
    QmlModelState();
    QmlModelState(const ModelNode &modelNode);

    QmlPropertyChanges propertyChanges(const ModelNode &node);
    QList<QmlModelStateOperation> stateOperations(const ModelNode &node) const;
    QList<QmlPropertyChanges> propertyChanges() const;
    QList<QmlModelStateOperation> stateOperations() const;
    QList<QmlModelStateOperation> allInvalidStateOperations() const;

    bool hasPropertyChanges(const ModelNode &node) const;

    bool hasStateOperation(const ModelNode &node) const;

    void removePropertyChanges(const ModelNode &node);

    bool affectsModelNode(const ModelNode &node) const;
    QList<QmlObjectNode> allAffectedNodes() const;
    QString name() const;
    void setName(const QString &name);
    bool isValid() const;
    explicit operator bool() const { return isValid(); }
    static bool isValidQmlModelState(const ModelNode &modelNode);
    void destroy();

    bool isBaseState() const;
    static bool isBaseState(const ModelNode &modelNode);
    QmlModelState duplicate(const QString &name) const;
    QmlModelStateGroup stateGroup() const;

    static ModelNode createQmlState(AbstractView *view, const PropertyListType &propertyList);

    void setAsDefault();
    bool isDefault() const;

    void setAnnotation(const Annotation &annotation, const QString &id);
    Annotation annotation() const;
    QString annotationName() const;
    bool hasAnnotation() const;
    void removeAnnotation();

    QString extend() const;
    void setExtend(const QString &name);
    bool hasExtend() const;

protected:
    void addChangeSetIfNotExists(const ModelNode &node);
    static QmlModelState createBaseState(const AbstractView *view);
};

} //QmlDesigner
