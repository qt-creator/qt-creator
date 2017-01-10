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

#include <qmldesignercorelib_global.h>

#include <documentmessage.h>

#include <QObject>
#include <QPair>

#include <import.h>

QT_BEGIN_NAMESPACE
class QUrl;
QT_END_NAMESPACE

namespace QmlDesigner {

namespace Internal { class ModelPrivate; }

class AnchorLine;
class ModelNode;
class NodeState;
class AbstractView;
class NodeStateChangeSet;
class MetaInfo;
class NodeMetaInfo;
class ModelState;
class NodeAnchors;
class AbstractProperty;
class RewriterView;
class NodeInstanceView;
class TextModifier;

typedef QList<QPair<PropertyName, QVariant> > PropertyListType;

class QMLDESIGNERCORE_EXPORT Model : public QObject
{
    friend class QmlDesigner::ModelNode;
    friend class QmlDesigner::NodeState;
    friend class QmlDesigner::ModelState;
    friend class QmlDesigner::NodeAnchors;
    friend class QmlDesigner::AbstractProperty;
    friend class QmlDesigner::AbstractView;
    friend class Internal::ModelPrivate;

    Q_OBJECT

public:
    enum ViewNotification { NotifyView, DoNotNotifyView };

    virtual ~Model();

    static Model *create(TypeName type, int major = 1, int minor = 1, Model *metaInfoPropxyModel = 0);

    QUrl fileUrl() const;
    void setFileUrl(const QUrl &url);

    const MetaInfo metaInfo() const;
    MetaInfo metaInfo();
    NodeMetaInfo metaInfo(const TypeName &typeName, int majorVersion = -1, int minorVersion = -1);
    bool hasNodeMetaInfo(const TypeName &typeName, int majorVersion = -1, int minorVersion = -1);

    void attachView(AbstractView *view);
    void detachView(AbstractView *view, ViewNotification emitDetachNotify = NotifyView);

    // Editing sub-components:

    // Imports:
    QList<Import> imports() const;
    QList<Import> possibleImports() const;
    QList<Import> usedImports() const;
    void changeImports(const QList<Import> &importsToBeAdded, const QList<Import> &importsToBeRemoved);
    void setPossibleImports(const QList<Import> &possibleImports);
    void setUsedImports(const QList<Import> &usedImports);
    bool hasImport(const Import &import, bool ignoreAlias = true, bool allowHigherVersion = false);
    QString pathForImport(const Import &import);
    QStringList importPaths() const;

    RewriterView *rewriterView() const;
    void setRewriterView(RewriterView *rewriterView);

    NodeInstanceView *nodeInstanceView() const;
    void setNodeInstanceView(NodeInstanceView *nodeInstanceView);

    Model *metaInfoProxyModel();

    TextModifier *textModifier() const;
    void setTextModifier(TextModifier *textModifier);
    void setDocumentMessages(const QList<DocumentMessage> &errors, const QList<DocumentMessage> &warnings);

protected:
    Model();

public:
    Internal::ModelPrivate *d;
};

}
