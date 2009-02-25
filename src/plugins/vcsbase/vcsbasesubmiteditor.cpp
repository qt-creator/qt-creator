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

#include "vcsbasesubmiteditor.h"
#include "submiteditorfile.h"

#include <coreplugin/ifile.h>
#include <coreplugin/uniqueidmanager.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <extensionsystem/pluginmanager.h>
#include <utils/submiteditorwidget.h>
#include <find/basetextfind.h>

#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/session.h>

#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QPointer>
#include <QtCore/QTextStream>
#include <QtGui/QStyle>
#include <QtGui/QToolBar>

enum { debug = 0 };
enum { wantToolBar = 0 };

namespace VCSBase {

struct VCSBaseSubmitEditorPrivate
{
    VCSBaseSubmitEditorPrivate(const VCSBaseSubmitEditorParameters *parameters,
                               Core::Utils::SubmitEditorWidget *editorWidget,
                               QObject *q);

    Core::Utils::SubmitEditorWidget *m_widget;
    QToolBar *m_toolWidget;
    const VCSBaseSubmitEditorParameters *m_parameters;
    QString m_displayName;
    VCSBase::Internal::SubmitEditorFile *m_file;
    QList<int> m_contexts;

    QPointer<QAction> m_diffAction;
    QPointer<QAction> m_submitAction;
};

VCSBaseSubmitEditorPrivate::VCSBaseSubmitEditorPrivate(const VCSBaseSubmitEditorParameters *parameters,
                                                       Core::Utils::SubmitEditorWidget *editorWidget,
                                                       QObject *q) :
    m_widget(editorWidget),
    m_toolWidget(0),
    m_parameters(parameters),
    m_file(new VCSBase::Internal::SubmitEditorFile(QLatin1String(m_parameters->mimeType), q))
{
    m_contexts << Core::UniqueIDManager::instance()->uniqueIdentifier(m_parameters->context);
}

VCSBaseSubmitEditor::VCSBaseSubmitEditor(const VCSBaseSubmitEditorParameters *parameters,
                                         Core::Utils::SubmitEditorWidget *editorWidget) :
    m_d(new VCSBaseSubmitEditorPrivate(parameters, editorWidget, this))
{
    m_d->m_file->setModified(false);
    // We are always clean to prevent the editor manager from asking to save.
    connect(m_d->m_file, SIGNAL(saveMe(QString)), this, SLOT(save(QString)));

    connect(m_d->m_widget, SIGNAL(diffSelected(QStringList)), this, SLOT(slotDiffSelectedVCSFiles(QStringList)));
    connect(m_d->m_widget->descriptionEdit(), SIGNAL(textChanged()), this, SLOT(slotDescriptionChanged()));

    Aggregation::Aggregate *aggregate = new Aggregation::Aggregate;
    aggregate->add(new Find::BaseTextFind(m_d->m_widget->descriptionEdit()));
    aggregate->add(this);
}

VCSBaseSubmitEditor::~VCSBaseSubmitEditor()
{
    delete m_d->m_toolWidget;
    delete m_d->m_widget;
    delete m_d;
}

void VCSBaseSubmitEditor::registerActions(QAction *editorUndoAction,  QAction *editorRedoAction,
                                          QAction *submitAction, QAction *diffAction)\
{
    m_d->m_widget->registerActions(editorUndoAction, editorRedoAction, submitAction, diffAction);
    m_d->m_diffAction = diffAction;
    m_d->m_submitAction = submitAction;
}

void VCSBaseSubmitEditor::unregisterActions(QAction *editorUndoAction,  QAction *editorRedoAction,
                           QAction *submitAction, QAction *diffAction)
{
    m_d->m_widget->unregisterActions(editorUndoAction, editorRedoAction, submitAction, diffAction);
    m_d->m_diffAction = m_d->m_submitAction = 0;
}
int VCSBaseSubmitEditor::fileNameColumn() const
{
    return m_d->m_widget->fileNameColumn();
}

void VCSBaseSubmitEditor::setFileNameColumn(int c)
{
    m_d->m_widget->setFileNameColumn(c);
}

QAbstractItemView::SelectionMode VCSBaseSubmitEditor::fileListSelectionMode() const
{
    return m_d->m_widget->fileListSelectionMode();
}

void VCSBaseSubmitEditor::setFileListSelectionMode(QAbstractItemView::SelectionMode sm)
{
    m_d->m_widget->setFileListSelectionMode(sm);
}


void VCSBaseSubmitEditor::slotDescriptionChanged()
{
}

bool VCSBaseSubmitEditor::createNew(const QString &contents)
{
    setFileContents(contents);
    return true;
}

bool VCSBaseSubmitEditor::open(const QString &fileName)
{
    if (fileName.isEmpty())
        return false;

    const QFileInfo fi(fileName);
    if (!fi.isFile() || !fi.isReadable())
        return false;

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly|QIODevice::Text)) {
        qWarning("Unable to open %s: %s", qPrintable(fileName), qPrintable(file.errorString()));
        return false;
    }

    const QString text = QString::fromLocal8Bit(file.readAll());
    if (!createNew(text))
        return false;

    m_d->m_file->setFileName(fi.absoluteFilePath());
    return true;
}

Core::IFile *VCSBaseSubmitEditor::file()
{
    return m_d->m_file;
}

QString VCSBaseSubmitEditor::displayName() const
{
    return m_d->m_displayName;
}

void VCSBaseSubmitEditor::setDisplayName(const QString &title)
{
    m_d->m_displayName = title;
}

bool VCSBaseSubmitEditor::duplicateSupported() const
{
    return false;
}

Core::IEditor *VCSBaseSubmitEditor::duplicate(QWidget * /*parent*/)
{
    return 0;
}

const char *VCSBaseSubmitEditor::kind() const
{
    return m_d->m_parameters->kind;
}

static QToolBar *createToolBar(const QWidget *someWidget, QAction *submitAction, QAction *diffAction)
{
    // Create
    QToolBar *toolBar = new QToolBar;
    toolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    const int size = someWidget->style()->pixelMetric(QStyle::PM_SmallIconSize);
    toolBar->setIconSize(QSize(size, size));
    toolBar->addSeparator();

    if (submitAction)
        toolBar->addAction(submitAction);
    if (diffAction)
        toolBar->addAction(diffAction);
    return toolBar;
}

QToolBar *VCSBaseSubmitEditor::toolBar()
{
    if (!wantToolBar)
        return 0;

    if (m_d->m_toolWidget)
        return m_d->m_toolWidget;

    if (!m_d->m_diffAction && !m_d->m_submitAction)
        return 0;

    // Create
    m_d->m_toolWidget = createToolBar(m_d->m_widget, m_d->m_submitAction, m_d->m_diffAction);
    return m_d->m_toolWidget;
}

QList<int> VCSBaseSubmitEditor::context() const
{
    return m_d->m_contexts;
}

QWidget *VCSBaseSubmitEditor::widget()
{
    return m_d->m_widget;
}

QByteArray VCSBaseSubmitEditor::saveState() const
{
    return QByteArray();
}

bool VCSBaseSubmitEditor::restoreState(const QByteArray &/*state*/)
{
    return true;
}

QStringList VCSBaseSubmitEditor::checkedFiles() const
{
    return m_d->m_widget->checkedFiles();
}

void VCSBaseSubmitEditor::setFileModel(QAbstractItemModel *m)
{
    m_d->m_widget->setFileModel(m);
}

QAbstractItemModel *VCSBaseSubmitEditor::fileModel() const
{
    return m_d->m_widget->fileModel();
}

void VCSBaseSubmitEditor::slotDiffSelectedVCSFiles(const QStringList &rawList)
{
     emit diffSelectedFiles(rawList);
}

bool VCSBaseSubmitEditor::save(const QString &fileName)
{
    const QString fName = fileName.isEmpty() ? m_d->m_file->fileName() : fileName;
    QFile file(fName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        qWarning("Unable to open %s: %s", qPrintable(fName), qPrintable(file.errorString()));
        return false;
    }
    file.write(fileContents().toLocal8Bit());
    if (!file.flush())
        return false;
    file.close();
    const QFileInfo fi(fName);
    m_d->m_file->setFileName(fi.absoluteFilePath());
    m_d->m_file->setModified(false);
    return true;
}

QString VCSBaseSubmitEditor::fileContents() const
{
    return m_d->m_widget->trimmedDescriptionText();
}

bool VCSBaseSubmitEditor::setFileContents(const QString &contents)
{
    m_d->m_widget->setDescriptionText(contents);
    return true;
}

QIcon VCSBaseSubmitEditor::diffIcon()
{
    return QIcon(QLatin1String(":/vcsbase/images/diff.png"));
}

QIcon VCSBaseSubmitEditor::submitIcon()
{
    return QIcon(QLatin1String(":/vcsbase/images/submit.png"));
}

QStringList VCSBaseSubmitEditor::currentProjectFiles(bool nativeSeparators, QString *name)
{
    if (name)
        name->clear();
    ProjectExplorer::ProjectExplorerPlugin *projectExplorer = ExtensionSystem::PluginManager::instance()->getObject<ProjectExplorer::ProjectExplorerPlugin>();
    if (!projectExplorer)
        return QStringList();
    QStringList files;
    if (const ProjectExplorer::Project *currentProject = projectExplorer->currentProject()) {
        files << currentProject->files(ProjectExplorer::Project::ExcludeGeneratedFiles);
        if (name)
            *name = currentProject->name();
    } else {
        if (const ProjectExplorer::SessionManager *session = projectExplorer->session()) {
            if (name)
                *name = session->file()->fileName();
        const QList<ProjectExplorer::Project *> projects = session->projects();
        foreach (ProjectExplorer::Project *project, projects)
            files << project->files(ProjectExplorer::Project::ExcludeGeneratedFiles);
        }
    }
    if (nativeSeparators && !files.empty()) {
        const QStringList::iterator end = files.end();
        for (QStringList::iterator it = files.begin(); it != end; ++it)
            *it = QDir::toNativeSeparators(*it);
    }
    return files;
}
} // namespace VCSBase
