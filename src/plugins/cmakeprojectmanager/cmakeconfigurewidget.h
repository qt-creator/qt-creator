#ifndef CMAKECONFIGUREWIDGET_H
#define CMAKECONFIGUREWIDGET_H

#include "ui_cmakeconfigurewidget.h"
#include <QtGui/QWidget>
#include <QtGui/QDialog>

namespace CMakeProjectManager {
namespace Internal {

class CMakeManager;

class CMakeConfigureWidget : public QWidget
{
    Q_OBJECT
public:
    CMakeConfigureWidget(QWidget *parent, CMakeManager *manager, const QString &sourceDirectory);
    Ui::CMakeConfigureWidget m_ui;

    QString buildDirectory();
    QStringList arguments();
    bool configureSucceded();

private slots:
    void runCMake();
private:
    bool m_configureSucceded;
    CMakeManager *m_cmakeManager;
    QString m_sourceDirectory;
};

class CMakeConfigureDialog : public QDialog
{
public:
    CMakeConfigureDialog(QWidget *parent, CMakeManager *manager, const QString &sourceDirectory);

    QString buildDirectory();
    QStringList arguments();
    bool configureSucceded();

private:
    CMakeConfigureWidget *m_cmakeConfigureWidget;
};

}
}

#endif // CMAKECONFIGUREWIDGET_H
