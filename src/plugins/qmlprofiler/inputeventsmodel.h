// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmlprofilertimelinemodel.h"

namespace QmlProfiler {
namespace Internal {

class InputEventsModel : public QmlProfilerTimelineModel
{
    Q_OBJECT

public:
    struct Item {
        Item(InputEventType type = UndefinedInputEventType, int a = 0, int b = 0);
        InputEventType type;
        int a;
        int b;
    };

    InputEventsModel(QmlProfilerModelManager *manager, Timeline::TimelineModelAggregator *parent);

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

    QVector<Item> m_data;
};

} // namespace Internal
} // namespace QmlProfiler
