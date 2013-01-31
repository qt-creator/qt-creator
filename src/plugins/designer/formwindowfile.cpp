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

#include "formwindowfile.h"
#include "designerconstants.h"

#include <coreplugin/icore.h>
#include <utils/reloadpromptutils.h>
#include <utils/fileutils.h>
#include <utils/qtcassert.h>

#include <QDesignerFormWindowInterface>
#include <QDesignerFormWindowManagerInterface>
#include <QDesignerFormEditorInterface>
#if QT_VERSION < 0x050000
#    include "qt_private/qsimpleresource_p.h"
#endif

#include <QMessageBox>
#include <QUndoStack>

#include <QFile>
#include <QFileInfo>
#include <QByteArray>
#include <QDebug>
#include <QTextCodec>

namespace Designer {
namespace Internal {

FormWindowFile::FormWindowFile(QDesignerFormWindowInterface *form, QObject *parent)
  : Core::TextDocument(parent),
    m_mimeType(QLatin1String(Designer::Constants::FORM_MIMETYPE)),
    m_shouldAutoSave(false),
    m_formWindow(form)
{
    // Designer needs UTF-8 regardless of settings.
    setCodec(QTextCodec::codecForName("UTF-8"));
    connect(m_formWindow->core()->formWindowManager(), SIGNAL(formWindowRemoved(QDesignerFormWindowInterface*)),
            this, SLOT(slotFormWindowRemoved(QDesignerFormWindowInterface*)));
    connect(m_formWindow->commandHistory(), SIGNAL(indexChanged(int)),
            this, SLOT(setShouldAutoSave()));
}

bool FormWindowFile::save(QString *errorString, const QString &name, bool autoSave)
{
    const QString actualName = name.isEmpty() ? fileName() : name;

    if (Designer::Constants::Internal::debug)
        qDebug() << Q_FUNC_INFO << name << "->" << actualName;

    QTC_ASSERT(m_formWindow, return false);

    if (actualName.isEmpty())
        return false;

    const QFileInfo fi(actualName);
    const QString oldFormName = m_formWindow->fileName();
    if (!autoSave)
        m_formWindow->setFileName(fi.absoluteFilePath());
#if QT_VERSION >= 0x050000
    const bool writeOK = writeFile(actualName, errorString);
#else
    const bool warningsEnabled = qdesigner_internal::QSimpleResource::setWarningsEnabled(false);
    const bool writeOK = writeFile(actualName, errorString);
    qdesigner_internal::QSimpleResource::setWarningsEnabled(warningsEnabled);
#endif
    m_shouldAutoSave = false;
    if (autoSave)
        return writeOK;

    if (!writeOK) {
        m_formWindow->setFileName(oldFormName);
        return false;
    }

    const QString oldFileName = m_fileName;
    m_fileName = fi.absoluteFilePath();
    emit setDisplayName(fi.fileName());
    m_formWindow->setDirty(false);
    emit fileNameChanged(oldFileName, m_fileName);
    emit changed();
    emit saved();

    return true;
}

void FormWindowFile::rename(const QString &newName)
{
    m_formWindow->setFileName(newName);
    QFileInfo fi(newName);
    const QString oldFileName = m_fileName;
    m_fileName = fi.absoluteFilePath();
    emit setDisplayName(fi.fileName());
    emit fileNameChanged(oldFileName, m_fileName);
    emit changed();
}

QString FormWindowFile::fileName() const
{
    return m_fileName;
}

bool FormWindowFile::shouldAutoSave() const
{
    return m_shouldAutoSave;
}

bool FormWindowFile::isModified() const
{
    return m_formWindow && m_formWindow->isDirty();
}

bool FormWindowFile::isSaveAsAllowed() const
{
    return true;
}

bool FormWindowFile::reload(QString *errorString, ReloadFlag flag, ChangeType type)
{
    if (flag == FlagIgnore)
        return true;
    if (type == TypePermissions) {
        emit changed();
    } else {
        emit aboutToReload();
        emit reload(errorString, m_fileName);
        if (!errorString->isEmpty())
            return false;
        emit reloaded();
    }
    return true;
}

QString FormWindowFile::defaultPath() const
{
    return QString();
}

void FormWindowFile::setSuggestedFileName(const QString &fileName)
{
    if (Designer::Constants::Internal::debug)
        qDebug() << Q_FUNC_INFO << m_fileName << fileName;

    m_suggestedName = fileName;
}

QString FormWindowFile::suggestedFileName() const
{
    return m_suggestedName;
}

QString FormWindowFile::mimeType() const
{
    return m_mimeType;
}

bool FormWindowFile::writeFile(const QString &fileName, QString *errorString) const
{
    if (Designer::Constants::Internal::debug)
        qDebug() << Q_FUNC_INFO << m_fileName << fileName;
    return write(fileName, format(), m_formWindow->contents(), errorString);
}

void FormWindowFile::setFileName(const QString &fname)
{
    m_fileName = fname;
}

QDesignerFormWindowInterface *FormWindowFile::formWindow() const
{
    return m_formWindow;
}

void FormWindowFile::slotFormWindowRemoved(QDesignerFormWindowInterface *w)
{
    // Release formwindow as soon as the FormWindowManager removes
    // as calls to isDirty() are triggered at arbitrary times
    // while building.
    if (w == m_formWindow)
        m_formWindow = 0;
}

} // namespace Internal
} // namespace Designer
