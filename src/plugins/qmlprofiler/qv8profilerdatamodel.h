#ifndef QV8PROFILERDATAMODEL_H
#define QV8PROFILERDATAMODEL_H

#include <QObject>
#include <QHash>

#include <QXmlStreamReader>
#include <QXmlStreamWriter>

namespace QmlProfiler {
namespace Internal {

class QmlProfilerDataModel;
struct QV8EventSub;

struct QV8EventData
{
    QV8EventData();
    ~QV8EventData();

    QString displayName;
    QString eventHashStr;
    QString filename;
    QString functionName;
    int line;
    double totalTime; // given in milliseconds
    double totalPercent;
    double selfTime;
    double selfPercent;
    QHash <QString, QV8EventSub *> parentHash;
    QHash <QString, QV8EventSub *> childrenHash;
    int eventId;

    QV8EventData &operator=(const QV8EventData &ref);
};

struct QV8EventSub {
    QV8EventSub(QV8EventData *from) : reference(from), totalTime(0) {}
    QV8EventSub(QV8EventSub *from) : reference(from->reference), totalTime(from->totalTime) {}

    QV8EventData *reference;
    qint64 totalTime;
};

class QV8ProfilerDataModel : public QObject
{
    Q_OBJECT
public:
    explicit QV8ProfilerDataModel(QObject *parent, QmlProfilerDataModel *profilerDataModel);
    ~QV8ProfilerDataModel();

    void clear();
    bool isEmpty() const;
    QList<QV8EventData *> getV8Events() const;
    QV8EventData *v8EventDescription(int eventId) const;

    qint64 v8MeasuredTime() const;
    void collectV8Statistics();

    void save(QXmlStreamWriter &stream);
    void load(QXmlStreamReader &stream);

public slots:
    void addV8Event(int depth,
                    const QString &function,
                    const QString &filename,
                    int lineNumber,
                    double totalTime,
                    double selfTime);

private:
    class QV8ProfilerDataModelPrivate;
    QV8ProfilerDataModelPrivate *d;
};

} // namespace Internal
} // namespace QmlProfiler

#endif // QV8PROFILERDATAMODEL_H
