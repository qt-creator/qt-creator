/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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

#include <model.h>

#include <utils/optional.h>

#include <QHash>
#include <QQueue>

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

    Utils::optional<DataType> take(Model *model)
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
