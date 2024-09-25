// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <qmldesignercorelib_global.h>

#include <model.h>

#include <QHash>
#include <QQueue>

#include <optional>

namespace QmlDesigner {

template<class DataType>
class ModelCache
{
public:
    ModelCache(int max = 20)
        : m_maxEntries(max)
    {}

    void insert(Model *model, const DataType &data)
    {
        QObject::connect(model, &Model::destroyed, [this](QObject *o) {
            QObject *deletedModel = o;

            if (deletedModel) {
                m_content.remove(deletedModel);
                m_queue.removeAll(deletedModel);
            }
        });

        m_content.insert(model, data);
        if (!m_queue.contains(model))
            m_queue.append(model);
        if (m_queue.length() > m_maxEntries) {
            QObject *first = m_queue.takeFirst();
            m_content.remove(first);
        }
    }

    std::optional<DataType> take(Model *model)
    {
        if (!m_content.contains(model))
            return {};
        m_queue.removeOne(model);
        return m_content.take(model);
    }

private:
    QHash<QObject *, DataType> m_content;
    QQueue<QObject *> m_queue;
    int m_maxEntries = 20;
};

} // namespace QmlDesigner
