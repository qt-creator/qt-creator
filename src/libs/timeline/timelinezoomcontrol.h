/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef TIMELINEZOOMCONTROL_H
#define TIMELINEZOOMCONTROL_H

#include "timeline_global.h"
#include <QTimer>

namespace Timeline {

class TIMELINE_EXPORT TimelineZoomControl : public QObject {
    Q_OBJECT

    Q_PROPERTY(qint64 traceStart READ traceStart NOTIFY traceChanged)
    Q_PROPERTY(qint64 traceEnd READ traceEnd NOTIFY traceChanged)
    Q_PROPERTY(qint64 traceDuration READ traceDuration NOTIFY traceChanged)

    Q_PROPERTY(qint64 windowStart READ windowStart NOTIFY windowChanged)
    Q_PROPERTY(qint64 windowEnd READ windowEnd NOTIFY windowChanged)
    Q_PROPERTY(qint64 windowDuration READ windowDuration NOTIFY windowChanged)

    Q_PROPERTY(qint64 rangeStart READ rangeStart NOTIFY rangeChanged)
    Q_PROPERTY(qint64 rangeEnd READ rangeEnd NOTIFY rangeChanged)
    Q_PROPERTY(qint64 rangeDuration READ rangeDuration NOTIFY rangeChanged)

    Q_PROPERTY(qint64 selectionStart READ selectionStart NOTIFY selectionChanged)
    Q_PROPERTY(qint64 selectionEnd READ selectionEnd NOTIFY selectionChanged)
    Q_PROPERTY(qint64 selectionDuration READ selectionDuration NOTIFY selectionChanged)

    Q_PROPERTY(bool windowLocked READ windowLocked WRITE setWindowLocked NOTIFY windowLockedChanged)

public:
    static const qint64 MAX_ZOOM_FACTOR = 1 << 10;

    TimelineZoomControl(QObject *parent = 0);
    qint64 traceStart() const { return m_traceStart; }
    qint64 traceEnd() const { return m_traceEnd; }
    qint64 traceDuration() const { return m_traceEnd - m_traceStart; }

    qint64 windowStart() const { return m_windowStart; }
    qint64 windowEnd() const { return m_windowEnd; }
    qint64 windowDuration() const { return m_windowEnd - m_windowStart; }

    qint64 rangeStart() const { return m_rangeStart; }
    qint64 rangeEnd() const { return m_rangeEnd; }
    qint64 rangeDuration() const { return m_rangeEnd - m_rangeStart; }

    qint64 selectionStart() const { return m_selectionStart; }
    qint64 selectionEnd() const { return m_selectionEnd; }
    qint64 selectionDuration() const { return m_selectionEnd - m_selectionStart; }

    bool windowLocked() const { return m_windowLocked; }
    virtual void clear();

signals:
    void traceChanged(qint64 start, qint64 end);
    void windowChanged(qint64 start, qint64 end);
    void rangeChanged(qint64 start, qint64 end);
    void selectionChanged(qint64 start, qint64 end);
    void windowLockedChanged(bool windowLocked);

public slots:
    void setTrace(qint64 start, qint64 end);
    void setRange(qint64 start, qint64 end);
    void setSelection(qint64 start, qint64 end);
    void setWindowLocked(bool windowLocked);

protected slots:
    void moveWindow();

protected:
    qint64 m_traceStart;
    qint64 m_traceEnd;
    qint64 m_windowStart;
    qint64 m_windowEnd;
    qint64 m_rangeStart;
    qint64 m_rangeEnd;
    qint64 m_selectionStart;
    qint64 m_selectionEnd;

    QTimer m_timer;
    bool m_windowLocked;

    void rebuildWindow();
    void clampRangeToWindow();
};

} // namespace Timeline

#endif // TIMELINEZOOMCONTROL_H
