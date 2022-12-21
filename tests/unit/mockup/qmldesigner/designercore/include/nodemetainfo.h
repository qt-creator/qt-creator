// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "propertymetainfo.h"

#include <projectstorage/projectstoragefwd.h>
#include <projectstorageids.h>

#include <QIcon>
#include <QList>
#include <QString>
#include <QVariant>

#include <qmldesignercorelib_global.h>

QT_BEGIN_NAMESPACE
class QDeclarativeContext;
QT_END_NAMESPACE

namespace QmlDesigner {

class MetaInfo;
class Model;
class AbstractProperty;

class NodeMetaInfo
{
public:
    NodeMetaInfo() {}
    NodeMetaInfo(Model *, const TypeName &, int, int) {}
    NodeMetaInfo(TypeId, NotNullPointer<const ProjectStorage<Sqlite::Database>>) {}
    NodeMetaInfo(NotNullPointer<const ProjectStorage<Sqlite::Database>>) {}

    bool isValid() const { return {}; }
    bool isFileComponent() const { return {}; }
    bool hasProperty(const PropertyName &) const { return {}; }
    PropertyMetaInfos properties() const { return {}; }
    PropertyMetaInfos localProperties() const { return {}; }
    PropertyMetaInfo property(const PropertyName &) const { return {}; }
    PropertyNameList propertyNames() const { return {}; }
    PropertyNameList signalNames() const { return {}; }
    PropertyNameList directPropertyNames() const { return {}; }
    PropertyName defaultPropertyName() const { return "data"; }
    bool hasDefaultProperty() const { return {}; }

    std::vector<NodeMetaInfo> classHierarchy() const { return {}; }
    std::vector<NodeMetaInfo> superClasses() const { return {}; }

    bool defaultPropertyIsComponent() const { return {}; }

    TypeName typeName() const { return {}; }
    TypeName simplifiedTypeName() const { return {}; }
    int majorVersion() const { return {}; }
    int minorVersion() const { return {}; }

    QString componentSource() const { return {}; }
    QString componentFileName() const { return {}; }

    bool hasCustomParser() const { return {}; }

    bool availableInVersion(int, int) const { return {}; }
    bool isBasedOn(const NodeMetaInfo &) const { return {}; }
    bool isBasedOn(const NodeMetaInfo &, const NodeMetaInfo &) const { return {}; }
    bool isBasedOn(const NodeMetaInfo &, const NodeMetaInfo &, const NodeMetaInfo &) const
    {
        return {};
    }

    bool isGraphicalItem() const { return {}; }
    bool isLayoutable() const { return {}; }
    bool isView() const { return {}; }
    bool isTabView() const { return {}; }
    bool isQtQuick3DNode() const { return {}; }
    bool isQtQuick3DModel() const { return {}; }
    bool isQtQuick3DMaterial() const { return {}; }
    bool isQtQuickLoader() const { return {}; }
    bool isQtQuickItem() const { return {}; }
    bool isQtQuick3DTexture() const { return {}; }

    QString importDirectoryPath() const { return {}; }

    static void clearCache() {}
};

using NodeMetaInfos = std::vector<NodeMetaInfo>;

} // namespace QmlDesigner
