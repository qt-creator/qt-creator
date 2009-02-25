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

#include "resourceeditorw.h"
#include "resourceeditorplugin.h"
#include "resourceeditorconstants.h"

#include <qrceditor.h>

#include <coreplugin/icore.h>
#include <utils/reloadpromptutils.h>

#include <QtCore/QTemporaryFile>
#include <QtCore/QFileInfo>
#include <QtCore/QDir>
#include <QtCore/qdebug.h>
#include <QtGui/QMainWindow>
#include <QtGui/QHBoxLayout>

namespace ResourceEditor {
namespace Internal {

enum { debugResourceEditorW = 0 };



ResourceEditorFile::ResourceEditorFile(ResourceEditorW *parent) :
    IFile(parent),
    m_mimeType(QLatin1String(ResourceEditor::Constants::C_RESOURCE_MIMETYPE)),
    m_parent(parent)
{
    if (debugResourceEditorW)
        qDebug() <<  "ResourceEditorFile::ResourceEditorFile()";
}

QString ResourceEditorFile::mimeType() const
{
    return m_mimeType;
}


ResourceEditorW::ResourceEditorW(const QList<int> &context,
                               ResourceEditorPlugin *plugin,
                               QWidget *parent)
        : m_context(context),
        m_resourceEditor(new SharedTools::QrcEditor(parent)),
        m_resourceFile(new ResourceEditorFile(this)),
        m_plugin(plugin)
{
    m_resourceEditor->setResourceDragEnabled(true);
    m_resourceEditor->layout()->setMargin(9);

    connect(m_resourceEditor, SIGNAL(dirtyChanged(bool)), this, SLOT(dirtyChanged(bool)));
    connect(m_resourceEditor, SIGNAL(undoStackChanged(bool, bool)),
            this, SLOT(onUndoStackChanged(bool, bool)));
    connect(m_resourceFile, SIGNAL(changed()), this, SIGNAL(changed()));
    if (debugResourceEditorW)
        qDebug() <<  "ResourceEditorW::ResourceEditorW()";
}

ResourceEditorW::~ResourceEditorW()
{
    if (m_resourceEditor)
        m_resourceEditor->deleteLater();
}

bool ResourceEditorW::createNew(const QString &contents)
{
    QTemporaryFile tempFile(0);
    tempFile.setAutoRemove(true);
    if (!tempFile.open())
        return false;
    const QString tempFileName =  tempFile.fileName();
    tempFile.write(contents.toUtf8());
    tempFile.close();

    const bool rc = m_resourceEditor->load(tempFileName);
    m_resourceEditor->setFileName(QString());
    if (debugResourceEditorW)
        qDebug() <<  "ResourceEditorW::createNew: " << contents << " (" << tempFileName << ") returns " << rc;
    return rc;
}

bool ResourceEditorW::open(const QString &fileName /*= QString()*/)
{
    if (debugResourceEditorW)
        qDebug() <<  "ResourceEditorW::open: " << fileName;

    if (fileName.isEmpty()) {
        setDisplayName(tr("untitled"));
        return true;
    }

    const QFileInfo fi(fileName);

    const QString absFileName = fi.absoluteFilePath();

    if (!fi.isReadable())
        return false;

    if (!m_resourceEditor->load(absFileName))
        return false;

    setDisplayName(fi.fileName());

    emit changed();
    return true;
}

bool ResourceEditorFile::save(const QString &name /*= QString()*/)
{
    if (debugResourceEditorW)
        qDebug() <<  ">ResourceEditorW::save: " << name;

    const QString oldFileName = fileName();
    const QString actualName = name.isEmpty() ? oldFileName : name;
    if (actualName.isEmpty())
        return false;

    m_parent->m_resourceEditor->setFileName(actualName);
    if (!m_parent->m_resourceEditor->save()) {
        m_parent->m_resourceEditor->setFileName(oldFileName);
        return false;
    }

    m_parent->m_resourceEditor->setDirty(false);
    m_parent->setDisplayName(QFileInfo(actualName).fileName());

    emit changed();
    return true;
}

const char *ResourceEditorW::kind() const {
    return ResourceEditor::Constants::C_RESOURCEEDITOR;
}

QString ResourceEditorFile::fileName() const
{
    return m_parent->m_resourceEditor->fileName();
}

bool ResourceEditorFile::isModified() const
{
    return m_parent->m_resourceEditor->isDirty();
}

bool ResourceEditorFile::isReadOnly() const
{
    const QString fileName = m_parent->m_resourceEditor->fileName();
    if (fileName.isEmpty())
        return false;
    const QFileInfo fi(fileName);
    return !fi.isWritable();
}

bool ResourceEditorFile::isSaveAsAllowed() const
{
    return true;
}

void ResourceEditorFile::modified(Core::IFile::ReloadBehavior *behavior)
{
    const QString fileName = m_parent->m_resourceEditor->fileName();

    switch (*behavior) {
    case  Core::IFile::ReloadNone:
        return;
    case Core::IFile::ReloadAll:
        m_parent->open(fileName);
        return;
    case Core::IFile::ReloadPermissions:
        emit changed();
        return;
    case Core::IFile::AskForReload:
        break;
    }

    switch (Core::Utils::reloadPrompt(fileName, Core::ICore::instance()->mainWindow())) {
    case Core::Utils::ReloadCurrent:
        m_parent->open(fileName);
        break;
    case Core::Utils::ReloadAll:
        m_parent->open(fileName);
        *behavior = Core::IFile::ReloadAll;
        break;
    case Core::Utils::ReloadSkipCurrent:
        break;
    case Core::Utils::ReloadNone:
        *behavior = Core::IFile::ReloadNone;
        break;
    }
}

QString ResourceEditorFile::defaultPath() const
{
    return QString();
}

void ResourceEditorW::setSuggestedFileName(const QString &fileName)
{
    m_suggestedName = fileName;
}

QString ResourceEditorFile::suggestedFileName() const
{
    return m_parent->m_suggestedName;
}

void ResourceEditorW::dirtyChanged(bool dirty)
{
    if (debugResourceEditorW)
        qDebug() << " ResourceEditorW::dirtyChanged" <<  dirty;
    if (dirty)
        emit changed();
}

QWidget *ResourceEditorW::widget()
{
    return m_resourceEditor; /* we know it's a subclass of QWidget...*/
}

void ResourceEditorW::onUndoStackChanged(bool canUndo, bool canRedo)
{
    m_plugin->onUndoStackChanged(this, canUndo, canRedo);
}

void ResourceEditorW::onUndo()
{
    if (!m_resourceEditor.isNull())
        m_resourceEditor.data()->onUndo();
}

void ResourceEditorW::onRedo()
{
    if (!m_resourceEditor.isNull())
        m_resourceEditor.data()->onRedo();
}

} // namespace Internal
} // namespace ResourceEditor
