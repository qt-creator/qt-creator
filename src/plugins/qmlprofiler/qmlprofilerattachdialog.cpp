#include "qmlprofilerattachdialog.h"
#include "ui_qmlprofilerattachdialog.h"

namespace QmlProfiler {
namespace Internal {

QmlProfilerAttachDialog::QmlProfilerAttachDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::QmlProfilerAttachDialog)
{
    ui->setupUi(this);
}

QmlProfilerAttachDialog::~QmlProfilerAttachDialog()
{
    delete ui;
}

QString QmlProfilerAttachDialog::address() const
{
    return ui->addressLineEdit->text();
}

uint QmlProfilerAttachDialog::port() const
{
    return ui->portSpinBox->value();
}

void QmlProfilerAttachDialog::setAddress(const QString &address)
{
    ui->addressLineEdit->setText(address);
}

void QmlProfilerAttachDialog::setPort(uint port)
{
    ui->portSpinBox->setValue(port);
}

} // namespace Internal
} // namespace QmlProfiler
