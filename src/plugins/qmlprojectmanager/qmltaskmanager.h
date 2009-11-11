#ifndef QMLTASKMANAGER_H
#define QMLTASKMANAGER_H

#include <projectexplorer/taskwindow.h>
#include <qmleditor/qmldocument.h>
#include <QtCore/QObject>


namespace QmlProjectManager {
namespace Internal {

class QmlTaskManager : public QObject
{
    Q_OBJECT
public:
    QmlTaskManager(QObject *parent = 0);
    void setTaskWindow(ProjectExplorer::TaskWindow *taskWindow);

public slots:
    void documentUpdated(QmlEditor::QmlDocument::Ptr doc);

private:
    ProjectExplorer::TaskWindow *m_taskWindow;
};

} // Internal
} // QmlProjectManager

#endif // QMLTASKMANAGER_H
