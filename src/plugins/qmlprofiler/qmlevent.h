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

#include <QString>

namespace QmlProfiler {

struct QmlEvent {
    QmlEvent(qint64 startTime = -1, qint64 duration = -1, int typeIndex = -1,
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

    QmlEvent(qint64 startTime, qint64 duration, int typeIndex, const QString &data)
        : m_startTime(startTime), m_duration(duration), m_typeIndex(typeIndex)
    {
        assignStringData(data);
    }

    QmlEvent(const QmlEvent &other) :
        m_startTime(other.m_startTime), m_duration(other.m_duration),
        m_dataType(other.m_dataType), m_typeIndex(other.m_typeIndex)
    {
        assignData(other);
    }

    QmlEvent &operator=(const QmlEvent &other)
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

    ~QmlEvent()
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

    bool isValid() const
    {
        return m_startTime != -1;
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

    void assignData(const QmlEvent &other)
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

} // namespace QmlProfiler
