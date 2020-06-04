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

#include <QJsonValue>
#include <QPointer>
#include <QByteArrayList>

#include <vector>
#include <memory>

#include "utils/qtcassert.h"

QT_BEGIN_NAMESPACE
class QJsonArray;
QT_END_NAMESPACE

namespace QmlDesigner {
class Model;
class ModelNode;
class ComponentExporter;
class ModelNodeParser;

namespace  Internal {
class NodeParserCreatorBase
{
public:
    virtual ~NodeParserCreatorBase() {}
protected:
    virtual ModelNodeParser *instance(const QByteArrayList &, const ModelNode &) const = 0;
    friend class QmlDesigner::ComponentExporter;
};

template<class T>
class NodeParserCreator : public NodeParserCreatorBase
{
public:
    NodeParserCreator() = default;
    ~NodeParserCreator() = default;

protected:
    ModelNodeParser *instance(const QByteArrayList &lineage, const ModelNode &node) const {
        return new T(lineage, node);
    }
};
} //Internal

class ComponentExporter
{
public:
    ComponentExporter(const ModelNode &rootNode);

    QJsonObject exportComponent() const;

    template<typename T> static void addNodeParser()
    {
        QTC_ASSERT((std::is_base_of<ModelNodeParser, T>::value), return);
        m_readers.push_back(std::make_unique<Internal::NodeParserCreator<T>>());
    }
private:
    ModelNodeParser* createNodeParser(const ModelNode &node) const;
    QJsonObject nodeToJson(const ModelNode &node) const;

private:
    const ModelNode &m_rootNode;
    static std::vector<std::unique_ptr<Internal::NodeParserCreatorBase>> m_readers;
};
}
