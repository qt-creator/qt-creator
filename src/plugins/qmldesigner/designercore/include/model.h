/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef DESIGNERMODEL_H
#define DESIGNERMODEL_H

#include <qmldesignercorelib_global.h>
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

protected:
    Model();

public:
    Internal::ModelPrivate *d;
};

}

#endif // DESIGNERMODEL_H
