#ifndef VIRTUALSERIALPORT_H
#define VIRTUALSERIALPORT_H

#include <QtCore/QIODevice>
#include <QtCore/QString>
#include <QtCore/QMutex>

QT_BEGIN_NAMESPACE
class QWaitCondition;
QT_END_NAMESPACE

#include "symbianutils_global.h"

namespace SymbianUtils {

class VirtualSerialDevicePrivate;

class SYMBIANUTILS_EXPORT VirtualSerialDevice : public QIODevice
{
    Q_OBJECT
public:
    explicit VirtualSerialDevice(const QString &name, QObject *parent = 0);
    ~VirtualSerialDevice();

    bool open(OpenMode mode);
    void close();
    const QString &getPortName() const;
    void flush();

    qint64 bytesAvailable() const;
    bool isSequential() const;
    bool waitForBytesWritten(int msecs);
    bool waitForReadyRead(int msecs);

protected:
    qint64 readData(char *data, qint64 maxSize);
    qint64 writeData(const char *data, qint64 maxSize);

private:
    Q_DISABLE_COPY(VirtualSerialDevice)
    void platInit();
    void platClose();
    void emitBytesWrittenIfNeeded(QMutexLocker &locker, qint64 len);

private:
    QString portName;
    mutable QMutex lock;
    QList<QByteArray> pendingWrites;
    bool emittingBytesWritten;
    QWaitCondition* waiterForBytesWritten;
    VirtualSerialDevicePrivate *d;

// Platform-specific stuff
#ifdef Q_OS_WIN
private:
    qint64 writeNextBuffer(QMutexLocker &locker);
    void doWriteCompleted(QMutexLocker &locker);
private slots:
    void writeCompleted();
    void commEventOccurred();
#endif

#ifdef Q_OS_UNIX
private:
    bool tryWrite(const char *data, qint64 maxSize, qint64 &bytesWritten);
    enum FlushPendingOption {
        NothingSpecial = 0,
        StopAfterWritingOneBuffer = 1,
        EmitBytesWrittenAsync = 2, // Needed so we don't emit bytesWritten signal directly from writeBytes
    };
    Q_DECLARE_FLAGS(FlushPendingOptions, FlushPendingOption)
    bool tryFlushPendingBuffers(QMutexLocker& locker, FlushPendingOptions flags = NothingSpecial);

private slots:
    void writeHasUnblocked(int fileHandle);

signals:
    void AsyncCall_emitBytesWrittenIfNeeded(qint64 len);

#endif

};

} // namespace SymbianUtils

#endif // VIRTUALSERIALPORT_H
