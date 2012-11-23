#ifndef QMLWARNINGDIALOG_H
#define QMLWARNINGDIALOG_H

#include <QDialog>

QT_BEGIN_NAMESPACE
namespace Ui {
    class QmlWarningDialog;
}
QT_END_NAMESPACE

namespace QmlDesigner {

namespace Internal {

class QmlWarningDialog : public QDialog
{
    Q_OBJECT

public:
    explicit QmlWarningDialog(QWidget *parent, const QStringList &warnings);
    ~QmlWarningDialog();

    bool warningsEnabled() const;

public slots:
    void ignoreButtonPressed();
    void okButtonPressed();
    void checkBoxToggled(bool);
    void linkClicked(const QString &link);

private:
    Ui::QmlWarningDialog *ui;
    const QStringList m_warnings;
};

} //Internal

} //QmlDesigner

#endif // QMLWARNINGDIALOG_H
