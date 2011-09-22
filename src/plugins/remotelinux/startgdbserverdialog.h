#ifndef STARTGDBSERVERDIALOG_H
#define STARTGDBSERVERDIALOG_H

#include "remotelinux_export.h"

#include <QtGui/QDialog>

namespace RemoteLinux {

namespace Internal {
class StartGdbServerDialogPrivate;
} // namespace Internal

class REMOTELINUX_EXPORT StartGdbServerDialog : public QDialog
{
    Q_OBJECT

public:
    explicit StartGdbServerDialog(QWidget *parent = 0);
    ~StartGdbServerDialog();

    void startGdbServer();

signals:
    void pidSelected(int pid);
    void portSelected(int port);
    void portOpened(int port);
    void processAborted();

private slots:
    void attachToDevice(int index);
    void handleRemoteError(const QString &errorMessage);
    void handleProcessListUpdated();
    void updateProcessList();
    void attachToProcess();
    void handleProcessKilled();
    void handleSelectionChanged();
    void portGathererError(const QString &errorMessage);
    void portListReady();

    void handleProcessClosed(int);
    void handleProcessErrorOutput(const QByteArray &data);
    void handleProcessOutputAvailable(const QByteArray &data);
    void handleProcessStarted();
    void handleConnectionError();

private:
    void startGdbServerOnPort(int port, int pid);
    Internal::StartGdbServerDialogPrivate *d;
};

} // namespace RemoteLinux

#endif // STARTGDBSERVERDIALOG_H
