/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd
** All rights reserved.
** For any questions to The Qt Company, please use contact form at http://www.qt.io/contact-us
**
** This file is part of the Qt Enterprise Qt Quick Profiler Add-on.
**
** Licensees holding valid Qt Enterprise licenses may use this file in
** accordance with the Qt Enterprise License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.
**
** If you have questions regarding the use of this file, please use
** contact form at http://www.qt.io/contact-us
**
****************************************************************************/

#ifndef DEBUGMESSAGESMODEL_H
#define DEBUGMESSAGESMODEL_H

#include "qmlprofiler/qmlprofilertimelinemodel.h"

namespace QmlProfilerExtension {
namespace Internal {

class DebugMessagesModel : public QmlProfiler::QmlProfilerTimelineModel
{
    Q_OBJECT

protected:
    bool accepted(const QmlProfiler::QmlProfilerDataModel::QmlEventTypeData &event) const;

public:
    DebugMessagesModel(QmlProfiler::QmlProfilerModelManager *manager, QObject *parent = 0);

    int typeId(int index) const override;
    QColor color(int index) const override;
    QVariantList labels() const override;
    QVariantMap details(int index) const override;
    int expandedRow(int index) const override;
    int collapsedRow(int index) const override;
    void loadData() override;
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

}
}

#endif // DEBUGMESSAGESMODEL_H
