#include "startexternalqmldialog.h"
#include "ui_startexternalqmldialog.h"

#include <utils/pathchooser.h>
#include <QPushButton>


namespace Qml {
namespace Internal {


StartExternalQmlDialog::StartExternalQmlDialog(QWidget *parent)
  : QDialog(parent), m_ui(new Ui::StartExternalQmlDialog), m_debugMode(QmlProjectWithCppPlugins)
{
    m_ui->setupUi(this);
    m_ui->buttonBox->button(QDialogButtonBox::Ok)->setDefault(true);
    m_ui->projectDisplayName->setText(tr("<No project>"));

    connect(m_ui->buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(m_ui->buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
}

StartExternalQmlDialog::~StartExternalQmlDialog()
{
    delete m_ui;
}

void StartExternalQmlDialog::setDebuggerUrl(const QString &str)
{
    m_ui->urlLine->setText(str);
}

QString StartExternalQmlDialog::debuggerUrl() const
{
    return m_ui->urlLine->text();
}

void StartExternalQmlDialog::setPort(quint16 str)
{
    m_ui->portSpinBox->setValue(str);
}

quint16 StartExternalQmlDialog::port() const
{
    return m_ui->portSpinBox->value();
}

void StartExternalQmlDialog::setProjectDisplayName(const QString &projectName)
{
    m_ui->projectDisplayName->setText(projectName);
}

void StartExternalQmlDialog::setQmlViewerPath(const QString &path)
{
    m_ui->viewerPathLineEdit->setText(path);
}

QString StartExternalQmlDialog::qmlViewerPath() const
{
    return m_ui->viewerPathLineEdit->text();
}

void StartExternalQmlDialog::setQmlViewerArguments(const QString &arguments)
{
    m_ui->viewerArgumentsLineEdit->setText(arguments);
}

QString StartExternalQmlDialog::qmlViewerArguments() const
{
    return m_ui->viewerArgumentsLineEdit->text();
}

void StartExternalQmlDialog::setDebugMode(DebugMode mode)
{
    m_debugMode = mode;
    if (m_debugMode == QmlProjectWithCppPlugins) {
        m_ui->labelViewerPath->show();
        m_ui->viewerPathLineEdit->show();
        m_ui->labelViewerArguments->show();
        m_ui->viewerArgumentsLineEdit->show();
        setMinimumHeight(270);
        resize(width(), 270);
    } else {
        m_ui->labelViewerPath->hide();
        m_ui->viewerPathLineEdit->hide();
        m_ui->labelViewerArguments->hide();
        m_ui->viewerArgumentsLineEdit->hide();
        setMinimumHeight(200);
        resize(width(), 200);
    }
}

} // Internal
} // Qml
