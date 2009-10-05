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

#include "qmlnewprojectwizard.h"

#include <coreplugin/icore.h>
#include <coreplugin/mimedatabase.h>
#include <projectexplorer/projectexplorer.h>

#include <utils/filenamevalidatinglineedit.h>
#include <utils/filewizardpage.h>
#include <utils/pathchooser.h>
#include <utils/projectintropage.h>

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
// QmlNewProjectWizardDialog
//////////////////////////////////////////////////////////////////////////////


QmlNewProjectWizardDialog::QmlNewProjectWizardDialog(QWidget *parent)
    : QWizard(parent)
{
    setWindowTitle(tr("New QML Project"));

    m_introPage = new Utils::ProjectIntroPage();
    m_introPage->setDescription(tr("This wizard generates a QML application project."));

    addPage(m_introPage);
}

QmlNewProjectWizardDialog::~QmlNewProjectWizardDialog()
{ }

QString QmlNewProjectWizardDialog::path() const
{
    return m_introPage->path();
}

void QmlNewProjectWizardDialog::setPath(const QString &path)
{
    m_introPage->setPath(path);
}

QString QmlNewProjectWizardDialog::projectName() const
{
    return m_introPage->name();
}

void QmlNewProjectWizardDialog::updateFilesView(const QModelIndex &current,
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

void QmlNewProjectWizardDialog::initializePage(int id)
{
    Q_UNUSED(id)
}

bool QmlNewProjectWizardDialog::validateCurrentPage()
{
    return QWizard::validateCurrentPage();
}

QmlNewProjectWizard::QmlNewProjectWizard()
    : Core::BaseFileWizard(parameters())
{ }

QmlNewProjectWizard::~QmlNewProjectWizard()
{ }

Core::BaseFileWizardParameters QmlNewProjectWizard::parameters()
{
    static Core::BaseFileWizardParameters parameters(ProjectWizard);
    parameters.setIcon(QIcon(":/wizards/images/console.png"));
    parameters.setName(tr("QML Application"));
    parameters.setDescription(tr("Creates a QML application."));
    parameters.setCategory(QLatin1String("Projects"));
    parameters.setTrCategory(tr("Projects"));
    return parameters;
}

QWizard *QmlNewProjectWizard::createWizardDialog(QWidget *parent,
                                                  const QString &defaultPath,
                                                  const WizardPageList &extensionPages) const
{
    QmlNewProjectWizardDialog *wizard = new QmlNewProjectWizardDialog(parent);
    setupWizard(wizard);

    wizard->setPath(defaultPath);

    foreach (QWizardPage *p, extensionPages)
        wizard->addPage(p);

    return wizard;
}

Core::GeneratedFiles QmlNewProjectWizard::generateFiles(const QWizard *w,
                                                     QString *errorMessage) const
{
    Q_UNUSED(errorMessage)

    const QmlNewProjectWizardDialog *wizard = qobject_cast<const QmlNewProjectWizardDialog *>(w);
    const QString projectName = wizard->projectName();
    const QString projectPath = wizard->path() + QLatin1Char('/') + projectName;

    const QString creatorFileName = Core::BaseFileWizard::buildFileName(projectPath,
                                                                        projectName,
                                                                        QLatin1String("qmlproject"));

    const QString mainFileName = Core::BaseFileWizard::buildFileName(projectPath,
                                                                     projectName,
                                                                     QLatin1String("qml"));

    QString contents;
    QTextStream out(&contents);

    out
        << "import Qt 4.6" << endl
        << endl
        << "Rectangle {" << endl
        << "    width: 200" << endl
        << "    height: 200" << endl
        << "    color: \"white\"" << endl
        << "    Text {" << endl
        << "        text: \"Hello World\"" << endl
        << "        anchors.centerIn: parent" << endl
        << "    }" << endl
        << "}" << endl;

    Core::GeneratedFile generatedMainFile(mainFileName);
    generatedMainFile.setContents(contents);

    Core::GeneratedFile generatedCreatorFile(creatorFileName);
    generatedCreatorFile.setContents(projectName + QLatin1String(".qml\n"));

    Core::GeneratedFiles files;
    files.append(generatedMainFile);
    files.append(generatedCreatorFile);

    return files;
}

bool QmlNewProjectWizard::postGenerateFiles(const Core::GeneratedFiles &l, QString *errorMessage)
{
    // Post-Generate: Open the project
    const QString proFileName = l.back().path();
    if (!ProjectExplorer::ProjectExplorerPlugin::instance()->openProject(proFileName)) {
        *errorMessage = tr("The project %1 could not be opened.").arg(proFileName);
        return false;
    }
    return true;
}

