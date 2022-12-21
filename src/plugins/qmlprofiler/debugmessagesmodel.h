// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmlprofilertimelinemodel.h"

namespace QmlProfiler {
namespace Internal {

class DebugMessagesModel : public QmlProfilerTimelineModel
{
    Q_OBJECT

public:
    struct Item {
        Item(const QString &text = QString(), int typeId = -1) :
            text(text), typeId(typeId) {}
        QString text;
        int typeId;
    };

    DebugMessagesModel(QmlProfilerModelManager *manager, Timeline::TimelineModelAggregator *parent);

    int typeId(int index) const override;
    QRgb color(int index) const override;
    QVariantList labels() const override;
    QVariantMap details(int index) const override;
    int expandedRow(int index) const override;
    int collapsedRow(int index) const override;
    void loadEvent(const QmlEvent &event, const QmlEventType &type) override;
    void finalize() override;
    void clear() override;
    QVariantMap location(int index) const override;

private:
    static QString messageType(uint i);

    int m_maximumMsgType;
    QVector<Item> m_data;
};

} // namespace Internal
} // namespace Qmlprofiler
