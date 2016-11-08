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

class DebugMessagesModel : public QmlProfilerTimelineModel
{
    Q_OBJECT

public:
    DebugMessagesModel(QmlProfilerModelManager *manager, QObject *parent = 0);

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

    struct MessageData {
        MessageData(const QString &text = QString(), int typeId = -1) :
            text(text), typeId(typeId) {}
        QString text;
        int typeId;
    };

    int m_maximumMsgType;
    QVector<MessageData> m_data;
};

} // namespace Internal
} // namespace Qmlprofiler
