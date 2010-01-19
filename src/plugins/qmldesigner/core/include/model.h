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

#ifndef DESIGNERMODEL_H
#define DESIGNERMODEL_H

#include <corelib_global.h>
#include <QtCore/QObject>
#include <QtCore/QMimeData>
#include <QtCore/QPair>
#include <QtDeclarative/QmlError>

#include <import.h>

class QUrl;

namespace QmlDesigner {

namespace Internal {
    class ModelPrivate;
}

class AnchorLine;
class ModelNode;
class NodeState;
class AbstractView;
class WidgetQueryView;
class NodeStateChangeSet;
class MetaInfo;
class ModelState;
class NodeAnchors;
class AbstractProperty;

typedef QList<QPair<QString, QVariant> > PropertyListType;

class CORESHARED_EXPORT Model : public QObject
{
    friend class QmlDesigner::ModelNode;
    friend class QmlDesigner::NodeState;
    friend class QmlDesigner::ModelState;
    friend class QmlDesigner::NodeAnchors;
    friend class QmlDesigner::AbstractProperty;
    friend class QmlDesigner::AbstractView;
    friend class Internal::ModelPrivate;

    Q_DISABLE_COPY(Model)
    Q_OBJECT
public:
    enum ViewNotification { NotifyView, DoNotNotifyView };

    virtual ~Model();

    static Model *create(QString type, int major = 4, int minor = 6);

    Model *masterModel() const;
    void setMasterModel(Model *model);

    QUrl fileUrl() const;
    void setFileUrl(const QUrl &url);

    const MetaInfo metaInfo() const;
    MetaInfo metaInfo();
    void setMetaInfo(const MetaInfo &metaInfo);

    void attachView(AbstractView *view);
    void detachView(AbstractView *view, ViewNotification emitDetachNotify = NotifyView);

    // Editing sub-components:

    // Imports:
    QSet<Import> imports() const;
    void addImport(const Import &import);
    void removeImport(const Import &import);

protected:
    Model();

public:
    Internal::ModelPrivate *m_d;
};

}

#endif // DESIGNERMODEL_H
