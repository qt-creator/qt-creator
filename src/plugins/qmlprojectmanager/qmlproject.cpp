/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "qmlproject.h"
#include "qmlprojectconstants.h"

#include <projectexplorer/toolchain.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <extensionsystem/pluginmanager.h>
#include <utils/pathchooser.h>
#include <utils/qtcassert.h>
#include <coreplugin/icore.h>
#include <coreplugin/editormanager/editormanager.h>

#include <QtCore/QtDebug>
#include <QtCore/QDir>
#include <QtCore/QSettings>
#include <QtCore/QProcess>
#include <QtCore/QCoreApplication>

#include <QtGui/QFormLayout>
#include <QtGui/QMainWindow>
#include <QtGui/QComboBox>
#include <QtGui/QStringListModel>
#include <QtGui/QListWidget>
#include <QtGui/QPushButton>

using namespace QmlProjectManager;
using namespace QmlProjectManager::Internal;

namespace {

/**
 * An editable string list model. New strings can be added by editing the entry
 * called "<new>", displayed at the end.
 */
class ListModel: public QStringListModel
{
public:
    ListModel(QObject *parent)
        : QStringListModel(parent) {}

    virtual ~ListModel() {}

    virtual int rowCount(const QModelIndex &parent) const
    { return 1 + QStringListModel::rowCount(parent); }

    virtual Qt::ItemFlags flags(const QModelIndex &index) const
    { return QStringListModel::flags(index) | Qt::ItemIsEditable | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled; }

    virtual QModelIndex index(int row, int column, const QModelIndex &parent) const
    {
        if (row == stringList().size())
            return createIndex(row, column);

        return QStringListModel::index(row, column, parent);
    }

    virtual QVariant data(const QModelIndex &index, int role) const
    {
        if (role == Qt::DisplayRole || role == Qt::EditRole) {
            if (index.row() == stringList().size())
                return QCoreApplication::translate("QmlProject", "<new>");
        }

        return QStringListModel::data(index, role);
    }

    virtual bool setData(const QModelIndex &index, const QVariant &value, int role)
    {
        if (role == Qt::EditRole && index.row() == stringList().size())
            insertRow(index.row(), QModelIndex());

        return QStringListModel::setData(index, value, role);
    }
};

} // end of anonymous namespace

////////////////////////////////////////////////////////////////////////////////////
// QmlProject
////////////////////////////////////////////////////////////////////////////////////

QmlProject::QmlProject(Manager *manager, const QString &fileName)
    : m_manager(manager),
      m_fileName(fileName)
{
    QFileInfo fileInfo(m_fileName);
    QDir dir = fileInfo.dir();

    m_projectName      = fileInfo.completeBaseName();
    m_filesFileName    = QFileInfo(dir, m_projectName + QLatin1String(".files")).absoluteFilePath();

    m_file = new QmlProjectFile(this, fileName);
    m_rootNode = new QmlProjectNode(this, m_file);

    m_manager->registerProject(this);

    QSharedPointer<QmlApplicationRunConfiguration> runConf(new QmlApplicationRunConfiguration(this));
    addRunConfiguration(runConf);
}

QmlProject::~QmlProject()
{
    m_manager->unregisterProject(this);

    delete m_rootNode;
}

QString QmlProject::filesFileName() const
{ return m_filesFileName; }

static QStringList readLines(const QString &absoluteFileName)
{
    QStringList lines;

    QFile file(absoluteFileName);
    if (file.open(QFile::ReadOnly)) {
        QTextStream stream(&file);

        forever {
            QString line = stream.readLine();
            if (line.isNull())
                break;

            line = line.trimmed();
            if (line.isEmpty())
                continue;

            lines.append(line);
        }
    }

    return lines;
}


void QmlProject::parseProject(RefreshOptions options)
{
    if (options & Files) {
        m_files = convertToAbsoluteFiles(readLines(filesFileName()));
        m_files.removeDuplicates();
    }

    if (options & Configuration) {
        // update configuration
    }

    if (options & Files)
        emit fileListChanged();
}

void QmlProject::refresh(RefreshOptions options)
{
    QSet<QString> oldFileList;
    if (!(options & Configuration))
        oldFileList = m_files.toSet();

    parseProject(options);

    if (options & Files)
        m_rootNode->refresh();
}

QStringList QmlProject::convertToAbsoluteFiles(const QStringList &paths) const
{
    const QDir projectDir(QFileInfo(m_fileName).dir());
    QStringList absolutePaths;
    foreach (const QString &file, paths) {
        QFileInfo fileInfo(projectDir, file);
        absolutePaths.append(fileInfo.absoluteFilePath());
    }
    absolutePaths.removeDuplicates();
    return absolutePaths;
}

QStringList QmlProject::files() const
{ return m_files; }

QString QmlProject::buildParser(const QString &) const
{
    return QString();
}

QString QmlProject::name() const
{
    return m_projectName;
}

Core::IFile *QmlProject::file() const
{
    return m_file;
}

ProjectExplorer::IProjectManager *QmlProject::projectManager() const
{
    return m_manager;
}

QList<ProjectExplorer::Project *> QmlProject::dependsOn()
{
    return QList<Project *>();
}

bool QmlProject::isApplication() const
{
    return true;
}

ProjectExplorer::Environment QmlProject::environment(const QString &) const
{
    return ProjectExplorer::Environment::systemEnvironment();
}

QString QmlProject::buildDirectory(const QString &) const
{
    return QString();
}

ProjectExplorer::BuildStepConfigWidget *QmlProject::createConfigWidget()
{
    return new QmlBuildSettingsWidget(this);
}

QList<ProjectExplorer::BuildStepConfigWidget*> QmlProject::subConfigWidgets()
{
    return QList<ProjectExplorer::BuildStepConfigWidget*>();
}

void QmlProject::newBuildConfiguration(const QString &)
{
}

QmlProjectNode *QmlProject::rootProjectNode() const
{
    return m_rootNode;
}

QStringList QmlProject::files(FilesMode) const
{
    return m_files;
}

QStringList QmlProject::targets() const
{
    QStringList targets;
    return targets;
}

QmlMakeStep *QmlProject::makeStep() const
{
    return 0;
}

void QmlProject::restoreSettingsImpl(ProjectExplorer::PersistentSettingsReader &reader)
{
    Project::restoreSettingsImpl(reader);

    refresh(Everything);
}

void QmlProject::saveSettingsImpl(ProjectExplorer::PersistentSettingsWriter &writer)
{
    Project::saveSettingsImpl(writer);
}

////////////////////////////////////////////////////////////////////////////////////
// QmlBuildSettingsWidget
////////////////////////////////////////////////////////////////////////////////////

QmlBuildSettingsWidget::QmlBuildSettingsWidget(QmlProject *project)
    : m_project(project)
{
}

QmlBuildSettingsWidget::~QmlBuildSettingsWidget()
{ }

QString QmlBuildSettingsWidget::displayName() const
{ return tr("QML Manager"); }

void QmlBuildSettingsWidget::init(const QString &)
{
}

////////////////////////////////////////////////////////////////////////////////////
// QmlProjectFile
////////////////////////////////////////////////////////////////////////////////////

QmlProjectFile::QmlProjectFile(QmlProject *parent, QString fileName)
    : Core::IFile(parent),
      m_project(parent),
      m_fileName(fileName)
{ }

QmlProjectFile::~QmlProjectFile()
{ }

bool QmlProjectFile::save(const QString &)
{
    return false;
}

QString QmlProjectFile::fileName() const
{
    return m_fileName;
}

QString QmlProjectFile::defaultPath() const
{
    return QString();
}

QString QmlProjectFile::suggestedFileName() const
{
    return QString();
}

QString QmlProjectFile::mimeType() const
{
    return Constants::QMLMIMETYPE;
}

bool QmlProjectFile::isModified() const
{
    return false;
}

bool QmlProjectFile::isReadOnly() const
{
    return true;
}

bool QmlProjectFile::isSaveAsAllowed() const
{
    return false;
}

void QmlProjectFile::modified(ReloadBehavior *)
{
}

QmlApplicationRunConfiguration::QmlApplicationRunConfiguration(QmlProject *pro)
    : ProjectExplorer::ApplicationRunConfiguration(pro),
      m_project(pro)
{
    setName(tr("QML Viewer"));
}

QmlApplicationRunConfiguration::~QmlApplicationRunConfiguration()
{
}

QString QmlApplicationRunConfiguration::type() const
{
    return QLatin1String("QmlProject.QmlApplicationRunConfiguration");
}

QString QmlApplicationRunConfiguration::executable() const
{
    QString executable("qmlviewer");
    return executable;
}

QmlApplicationRunConfiguration::RunMode QmlApplicationRunConfiguration::runMode() const
{
    return Gui;
}

QString QmlApplicationRunConfiguration::workingDirectory() const
{
    QFileInfo projectFile(m_project->file()->fileName());
    return projectFile.filePath();
}

QStringList QmlApplicationRunConfiguration::commandLineArguments() const
{
    QStringList args;

    const QString s = mainScript();
    if (! s.isEmpty())
        args.append(s);

    return args;
}

ProjectExplorer::Environment QmlApplicationRunConfiguration::environment() const
{
    ProjectExplorer::Environment env;
    return env;
}

QString QmlApplicationRunConfiguration::dumperLibrary() const
{
    return QString();
}

QWidget *QmlApplicationRunConfiguration::configurationWidget()
{
    QComboBox *combo = new QComboBox;
    combo->addItem(tr("<Current File>"));
    connect(combo, SIGNAL(activated(QString)), this, SLOT(setMainScript(QString)));
    combo->addItems(m_project->files());
    return combo;
}

QString QmlApplicationRunConfiguration::mainScript() const
{
    if (m_scriptFile.isEmpty() || m_scriptFile == tr("<Current File>")) {
        Core::EditorManager *editorManager = Core::ICore::instance()->editorManager();
        if (Core::IEditor *editor = editorManager->currentEditor()) {
            return editor->file()->fileName();
        }
    }

    return m_scriptFile;
}

void QmlApplicationRunConfiguration::setMainScript(const QString &scriptFile)
{
    m_scriptFile = scriptFile;
}

void QmlApplicationRunConfiguration::save(ProjectExplorer::PersistentSettingsWriter &writer) const
{
    ProjectExplorer::ApplicationRunConfiguration::save(writer);
}

void QmlApplicationRunConfiguration::restore(const ProjectExplorer::PersistentSettingsReader &reader)
{
    ProjectExplorer::ApplicationRunConfiguration::restore(reader);
}
