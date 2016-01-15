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

#ifndef QMLPROFILERDATAMODEL_H
#define QMLPROFILERDATAMODEL_H

#include "qmlprofilermodelmanager.h"

#include <qmldebug/qmlprofilereventtypes.h>
#include <utils/fileinprojectfinder.h>

namespace QmlProfiler {

class QMLPROFILER_EXPORT QmlProfilerDataModel : public QObject
{
    Q_OBJECT
public:
    struct QmlEventTypeData {
        QmlEventTypeData(const QString &displayName = QString(),
                         const QmlDebug::QmlEventLocation &location = QmlDebug::QmlEventLocation(),
                         QmlDebug::Message message = QmlDebug::MaximumMessage,
                         QmlDebug::RangeType rangeType = QmlDebug::MaximumRangeType,
                         int detailType = -1, const QString &data = QString()) :
            displayName(displayName), location(location), message(message), rangeType(rangeType),
            detailType(detailType), data(data)
        {}

        QString displayName;
        QmlDebug::QmlEventLocation location;
        QmlDebug::Message message;
        QmlDebug::RangeType rangeType;
        int detailType; // can be EventType, BindingType, PixmapEventType or SceneGraphFrameType
        QString data;
    };

    struct QmlEventData {
        QmlEventData(qint64 startTime = -1, qint64 duration = -1, int typeIndex = -1,
                     qint64 num0 = 0, qint64 num1 = 0, qint64 num2 = 0, qint64 num3 = 0,
                     qint64 num4 = 0) :
            m_startTime(startTime), m_duration(duration), m_dataType(NumericData),
            m_typeIndex(typeIndex)
        {
            m_numericData[0] = num0;
            m_numericData[1] = num1;
            m_numericData[2] = num2;
            m_numericData[3] = num3;
            m_numericData[4] = num4;
        }

        QmlEventData(qint64 startTime, qint64 duration, int typeIndex, const QString &data)
            : m_startTime(startTime), m_duration(duration), m_typeIndex(typeIndex)
        {
            assignStringData(data);
        }

        QmlEventData(const QmlEventData &other) :
            m_startTime(other.m_startTime), m_duration(other.m_duration),
            m_dataType(other.m_dataType), m_typeIndex(other.m_typeIndex)
        {
            assignData(other);
        }

        QmlEventData &operator=(const QmlEventData &other)
        {
            if (this != &other) {
                if (m_dataType == StringData)
                    delete m_stringData;

                m_startTime = other.m_startTime;
                m_duration = other.m_duration;
                m_typeIndex = other.m_typeIndex;
                m_dataType = other.m_dataType;
                assignData(other);
            }
            return *this;
        }

        ~QmlEventData()
        {
            if (m_dataType == StringData)
                delete m_stringData;
        }

        qint64 startTime() const { return m_startTime; }
        void setStartTime(qint64 startTime) { m_startTime = startTime; }

        qint64 duration() const { return m_duration; }
        void setDuration(qint64 duration) { m_duration = duration; }

        int typeIndex() const { return m_typeIndex; }
        void setTypeIndex(int typeIndex) { m_typeIndex = typeIndex; }

        qint64 numericData(int i) const { return m_dataType == NumericData ? m_numericData[i] : 0; }
        void setNumericData(int i, qint64 data)
        {
            if (m_dataType == StringData)
                delete m_stringData;

            m_dataType = NumericData;
            m_numericData[i] = data;
        }

        QString stringData() const
        {
            switch (m_dataType) {
            case NumericData: return QString();
            case StringData: return *m_stringData;
            default: return QString::fromUtf8(m_characterData, m_characterDataLength);
            }
        }

        void setStringData(const QString &data)
        {
            if (m_dataType == StringData)
                delete m_stringData;

            assignStringData(data);
        }

    private:

        static const quint8 StringData = 254;
        static const quint8 NumericData = 255;

        qint64 m_startTime;
        qint64 m_duration;
        union {
            qint64 m_numericData[5];
            QString *m_stringData;
            char m_characterData[5 * sizeof(qint64) + 3];
        };

        union {
            quint8 m_dataType;
            quint8 m_characterDataLength;
        };
        qint32 m_typeIndex;

        void assignData(const QmlEventData &other)
        {
            switch (m_dataType) {
            case StringData:
                m_stringData = new QString(*other.m_stringData);
                break;
            case NumericData:
                for (int i = 0; i < 5; ++i)
                    m_numericData[i] = other.m_numericData[i];
                break;
            default:
                memcpy(m_characterData, other.m_characterData, m_characterDataLength);
                break;
            }
        }

        void assignStringData(const QString &data)
        {
            QByteArray cdata = data.toUtf8();
            if (cdata.length() <= (int)sizeof(m_characterData)) {
                m_characterDataLength = cdata.length();
                memcpy(m_characterData, cdata.constData(), m_characterDataLength);
            } else {
                m_dataType = StringData;
                m_stringData = new QString(data);
            }
        }
    };

    struct QmlEventNoteData {
        QmlEventNoteData(int typeIndex = -1, qint64 startTime = -1, qint64 duration = -1,
                         const QString &text = QString()) :
            typeIndex(typeIndex), startTime(startTime), duration(duration), text(text)
        {}

        int typeIndex;
        qint64 startTime;
        qint64 duration;
        QString text;
    };

    static QString formatTime(qint64 timestamp);

    explicit QmlProfilerDataModel(Utils::FileInProjectFinder *fileFinder,
                                  QmlProfilerModelManager *parent);
    ~QmlProfilerDataModel();

    const QVector<QmlEventData> &getEvents() const;
    const QVector<QmlEventTypeData> &getEventTypes() const;
    const QVector<QmlEventNoteData> &getEventNotes() const;
    void setData(qint64 traceStart, qint64 traceEnd, const QVector<QmlEventTypeData> &types,
                 const QVector<QmlEventData> &events);
    void setNoteData(const QVector<QmlEventNoteData> &notes);
    void processData();

    int count() const;
    void clear();
    bool isEmpty() const;
    void addQmlEvent(QmlDebug::Message message, QmlDebug::RangeType rangeType, int bindingType,
                     qint64 startTime, qint64 duration, const QString &data,
                     const QmlDebug::QmlEventLocation &location, qint64 ndata1, qint64 ndata2,
                     qint64 ndata3, qint64 ndata4, qint64 ndata5);
    qint64 lastTimeMark() const;

signals:
    void changed();
    void requestReload();

protected slots:
    void detailsChanged(int requestId, const QString &newString);
    void detailsDone();

private:
    class QmlProfilerDataModelPrivate;
    QmlProfilerDataModelPrivate *d_ptr;
    Q_DECLARE_PRIVATE(QmlProfilerDataModel)
};

}

#endif
