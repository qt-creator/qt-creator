/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "genericprojectwizard.h"

#include <coreplugin/icore.h>
#include <coreplugin/mimedatabase.h>
#include <projectexplorer/projectexplorer.h>

#include <utils/filenamevalidatinglineedit.h>
#include <utils/filewizardpage.h>
#include <utils/pathchooser.h>

#include <QtCore/QDir>
#include <QtCore/QtDebug>

#include <QtGui/QDirModel>
#include <QtGui/QFormLayout>
#include <QtGui/QListView>
#include <QtGui/QTreeView>

using namespace GenericProjectManager::Internal;
using namespace Utils;

namespace {

class DirModel : public QDirModel
{
public:
    DirModel(QObject *parent)
        : QDirModel(parent)
    { setFilter(QDir::Dirs | QDir::NoDotAndDotDot); }

    virtual ~DirModel()
    { }

public:
    virtual int columnCount(const QModelIndex &) const
    { return 1; }

    virtual Qt::ItemFlags flags(const QModelIndex &index) const
    { return QDirModel::flags(index) | Qt::ItemIsUserCheckable; }

    virtual QVariant data(const QModelIndex &index, int role) const
    {
        if (index.column() == 0 && role == Qt::CheckStateRole) {
            if (m_selectedPaths.contains(index))
                return Qt::Checked;

            return Qt::Unchecked;
        }

        return QDirModel::data(index, role);
    }

    virtual bool setData(const QModelIndex &index, const QVariant &value, int role)
    {
        if (index.column() == 0 && role == Qt::CheckStateRole) {
            if (value.toBool())
                m_selectedPaths.insert(index);
            else
                m_selectedPaths.remove(index);

            return true;
        }

        return QDirModel::setData(index, value, role);
    }

    void clearSelectedPaths()
    { m_selectedPaths.clear(); }

    QSet<QString> selectedPaths() const
    {
        QSet<QString> paths;

        foreach (const QModelIndex &index, m_selectedPaths)
            paths.insert(filePath(index));

        return paths;
    }

private:
    QSet<QModelIndex> m_selectedPaths;
};

} // end of anonymous namespace


//////////////////////////////////////////////////////////////////////////////
// GenericProjectWizardDialog
//////////////////////////////////////////////////////////////////////////////


GenericProjectWizardDialog::GenericProjectWizardDialog(QWidget *parent)
    : QWizard(parent)
{
    setWindowTitle(tr("Import of Makefile-based Project"));

    // first page
    m_firstPage = new FileWizardPage;
    m_firstPage->setTitle(tr("Generic Project"));
    m_firstPage->setNameLabel(tr("Project name:"));
    m_firstPage->setPathLabel(tr("Location:"));

    addPage(m_firstPage);

#if 0
    // second page
    QWizardPage *secondPage = new QWizardPage;
    secondPage->setTitle(tr("Second Page Title"));

    QFormLayout *secondPageLayout = new QFormLayout(secondPage);

    m_dirView = new QTreeView;
    m_dirModel = new DirModel(this);
    m_dirView->setModel(_dirModel);

    Core::ICore *core = Core::ICore::instance();
    Core::MimeDatabase *mimeDatabase = core->mimeDatabase();

    const QStringList suffixes = mimeDatabase->suffixes();

    QStringList nameFilters;
    foreach (const QString &suffix, suffixes) {
        QString nameFilter;
        nameFilter.append(QLatin1String("*."));
        nameFilter.append(suffix);
        nameFilters.append(nameFilter);
    }

    m_filesView = new QListView;
    m_filesModel = new QDirModel(this);
    m_filesModel->setNameFilters(nameFilters);
    m_filesModel->setFilter(QDir::Files);

    connect(_dirView->selectionModel(), SIGNAL(currentChanged(QModelIndex,QModelIndex)),
            this, SLOT(updateFilesView(QModelIndex,QModelIndex)));

    secondPageLayout->addRow(_dirView);
    secondPageLayout->addRow(_filesView);

    m_secondPageId = addPage(secondPage);
#endif
}

GenericProjectWizardDialog::~GenericProjectWizardDialog()
{ }

QString GenericProjectWizardDialog::path() const
{
    return m_firstPage->path();
}

void GenericProjectWizardDialog::setPath(const QString &path)
{
    m_firstPage->setPath(path);
}

QString GenericProjectWizardDialog::projectName() const
{
    return m_firstPage->name();
}

void GenericProjectWizardDialog::updateFilesView(const QModelIndex &current,
                                                 const QModelIndex &)
{
    if (! current.isValid())
        m_filesView->setModel(0);

    else {
        const QString selectedPath = m_dirModel->filePath(current);

        if (! m_filesView->model())
            m_filesView->setModel(m_filesModel);

        m_filesView->setRootIndex(m_filesModel->index(selectedPath));
    }
}

void GenericProjectWizardDialog::initializePage(int id)
{
    Q_UNUSED(id)
#if 0
    if (id == m_secondPageId) {
        using namespace Utils;

        const QString projectPath = m_pathChooser->path();

        QDirModel *dirModel = qobject_cast<QDirModel *>(_dirView->model());
        m_dirView->setRootIndex(dirModel->index(projectPath));
    }
#endif
}

bool GenericProjectWizardDialog::validateCurrentPage()
{
    using namespace Utils;

#if 0
    if (currentId() == m_secondPageId) {
        return true;
    }
#endif

    return QWizard::validateCurrentPage();
}

GenericProjectWizard::GenericProjectWizard()
    : Core::BaseFileWizard(parameters())
{ }

GenericProjectWizard::~GenericProjectWizard()
{ }

Core::BaseFileWizardParameters GenericProjectWizard::parameters()
{
    static Core::BaseFileWizardParameters parameters(ProjectWizard);
    parameters.setIcon(QIcon(":/wizards/images/console.png"));
    parameters.setName(tr("Import of Makefile-based Project"));
    parameters.setDescription(tr("Creates a generic project, supporting any build system."));
    parameters.setCategory(QLatin1String("Projects"));
    parameters.setTrCategory(tr("Projects"));
    return parameters;
}

QWizard *GenericProjectWizard::createWizardDialog(QWidget *parent,
                                                  const QString &defaultPath,
                                                  const WizardPageList &extensionPages) const
{
    GenericProjectWizardDialog *wizard = new GenericProjectWizardDialog(parent);
    setupWizard(wizard);

    wizard->setPath(defaultPath);

    foreach (QWizardPage *p, extensionPages)
        wizard->addPage(p);

    return wizard;
}

void GenericProjectWizard::getFileList(const QDir &dir, const QString &projectRoot,
                                       const QStringList &suffixes,
                                       QStringList *files, QStringList *paths) const
{
    const QFileInfoList fileInfoList = dir.entryInfoList(QDir::Files |
                                                         QDir::Dirs |
                                                         QDir::NoDotAndDotDot |
                                                         QDir::NoSymLinks);

    foreach (const QFileInfo &fileInfo, fileInfoList) {
        QString filePath = fileInfo.absoluteFilePath();
        filePath = filePath.mid(projectRoot.length() + 1);

        if (fileInfo.isDir() && isValidDir(fileInfo)) {
            getFileList(QDir(fileInfo.absoluteFilePath()), projectRoot,
                        suffixes, files, paths);

            if (! paths->contains(filePath))
                paths->append(filePath);
        }

        else if (suffixes.contains(fileInfo.suffix()))
            files->append(filePath);
    }
}

bool GenericProjectWizard::isValidDir(const QFileInfo &fileInfo) const
{
    const QString fileName = fileInfo.fileName();
    const QString suffix = fileInfo.suffix();

    if (fileName.startsWith(QLatin1Char('.')))
        return false;

    else if (fileName == QLatin1String("CVS"))
        return false;

    // ### user include/exclude

    return true;
}

Core::GeneratedFiles GenericProjectWizard::generateFiles(const QWizard *w,
                                                         QString *errorMessage) const
{
    Q_UNUSED(errorMessage)

    const GenericProjectWizardDialog *wizard = qobject_cast<const GenericProjectWizardDialog *>(w);
    const QString projectPath = wizard->path();
    const QDir dir(projectPath);
    const QString projectName = wizard->projectName();
    const QString creatorFileName = QFileInfo(dir, projectName + QLatin1String(".creator")).absoluteFilePath();
    const QString filesFileName = QFileInfo(dir, projectName + QLatin1String(".files")).absoluteFilePath();
    const QString includesFileName = QFileInfo(dir, projectName + QLatin1String(".includes")).absoluteFilePath();
    const QString configFileName = QFileInfo(dir, projectName + QLatin1String(".config")).absoluteFilePath();

    Core::ICore *core = Core::ICore::instance();
    Core::MimeDatabase *mimeDatabase = core->mimeDatabase();

    const QStringList suffixes = mimeDatabase->suffixes();

    QStringList sources, paths;
    getFileList(dir, projectPath, suffixes, &sources, &paths);

    Core::MimeType headerTy = mimeDatabase->findByType(QLatin1String("text/x-chdr"));

    QStringList nameFilters;
    foreach (const QRegExp &rx, headerTy.globPatterns())
        nameFilters.append(rx.pattern());

    QStringList includePaths;
    foreach (const QString &path, paths) {
        QFileInfo fileInfo(dir, path);
        QDir thisDir(fileInfo.absoluteFilePath());

        if (! thisDir.entryList(nameFilters, QDir::Files).isEmpty())
            includePaths.append(path);
    }

    Core::GeneratedFile generatedCreatorFile(creatorFileName);
    generatedCreatorFile.setContents(QLatin1String("[General]\n"));

    Core::GeneratedFile generatedFilesFile(filesFileName);
    generatedFilesFile.setContents(sources.join(QLatin1String("\n")));

    Core::GeneratedFile generatedIncludesFile(includesFileName);
    generatedIncludesFile.setContents(includePaths.join(QLatin1String("\n")));

    Core::GeneratedFile generatedConfigFile(configFileName);
    generatedConfigFile.setContents(QLatin1String("// ADD PREDEFINED MACROS HERE!\n"));

    Core::GeneratedFiles files;
    files.append(generatedFilesFile);
    files.append(generatedIncludesFile);
    files.append(generatedConfigFile);
    files.append(generatedCreatorFile);

    return files;
}

bool GenericProjectWizard::postGenerateFiles(const Core::GeneratedFiles &l, QString *errorMessage)
{
    // Post-Generate: Open the project
    const QString proFileName = l.back().path();
    if (!ProjectExplorer::ProjectExplorerPlugin::instance()->openProject(proFileName)) {
        *errorMessage = tr("The project %1 could not be opened.").arg(proFileName);
        return false;
    }
    return true;
}

