/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#include <QByteArrayList>
#include <QCoreApplication>
#include <QJsonObject>

#include <memory>
#include <vector>

#include "utils/qtcassert.h"

QT_BEGIN_NAMESPACE
class QJsonArray;
QT_END_NAMESPACE

namespace QmlDesigner {
class AssetExporter;
class ModelNode;
class Component;
class NodeDumper;

namespace  Internal {
class NodeDumperCreatorBase
{
public:
    virtual ~NodeDumperCreatorBase() {}
protected:
    virtual NodeDumper *instance(const QByteArrayList &, const ModelNode &) const = 0;
    friend Component;
};

template<class T>
class NodeDumperCreator : public NodeDumperCreatorBase
{
public:
    NodeDumperCreator() = default;
    ~NodeDumperCreator() = default;

protected:
    NodeDumper *instance(const QByteArrayList &lineage, const ModelNode &node) const {
        return new T(lineage, node);
    }
};
} //Internal

class Component
{
    Q_DECLARE_TR_FUNCTIONS(Component);

public:
    Component(AssetExporter& exporter, const ModelNode &rootNode);

    void exportComponent();
    const QString &name() const;
    QJsonObject json() const;

    AssetExporter &exporter();

    template<typename T> static void addNodeDumper()
    {
        QTC_ASSERT((std::is_base_of<NodeDumper, T>::value), return);
        m_readers.push_back(std::make_unique<Internal::NodeDumperCreator<T>>());
    }
private:
    NodeDumper* createNodeDumper(const ModelNode &node) const;
    QJsonObject nodeToJson(const ModelNode &node);
    void addReferenceAsset(QJsonObject &metadataObject) const;
    void stichChildrendAssets(const ModelNode &node, QPixmap &parentPixmap) const;
    void addImports();

private:
    AssetExporter& m_exporter;
    const ModelNode &m_rootNode;
    QString m_name;
    QJsonObject m_json;
    static std::vector<std::unique_ptr<Internal::NodeDumperCreatorBase>> m_readers;
};
}
