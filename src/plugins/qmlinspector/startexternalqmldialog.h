#ifndef STARTEXTERNALQMLDIALOG_H
#define STARTEXTERNALQMLDIALOG_H

#include <QDialog>

QT_BEGIN_NAMESPACE
namespace Ui {
    class StartExternalQmlDialog;
}
QT_END_NAMESPACE

namespace Qml {
namespace Internal {


class StartExternalQmlDialog : public QDialog
{
    Q_OBJECT

public:
    explicit StartExternalQmlDialog(QWidget *parent);
    ~StartExternalQmlDialog();

    enum DebugMode {
        QmlProjectWithCppPlugins,
        CppProjectWithQmlEngine
    };

    void setDebugMode(DebugMode mode);

    void setQmlViewerPath(const QString &path);
    QString qmlViewerPath() const;

    void setQmlViewerArguments(const QString &arguments);
    QString qmlViewerArguments() const;

    void setDebuggerUrl(const QString &url);
    QString debuggerUrl() const;

    void setPort(quint16 port);
    quint16 port() const;

    void setProjectDisplayName(const QString &projectName);

private slots:

private:
    Ui::StartExternalQmlDialog *m_ui;
    DebugMode m_debugMode;
};

} // Internal
} // Qml

#endif // STARTEXTERNALQMLDIALOG_H
