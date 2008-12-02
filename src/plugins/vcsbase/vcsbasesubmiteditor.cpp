/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.2, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/
#include "vcsbasesubmiteditor.h"
#include "submiteditorfile.h"

#include <coreplugin/ifile.h>
#include <coreplugin/icore.h>
#include <coreplugin/uniqueidmanager.h>
#include <coreplugin/actionmanager/actionmanagerinterface.h>

#include <utils/submiteditorwidget.h>
#include <find/basetextfind.h>

#include <QtGui/QToolBar>
#include <QtGui/QStyle>
#include <QtCore/QPointer>
#include <QtCore/QFileInfo>
#include <QtCore/QFile>
#include <QtCore/QTextStream>
#include <QtCore/QDebug>

enum { debug = 0 };

static inline QAction *actionFromId(const Core::ICore *core, const char *id)
{
    QAction *rc = 0;
    if (id)
       if (const Core::ICommand *cmd = core->actionManager()->command(id))
            rc = cmd->action();
    if (debug)
        qDebug() << Q_FUNC_INFO << id << rc;
    return rc;
}

namespace VCSBase {

struct VCSBaseSubmitEditorPrivate {
    VCSBaseSubmitEditorPrivate(const VCSBaseSubmitEditorParameters *parameters,
                               Core::Utils::SubmitEditorWidget *editorWidget,
                               QObject *q);

    Core::ICore *m_core;
    Core::Utils::SubmitEditorWidget *m_widget;
    QToolBar *m_toolWidget;
    const VCSBaseSubmitEditorParameters *m_parameters;
    QString m_displayName;
    VCSBase::Internal::SubmitEditorFile *m_file;
    QList<int> m_contexts;

    QPointer<QAction> m_undoAction;
    QPointer<QAction> m_redoAction;
    QPointer<QAction> m_submitAction;
    QPointer<QAction> m_diffAction;
};

VCSBaseSubmitEditorPrivate::VCSBaseSubmitEditorPrivate(const VCSBaseSubmitEditorParameters *parameters,
                                                       Core::Utils::SubmitEditorWidget *editorWidget,
                                                       QObject *q) :
    m_core(ExtensionSystem::PluginManager::instance()->getObject<Core::ICore>()),
    m_widget(editorWidget),
    m_toolWidget(0),
    m_parameters(parameters),
    m_file(new VCSBase::Internal::SubmitEditorFile(QLatin1String(m_parameters->mimeType), q)),
    m_undoAction(actionFromId(m_core, m_parameters->undoActionId)),
    m_redoAction(actionFromId(m_core, m_parameters->redoActionId)),
    m_submitAction(actionFromId(m_core, m_parameters->submitActionId)),
    m_diffAction(actionFromId(m_core, m_parameters->diffActionId))
{
    m_contexts << m_core->uniqueIDManager()->uniqueIdentifier(m_parameters->context);
}

VCSBaseSubmitEditor::VCSBaseSubmitEditor(const VCSBaseSubmitEditorParameters *parameters,
                                         Core::Utils::SubmitEditorWidget *editorWidget) :
    m_d(new VCSBaseSubmitEditorPrivate(parameters, editorWidget, this))
{
    m_d->m_file->setModified(false);
    // We are always clean to prevent the editor manager from asking to save.
    connect(m_d->m_file, SIGNAL(saveMe(QString)), this, SLOT(save(QString)));

    m_d->m_widget->registerActions(m_d->m_undoAction, m_d->m_redoAction,  m_d->m_submitAction, m_d->m_diffAction);
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

QToolBar *VCSBaseSubmitEditor::toolBar()
{
    if (m_d->m_toolWidget)
        return m_d->m_toolWidget;

    if (!m_d->m_diffAction && !m_d->m_submitAction)
        return 0;

    // Create
    QToolBar *toolBar = new QToolBar;
    toolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    const int size = m_d->m_widget->style()->pixelMetric(QStyle::PM_SmallIconSize);
    toolBar->setIconSize(QSize(size, size));
    toolBar->addSeparator();

    if (m_d->m_submitAction)
        toolBar->addAction(m_d->m_submitAction);
    if (m_d->m_diffAction)
        toolBar->addAction(m_d->m_diffAction);
    m_d->m_toolWidget = toolBar;
    return toolBar;
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

QStringList  VCSBaseSubmitEditor::checkedFiles() const
{
    return vcsFileListToFileList(m_d->m_widget->checkedFiles());
}

void VCSBaseSubmitEditor::setFileList(const QStringList &l)
{
    m_d->m_widget->setFileList(l);
}

void VCSBaseSubmitEditor::addFiles(const QStringList& list, bool checked, bool userCheckable)
{
     m_d->m_widget->addFiles(list, checked, userCheckable);
}

void  VCSBaseSubmitEditor::slotDiffSelectedVCSFiles(const QStringList &rawList)
{
     emit diffSelectedFiles(vcsFileListToFileList(rawList));
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

}
