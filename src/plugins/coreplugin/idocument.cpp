/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

namespace Core {

IDocument::IDocument(QObject *parent) : QObject(parent), m_infoBar(0), m_hasWriteWarning(false), m_restored(false)
{
}

IDocument::~IDocument()
{
    removeAutoSaveFile();
    delete m_infoBar;
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
    if (fileName().isEmpty())
        return false;
    return !QFileInfo(fileName()).isWritable();
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

} // namespace Core
