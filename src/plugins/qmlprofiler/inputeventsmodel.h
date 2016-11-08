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

#include "qmlprofilertimelinemodel.h"

namespace QmlProfiler {
namespace Internal {

class InputEventsModel : public QmlProfilerTimelineModel
{
    Q_OBJECT

public:
    struct InputEvent {
        InputEvent(InputEventType type = MaximumInputEventType, int a = 0, int b = 0);
        InputEventType type;
        int a;
        int b;
    };

    InputEventsModel(QmlProfilerModelManager *manager, QObject *parent = 0);

    bool accepted(const QmlEventType &type) const override;
    void loadEvent(const QmlEvent &event, const QmlEventType &type) override;
    void finalize() override;
    void clear() override;

    int typeId(int index) const override;
    QRgb color(int index) const override;
    QVariantList labels() const override;
    QVariantMap details(int index) const override;
    int expandedRow(int index) const override;
    int collapsedRow(int index) const override;

private:
    static QMetaEnum metaEnum(const char *name);
    int m_keyTypeId;
    int m_mouseTypeId;

    QVector<InputEvent> m_data;
};

} // namespace Internal
} // namespace QmlProfiler
