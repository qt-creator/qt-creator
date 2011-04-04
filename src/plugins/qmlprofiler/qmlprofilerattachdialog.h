#ifndef QMLPROFILERATTACHDIALOG_H
#define QMLPROFILERATTACHDIALOG_H

#include <QDialog>

namespace QmlProfiler {
namespace Internal {

namespace Ui {
    class QmlProfilerAttachDialog;
}

class QmlProfilerAttachDialog : public QDialog
{
    Q_OBJECT

public:
    explicit QmlProfilerAttachDialog(QWidget *parent = 0);
    ~QmlProfilerAttachDialog();

    QString address() const;
    uint port() const;

    void setAddress(const QString &address);
    void setPort(uint port);

private:
    Ui::QmlProfilerAttachDialog *ui;
};

} // namespace Internal
} // namespace QmlProfiler

#endif // QMLPROFILERATTACHDIALOG_H
