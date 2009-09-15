#ifndef MERCURIALJOBRUNNER_H
#define MERCURIALJOBRUNNER_H

#include <QtCore/QThread>
#include <QtCore/QQueue>
#include <QtCore/QMutex>
#include <QtCore/QWaitCondition>
#include <QtCore/QStringList>
#include <QtCore/QSharedPointer>
#include <QtCore/QString>

namespace VCSBase {
class VCSBaseEditor;
}

namespace Mercurial {
namespace Internal {

class MercurialPlugin;

class HgTask : public QObject
{
    Q_OBJECT
public:
    HgTask(const QString &workingDir, QStringList &arguments, bool emitRaw=false);
    HgTask(const QString &workingDir, QStringList &arguments,
           VCSBase::VCSBaseEditor *editor);

    bool shouldEmit() { return emitRaw; }
    VCSBase::VCSBaseEditor* displayEditor() { return editor; }
    QStringList args() { return arguments; }
    QString repositoryRoot() { return m_repositoryRoot; }

signals:
    void rawData(const QByteArray &data);

private:
    QString m_repositoryRoot;
    QStringList arguments;
    bool emitRaw;
    VCSBase::VCSBaseEditor *editor;
};


class MercurialJobRunner : public QThread
{
    Q_OBJECT
public:
    MercurialJobRunner();
    ~MercurialJobRunner();
    void enqueueJob(QSharedPointer<HgTask> &job);
    void restart();

protected:
    void run();

signals:
    void error(const QByteArray &error);
    void info(const QString &notice);
    void output(const QByteArray &output);

private:
    void task(QSharedPointer<HgTask> &job);
    void stop();
    void getSettings();

    QQueue<QSharedPointer<HgTask> > jobs;
    QMutex mutex;
    QWaitCondition waiter;
    MercurialPlugin *plugin;
    bool keepRunning;
    QString binary;
    QStringList standardArguments;
    int timeout;
};

} //namespace Internal
} //namespace Mercurial
#endif // MERCURIALJOBRUNNER_H
