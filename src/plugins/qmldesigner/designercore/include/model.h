// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <qmldesignercorelib_global.h>

#include <documentmessage.h>
#include <projectstorage/projectstoragefwd.h>

#include <QMimeData>
#include <QObject>
#include <QPair>

#include <import.h>

QT_BEGIN_NAMESPACE
class QPixmap;
class QUrl;
QT_END_NAMESPACE

namespace QmlDesigner {

namespace Internal {
class ModelPrivate;
class WriteLocker;
} //Internal

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

using PropertyListType = QList<QPair<PropertyName, QVariant> >;

class QMLDESIGNERCORE_EXPORT Model : public QObject
{
    friend ModelNode;
    friend AbstractProperty;
    friend AbstractView;
    friend Internal::ModelPrivate;
    friend Internal::WriteLocker;
    friend class NodeMetaInfoPrivate;

    Q_OBJECT

public:
    enum ViewNotification { NotifyView, DoNotNotifyView };

    Model(ProjectStorage<Sqlite::Database> &projectStorage,
          const TypeName &type,
          int major = 1,
          int minor = 1,
          Model *metaInfoProxyModel = nullptr);
    Model(const TypeName &typeName, int major = 1, int minor = 1, Model *metaInfoProxyModel = nullptr);

    ~Model();

    static std::unique_ptr<Model> create(const TypeName &typeName,
                                         int major = 1,
                                         int minor = 1,
                                         Model *metaInfoProxyModel = nullptr)
    {
        return std::make_unique<Model>(typeName, major, minor, metaInfoProxyModel);
    }

    QUrl fileUrl() const;
    QUrl projectUrl() const;
    void setFileUrl(const QUrl &url);

    const MetaInfo metaInfo() const;
    MetaInfo metaInfo();
    NodeMetaInfo metaInfo(const TypeName &typeName, int majorVersion = -1, int minorVersion = -1) const;
    bool hasNodeMetaInfo(const TypeName &typeName, int majorVersion = -1, int minorVersion = -1) const;
    void setMetaInfo(const MetaInfo &metaInfo);

    void attachView(AbstractView *view);
    void detachView(AbstractView *view, ViewNotification emitDetachNotify = NotifyView);

    // Editing sub-components:

    // Imports:
    const QList<Import> &imports() const;
    const QList<Import> &possibleImports() const;
    const QList<Import> &usedImports() const;
    void changeImports(const QList<Import> &importsToBeAdded, const QList<Import> &importsToBeRemoved);
    void setPossibleImports(const QList<Import> &possibleImports);
    void setUsedImports(const QList<Import> &usedImports);
    bool hasImport(const Import &import, bool ignoreAlias = true, bool allowHigherVersion = false) const;
    bool isImportPossible(const Import &import, bool ignoreAlias = true, bool allowHigherVersion = false) const;
    QString pathForImport(const Import &import);
    QStringList importPaths() const;
    Import highestPossibleImport(const QString &importPath);

    RewriterView *rewriterView() const;
    void setRewriterView(RewriterView *rewriterView);

    NodeInstanceView *nodeInstanceView() const;
    void setNodeInstanceView(NodeInstanceView *nodeInstanceView);

    Model *metaInfoProxyModel() const;

    TextModifier *textModifier() const;
    void setTextModifier(TextModifier *textModifier);
    void setDocumentMessages(const QList<DocumentMessage> &errors,
                             const QList<DocumentMessage> &warnings);

    QList<ModelNode> selectedNodes(AbstractView *view) const;

    void clearMetaInfoCache();

    bool hasId(const QString &id) const;
    bool hasImport(const QString &importUrl) const;

    QString generateNewId(const QString &prefixName) const;
    QString generateNewId(const QString &prefixName, const QString &fallbackPrefix) const;

    void startDrag(QMimeData *mimeData, const QPixmap &icon);
    void endDrag();

    NotNullPointer<const ProjectStorage<Sqlite::Database>> projectStorage() const;

private:
    std::unique_ptr<Internal::ModelPrivate> d;
};

}
