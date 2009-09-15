#ifndef MERCURIALPLUGIN_H
#define MERCURIALPLUGIN_H

#include <extensionsystem/iplugin.h>
#include <coreplugin/icorelistener.h>

#include <QtCore/QFileInfo>
#include <QtCore/QHash>
#include <QtCore/qglobal.h>

QT_BEGIN_NAMESPACE
class QAction;
class QTemporaryFile;
QT_END_NAMESPACE

namespace Core {
class ActionManager;
class ActionContainer;
class ICore;
class IVersionControl;
class IEditorFactory;
class IEditor;

namespace Utils {
class ParameterAction;
} //namespace Utils
} // namespace Core

namespace ProjectExplorer {
class ProjectExplorerPlugin;
class Project;
}

namespace Mercurial {
namespace Internal {

class MercurialOutputWindow;
class OptionsPage;
class MercurialClient;
class MercurialControl;
class MercurialEditor;
class MercurialSettings;

class MercurialPlugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT

public:
    MercurialPlugin();
    virtual ~MercurialPlugin();
    bool initialize(const QStringList &arguments, QString *error_message);
    void extensionsInitialized();
    static MercurialPlugin *instance() { return m_instance; }
    QFileInfo currentFile();
    QString currentProjectName();
    QFileInfo currentProjectRoot();
    bool closeEditor(Core::IEditor *editor);

    MercurialSettings *settings();
    MercurialOutputWindow *outputPane();


private slots:
    // File menu action Slots
    void annotateCurrentFile();
    void diffCurrentFile();
    void logCurrentFile();
    void revertCurrentFile();
    void statusCurrentFile();

    //Directory menu Action slots
    void diffRepository();
    void logRepository();
    void revertMulti();
    void statusMulti();

    //repository menu action slots
    void pull();
    void push();
    void update();
    void import();
    void incoming();
    void outgoing();
    void commit();
    void showCommitWidget(const QList<QPair<QString, QString> > &status);
    void commitFromEditor();
    void diffFromEditorSelected(const QStringList &files);

    //TODO implement
   /* //repository management action slots
    void merge();
    void branch();
    void heads();
    void parents();
    void tags();
    void tip();
    void paths();

    //less used repository action
    void init();
    void serve();*/

    //change the sates of the actions in the Mercurial Menu i.e. 2 be context sensitive
    void updateActions();
    void currentProjectChanged(ProjectExplorer::Project *project);


private:
    //methods
    void createMenu();
    void createSubmitEditorActions();
    void createSeparator(const QList<int> &context, const QString &id);
    void createFileActions(QList<int> &context);
    void createDirectoryActions(QList<int> &context);
    void createRepositoryActions(QList<int> &context);
    void createRepositoryManagementActions(QList<int> &context);
    void createLessUsedActions(QList<int> &context);
    void deleteCommitLog();
    //QString getSettingsByKey(const char * const key);

    //Variables
    static MercurialPlugin *m_instance;
    MercurialSettings *mercurialSettings;
    MercurialOutputWindow *outputWindow;
    OptionsPage *optionsPage;
    MercurialClient *client;

    Core::IVersionControl *mercurialVC;
    Core::ICore *core;
    Core::ActionManager *actionManager;
    Core::ActionContainer *mercurialContainer;
    ProjectExplorer::ProjectExplorerPlugin *projectExplorer;

    //provide a mapping of projectName -> repositoryRoot for each project
    QHash<QString, QFileInfo> projectMapper;
    QList<QAction *> actionList;
    QTemporaryFile *changeLog;

    //Menu Items (file actions)
    Core::Utils::ParameterAction *annotateFile;
    Core::Utils::ParameterAction *diffFile;
    Core::Utils::ParameterAction *logFile;
    Core::Utils::ParameterAction *renameFile;
    Core::Utils::ParameterAction *revertFile;
    Core::Utils::ParameterAction *statusFile;

    //submit editor actions
    QAction *editorCommit;
    QAction *editorDiff;
    QAction *editorUndo;
    QAction *editorRedo;
};

class ListenForClose : public Core::ICoreListener
{
    Q_OBJECT
public:
    ListenForClose(QObject *parent=0) : Core::ICoreListener(parent) {}
    bool editorAboutToClose(Core::IEditor *editor);
};

} //namespace Internal
} //namespace Mercurial

#endif // MERCURIALPLUGIN_H
