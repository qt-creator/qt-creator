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
    NodeHints();
    NodeHints(const ModelNode &modelNode);

    bool canBeContainer() const;
    bool forceClip() const;
    bool doesLayoutChildren() const;
    bool canBeDroppedInFormEditor() const;
    bool canBeDroppedInNavigator() const;
    bool isMovable() const;
    bool isStackedContainer() const;
    QString indexPropertyForStackedContainer() const;

    QHash<QString, QString> hints() const;

private:
    ModelNode modelNode() const;
    bool isValid() const;
    Model *model() const;
    bool evaluateBooleanExpression(const QString &hintName, bool defaultValue) const;

    ModelNode m_modelNode;
    QHash<QString, QString> m_hints;
};

namespace Internal {

class JSObject : public QObject {

    Q_OBJECT

    Q_PROPERTY(bool hasParent READ hasParent NOTIFY modelNodeChanged)
    Q_PROPERTY(bool hasChildren READ hasChildren NOTIFY modelNodeChanged)
    Q_PROPERTY(bool currentParentIsRoot READ currentParentIsRoot NOTIFY modelNodeChanged)

public:
    JSObject();
    JSObject(QObject *parent = 0);
    void setModelNode(const ModelNode &node);
    bool hasParent() const;
    bool hasChildren() const;
    bool currentParentIsRoot() const;

    Q_INVOKABLE bool isSubclassOf(const QString &typeName);
    Q_INVOKABLE bool rootItemIsSubclassOf(const QString &typeName);
    Q_INVOKABLE bool currentParentIsSubclassOf(const QString &typeName);

signals:
    void modelNodeChanged();

private:
    ModelNode m_modelNode;
};

} //Internal

} //QmlDesigner
