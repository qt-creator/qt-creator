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

#include "qmlprojectwizard.h"

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

using namespace QmlProjectManager::Internal;
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
// QmlProjectWizardDialog
//////////////////////////////////////////////////////////////////////////////


QmlProjectWizardDialog::QmlProjectWizardDialog(QWidget *parent)
    : QWizard(parent)
{
    setWindowTitle(tr("Import of QML Project"));

    // first page
    m_firstPage = new FileWizardPage;
    m_firstPage->setTitle(tr("QML Project"));
    m_firstPage->setNameLabel(tr("Project name:"));
    m_firstPage->setPathLabel(tr("Location:"));

    addPage(m_firstPage);
}

QmlProjectWizardDialog::~QmlProjectWizardDialog()
{ }

QString QmlProjectWizardDialog::path() const
{
    return m_firstPage->path();
}

void QmlProjectWizardDialog::setPath(const QString &path)
{
    m_firstPage->setPath(path);
}

QString QmlProjectWizardDialog::projectName() const
{
    return m_firstPage->name();
}

void QmlProjectWizardDialog::updateFilesView(const QModelIndex &current,
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

void QmlProjectWizardDialog::initializePage(int id)
{
    Q_UNUSED(id)
}

bool QmlProjectWizardDialog::validateCurrentPage()
{
    return QWizard::validateCurrentPage();
}

QmlProjectWizard::QmlProjectWizard()
    : Core::BaseFileWizard(parameters())
{ }

QmlProjectWizard::~QmlProjectWizard()
{ }

Core::BaseFileWizardParameters QmlProjectWizard::parameters()
{
    static Core::BaseFileWizardParameters parameters(ProjectWizard);
    parameters.setIcon(QIcon(":/wizards/images/console.png"));
    parameters.setName(tr("Import of existing QML directory"));
    parameters.setDescription(tr("Creates a QML project from an existing directory of QML files."));
    parameters.setCategory(QLatin1String("Projects"));
    parameters.setTrCategory(tr("Projects"));
    return parameters;
}

QWizard *QmlProjectWizard::createWizardDialog(QWidget *parent,
                                                  const QString &defaultPath,
                                                  const WizardPageList &extensionPages) const
{
    QmlProjectWizardDialog *wizard = new QmlProjectWizardDialog(parent);
    setupWizard(wizard);

    wizard->setPath(defaultPath);

    foreach (QWizardPage *p, extensionPages)
        wizard->addPage(p);

    return wizard;
}

void QmlProjectWizard::getFileList(const QDir &dir, const QString &projectRoot,
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

bool QmlProjectWizard::isValidDir(const QFileInfo &fileInfo) const
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

Core::GeneratedFiles QmlProjectWizard::generateFiles(const QWizard *w,
                                                     QString *errorMessage) const
{
    Q_UNUSED(errorMessage)

    const QmlProjectWizardDialog *wizard = qobject_cast<const QmlProjectWizardDialog *>(w);
    const QString projectPath = wizard->path();
    const QDir dir(projectPath);
    const QString projectName = wizard->projectName();
    const QString creatorFileName = QFileInfo(dir, projectName + QLatin1String(".qmlproject")).absoluteFilePath();

    Core::ICore *core = Core::ICore::instance();
    Core::MimeDatabase *mimeDatabase = core->mimeDatabase();

    const QStringList suffixes = mimeDatabase->suffixes();

    QStringList sources, paths;
    getFileList(dir, projectPath, suffixes, &sources, &paths);

    Core::GeneratedFile generatedCreatorFile(creatorFileName);
    generatedCreatorFile.setContents(sources.join(QLatin1String("\n")));

    Core::GeneratedFiles files;
    files.append(generatedCreatorFile);

    return files;
}

bool QmlProjectWizard::postGenerateFiles(const Core::GeneratedFiles &l, QString *errorMessage)
{
    // Post-Generate: Open the project
    const QString proFileName = l.back().path();
    if (!ProjectExplorer::ProjectExplorerPlugin::instance()->openProject(proFileName)) {
        *errorMessage = tr("The project %1 could not be opened.").arg(proFileName);
        return false;
    }
    return true;
}

