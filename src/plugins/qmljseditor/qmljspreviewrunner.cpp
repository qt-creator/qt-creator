#include "qmljspreviewrunner.h"

#include <projectexplorer/environment.h>
#include <utils/synchronousprocess.h>

#include <QtGui/QMessageBox>
#include <QtGui/QApplication>

#include <QDebug>

namespace QmlJSEditor {
namespace Internal {

QmlJSPreviewRunner::QmlJSPreviewRunner(QObject *parent) :
    QObject(parent)
{
    // prepend creator/bin dir to search path (only useful for special creator-qml package)
    const QString searchPath = QCoreApplication::applicationDirPath()
                               + Utils::SynchronousProcess::pathSeparator()
                               + QString(qgetenv("PATH"));
    m_qmlViewerDefaultPath = Utils::SynchronousProcess::locateBinary(searchPath, QLatin1String("qmlviewer"));

    ProjectExplorer::Environment environment = ProjectExplorer::Environment::systemEnvironment();
    m_applicationLauncher.setEnvironment(environment.toStringList());
}

bool QmlJSPreviewRunner::isReady() const
{
    return !m_qmlViewerDefaultPath.isEmpty();
}

void QmlJSPreviewRunner::run(const QString &filename)
{
    QString errorMessage;
    if (!filename.isEmpty()) {
        m_applicationLauncher.start(ProjectExplorer::ApplicationLauncher::Gui, m_qmlViewerDefaultPath,
                                    QStringList() << filename);

    } else {
        errorMessage = "No file specified.";
    }

    if (!errorMessage.isEmpty())
        QMessageBox::warning(0, tr("Failed to preview Qt Quick file"),
                             tr("Could not preview Qt Quick (QML) file. Reason: \n%1").arg(errorMessage));
}


} // namespace Internal
} // namespace QmlJSEditor
