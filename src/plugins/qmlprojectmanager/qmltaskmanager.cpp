#include "qmltaskmanager.h"
#include "qmlprojectconstants.h"
#include <QDebug>

namespace QmlProjectManager {
namespace Internal {

QmlTaskManager::QmlTaskManager(QObject *parent) :
        QObject(parent),
        m_taskWindow(0)
{
}

void QmlTaskManager::setTaskWindow(ProjectExplorer::TaskWindow *taskWindow)
{
    Q_ASSERT(taskWindow);
    m_taskWindow = taskWindow;

    m_taskWindow->addCategory(Constants::TASK_CATEGORY_QML, "Qml");
}

void QmlTaskManager::documentUpdated(QmlEditor::QmlDocument::Ptr doc)
{
    m_taskWindow->clearTasks(Constants::TASK_CATEGORY_QML);

    foreach (const QmlJS::DiagnosticMessage &msg, doc->diagnosticMessages()) {
        ProjectExplorer::TaskWindow::TaskType type
                = msg.isError() ? ProjectExplorer::TaskWindow::Error
                                : ProjectExplorer::TaskWindow::Warning;

        ProjectExplorer::TaskWindow::Task task(type, msg.message, doc->fileName(), msg.loc.startLine,
                                                Constants::TASK_CATEGORY_QML);
        m_taskWindow->addTask(task);
    }
}

} // Internal
} // QmlEditor
