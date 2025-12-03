// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

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

class QMLDESIGNER_EXPORT QmlModelState final : public QmlModelNodeFacade
{
public:
    QmlModelState() = default;

    QmlModelState(const ModelNode &modelNode)
        : QmlModelNodeFacade(modelNode)
    {}

    QmlPropertyChanges ensurePropertyChangesForTarget(const ModelNode &node, SL sl = {});
    QmlPropertyChanges propertyChangesForTarget(const ModelNode &node, SL sl = {});
    QList<QmlModelStateOperation> stateOperations(const ModelNode &node, SL sl = {}) const;
    QList<QmlPropertyChanges> propertyChanges(SL sl = {}) const;
    QList<QmlModelStateOperation> stateOperations(SL sl = {}) const;
    QList<QmlModelStateOperation> allInvalidStateOperations(SL sl = {}) const;

    bool hasPropertyChanges(const ModelNode &node, SL sl = {}) const;

    bool hasStateOperation(const ModelNode &node, SL sl = {}) const;

    void removePropertyChanges(const ModelNode &node, SL sl = {});

    bool affectsModelNode(const ModelNode &node, SL sl = {}) const;
    QList<QmlObjectNode> allAffectedNodes(SL sl = {}) const;
    QString name(SL sl = {}) const;
    void setName(const QString &name, SL sl = {});
    bool isValid(SL sl = {}) const;
    explicit operator bool() const { return isValid(); }

    static bool isValidQmlModelState(const ModelNode &modelNode, SL sl = {});
    void destroy(SL sl = {});

    bool isBaseState(SL sl = {}) const;
    static bool isBaseState(const ModelNode &modelNode, SL sl = {});
    QmlModelState duplicate(const QString &name, SL sl = {}) const;
    QmlModelStateGroup stateGroup(SL sl = {}) const;

    static ModelNode createQmlState(AbstractView *view,
                                    const PropertyListType &propertyList,
                                    SL sl = {});

    static QmlModelState createBaseState(const AbstractView *view, SL sl = {});

    void setAsDefault(SL sl = {});
    bool isDefault(SL sl = {}) const;

    void setAnnotation(const Annotation &annotation, const QString &id, SL sl = {});
    Annotation annotation(SL sl = {}) const;
    QString annotationName(SL sl = {}) const;
    bool hasAnnotation(SL sl = {}) const;
    void removeAnnotation(SL sl = {});

    QString extend(SL sl = {}) const;
    void setExtend(const QString &name, SL sl = {});
    bool hasExtend(SL sl = {}) const;

private:
    void addChangeSetIfNotExists(const ModelNode &node);
};

} //QmlDesigner
