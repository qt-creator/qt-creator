/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "idocument.h"

#include "infobar.h"

#include <QFile>
#include <QFileInfo>

/*!
    \fn QString Core::IDocument::filePath() const
    Returns the absolute path of the file that this document refers to. May be empty for
    non-file documents.
    \sa setFilePath()
*/

namespace Core {

IDocument::IDocument(QObject *parent) : QObject(parent),
    m_temporary(false),
    m_infoBar(0),
    m_hasWriteWarning(false),
    m_restored(false)
{
}

IDocument::~IDocument()
{
    removeAutoSaveFile();
    delete m_infoBar;
}

/*!
    Used for example by EditorManager::openEditorWithContents() to set the contents
    of this document.
    Returns if setting the contents was successful.
    The base implementation does nothing and returns false.
*/
bool IDocument::setContents(const QByteArray &contents)
{
    Q_UNUSED(contents)
    return false;
}

IDocument::ReloadBehavior IDocument::reloadBehavior(ChangeTrigger state, ChangeType type) const
{
    if (type == TypePermissions)
        return BehaviorSilent;
    if (type == TypeContents && state == TriggerInternal && !isModified())
        return BehaviorSilent;
    return BehaviorAsk;
}

void IDocument::checkPermissions()
{
}

bool IDocument::shouldAutoSave() const
{
    return false;
}

bool IDocument::isFileReadOnly() const
{
    if (filePath().isEmpty())
        return false;
    return !QFileInfo(filePath()).isWritable();
}

/*!
    Returns if the document is a temporary that should for example not be considered
    when saving/restoring the session state, recent files, etc. Defaults to false.
    \sa setTemporary()
*/
bool IDocument::isTemporary() const
{
    return m_temporary;
}

/*!
    Sets if the document is \a temporary.
    \sa isTemporary()
*/
void IDocument::setTemporary(bool temporary)
{
    m_temporary = temporary;
}

bool IDocument::autoSave(QString *errorString, const QString &fileName)
{
    if (!save(errorString, fileName, true))
        return false;
    m_autoSaveName = fileName;
    return true;
}

static const char kRestoredAutoSave[] = "RestoredAutoSave";

void IDocument::setRestoredFrom(const QString &name)
{
    m_autoSaveName = name;
    m_restored = true;
    InfoBarEntry info(Id(kRestoredAutoSave),
          tr("File was restored from auto-saved copy. "
             "Select Save to confirm or Revert to Saved to discard changes."));
    infoBar()->addInfo(info);
}

void IDocument::removeAutoSaveFile()
{
    if (!m_autoSaveName.isEmpty()) {
        QFile::remove(m_autoSaveName);
        m_autoSaveName.clear();
        if (m_restored) {
            m_restored = false;
            infoBar()->removeInfo(Id(kRestoredAutoSave));
        }
    }
}

InfoBar *IDocument::infoBar()
{
    if (!m_infoBar)
        m_infoBar = new InfoBar;
    return m_infoBar;
}

/*!
    Set absolute file path for this file to \a filePath. Can be empty.
    The default implementation sets the file name and sends filePathChanged() and changed()
    signals. Can be reimplemented by subclasses to do more.
    \sa filePath()
*/
void IDocument::setFilePath(const QString &filePath)
{
    if (m_filePath == filePath)
        return;
    QString oldName = m_filePath;
    m_filePath = filePath;
    emit filePathChanged(oldName, m_filePath);
    emit changed();
}

/*!
    Returns the string to display for this document, e.g. in the open document combo box
    and pane.
    \sa setDisplayName()
*/
QString IDocument::displayName() const
{
    if (!m_displayName.isEmpty())
        return m_displayName;
    return QFileInfo(m_filePath).fileName();
}

/*!
    Sets the string that is displayed for this document, e.g. in the open document combo box
    and pane, to \a name. Defaults to the file name of the file path for this document.
    You can reset the display name to the default by passing an empty string.
    \sa displayName()
    \sa filePath()
 */
void IDocument::setDisplayName(const QString &name)
{
    if (name == m_displayName)
        return;
    m_displayName = name;
    emit changed();
}

} // namespace Core
