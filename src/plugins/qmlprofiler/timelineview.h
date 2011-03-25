#ifndef TIMELINEVIEW_H
#define TIMELINEVIEW_H

#include <QtDeclarative/QDeclarativeItem>
#include <QtScript/QScriptValue>

namespace QmlProfiler {
namespace Internal {

class TimelineView : public QDeclarativeItem
{
    Q_OBJECT
    Q_PROPERTY(QDeclarativeComponent *delegate READ delegate WRITE setDelegate NOTIFY delegateChanged)
    Q_PROPERTY(qint64 startTime READ startTime WRITE setStartTime NOTIFY startTimeChanged)
    Q_PROPERTY(qint64 endTime READ endTime WRITE setEndTime NOTIFY endTimeChanged)
    Q_PROPERTY(qreal startX READ startX WRITE setStartX NOTIFY startXChanged)
    Q_PROPERTY(qreal totalWidth READ totalWidth NOTIFY totalWidthChanged)

public:
    explicit TimelineView(QDeclarativeItem *parent = 0);

    QDeclarativeComponent * delegate() const
    {
        return m_delegate;
    }

    qint64 startTime() const
    {
        return m_startTime;
    }

    qint64 endTime() const
    {
        return m_endTime;
    }

    qreal startX() const
    {
        return m_startX;
    }

    qreal totalWidth() const
    {
        return m_totalWidth;
    }

signals:
    void delegateChanged(QDeclarativeComponent * arg);
    void startTimeChanged(qint64 arg);
    void endTimeChanged(qint64 arg);
    void startXChanged(qreal arg);
    void totalWidthChanged(qreal arg);

public slots:
    void clearData();
    void setRanges(const QScriptValue &value);
    void updateTimeline(bool updateStartX = true);

    void setDelegate(QDeclarativeComponent * arg)
    {
        if (m_delegate != arg) {
            m_delegate = arg;
            emit delegateChanged(arg);
        }
    }

    void setStartTime(qint64 arg)
    {
        if (m_startTime != arg) {
            m_startTime = arg;
            emit startTimeChanged(arg);
        }
    }

    void setEndTime(qint64 arg)
    {
        if (m_endTime != arg) {
            m_endTime = arg;
            emit endTimeChanged(arg);
        }
    }

    void setStartX(qreal arg);

protected:
    void componentComplete();

private:
    QDeclarativeComponent * m_delegate;
    QScriptValue m_ranges;
    typedef QList<QScriptValue> ValueList;
    QList<ValueList> m_rangeList;
    QHash<int,QDeclarativeItem*> m_items;
    qint64 m_startTime;
    qint64 m_endTime;
    qreal m_startX;
    int prevMin;
    int prevMax;

    struct PrevLimits {
        PrevLimits(int _min, int _max) : min(_min), max(_max) {}
        int min;
        int max;
    };

    QList<PrevLimits> m_prevLimits;
    qreal m_totalWidth;
};

} // namespace Internal
} // namespace QmlProfiler

QML_DECLARE_TYPE(QmlProfiler::Internal::TimelineView)

#endif // TIMELINEVIEW_H
