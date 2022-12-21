// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QList>
#include <QString>
#include "modelnode.h"

#include "qmldesignercorelib_global.h"
#include "invalidmetainfoexception.h"

QT_BEGIN_NAMESPACE
class QDeclarativeContext;
QT_END_NAMESPACE

namespace QmlDesigner {

class MetaInfo;
class Model;
class AbstractProperty;
class ItemLibraryEntry;

namespace Internal {
    class MetaInfoPrivate;
    class MetaInfoReader;
    class SubComponentManagerPrivate;
    class ItemLibraryEntryData;
    class NodeMetaInfoPrivate;
}

class QMLDESIGNERCORE_EXPORT NodeHints
{
public:
    bool canBeContainerFor(const ModelNode &potenialChild) const;
    bool forceClip() const;
    bool doesLayoutChildren() const;
    bool canBeDroppedInFormEditor() const;
    bool canBeDroppedInNavigator() const;
    bool canBeDroppedInView3D() const;
    bool isMovable() const;
    bool isResizable() const;
    bool hasFormEditorItem() const;
    bool isStackedContainer() const;
    bool canBeReparentedTo(const ModelNode &potenialParent);
    QString indexPropertyForStackedContainer() const;
    QStringList visibleNonDefaultProperties() const;
    bool takesOverRenderingOfChildren() const;
    bool visibleInNavigator() const;
    bool visibleInLibrary() const;
    QString forceNonDefaultProperty() const;
    QPair<QString, QVariant> setParentProperty() const;
    QString bindParentToProperty() const;

    QHash<QString, QString> hints() const;
    static NodeHints fromModelNode(const ModelNode &modelNode);
    static NodeHints fromItemLibraryEntry(const ItemLibraryEntry &entry);

private:
    explicit NodeHints(const ModelNode &modelNode);
    explicit NodeHints(const ItemLibraryEntry &entry);
    ModelNode modelNode() const;
    bool isValid() const;
    Model *model() const;
    bool evaluateBooleanExpression(const QString &hintName, bool defaultValue, const ModelNode potentialParent = ModelNode()) const;

    ModelNode m_modelNode;
    QHash<QString, QString> m_hints;
};

namespace Internal {

class JSObject : public QObject {

    Q_OBJECT

    Q_PROPERTY(bool hasParent READ hasParent NOTIFY modelNodeChanged)
    Q_PROPERTY(bool hasChildren READ hasChildren NOTIFY modelNodeChanged)
    Q_PROPERTY(bool currentParentIsRoot READ currentParentIsRoot NOTIFY modelNodeChanged)
    Q_PROPERTY(bool potentialParentIsRoot READ potentialParentIsRoot NOTIFY otherNodeChanged)
    Q_PROPERTY(bool potentialChildIsRoot READ potentialChildIsRoot NOTIFY otherNodeChanged)

public:
    JSObject() = default;
    JSObject(QObject *parent);
    void setModelNode(const ModelNode &node);
    void setOtherNode(const ModelNode &node);
    bool hasParent() const;
    bool hasChildren() const;
    bool currentParentIsRoot() const;
    bool potentialParentIsRoot() const;
    bool potentialChildIsRoot() const;

    Q_INVOKABLE bool isSubclassOf(const QString &typeName);
    Q_INVOKABLE bool rootItemIsSubclassOf(const QString &typeName);
    Q_INVOKABLE bool currentParentIsSubclassOf(const QString &typeName);
    Q_INVOKABLE bool potentialParentIsSubclassOf(const QString &typeName);
    Q_INVOKABLE bool potentialChildIsSubclassOf(const QString &typeName);

signals:
    void modelNodeChanged();
    void otherNodeChanged();

private:
    ModelNode m_modelNode;
    ModelNode m_otherNode;
};

} //Internal

} //QmlDesigner
