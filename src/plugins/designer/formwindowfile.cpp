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

#include "formwindowfile.h"
#include "designerconstants.h"
#include "resourcehandler.h"

#include <utils/qtcassert.h>

#include <QApplication>
#include <QDesignerFormWindowInterface>
#include <QDesignerFormWindowManagerInterface>
#include <QDesignerFormEditorInterface>
#include <QTextDocument>
#include <QUndoStack>
#include <QFileInfo>
#include <QDebug>
#include <QTextCodec>

namespace Designer {
namespace Internal {

FormWindowFile::FormWindowFile(QDesignerFormWindowInterface *form, QObject *parent)
  : m_shouldAutoSave(false),
    m_formWindow(form),
    m_isModified(false)
{
    setMimeType(QLatin1String(Designer::Constants::FORM_MIMETYPE));
    setParent(parent);
    setId(Core::Id(Designer::Constants::K_DESIGNER_XML_EDITOR_ID));
    // Designer needs UTF-8 regardless of settings.
    setCodec(QTextCodec::codecForName("UTF-8"));
    connect(m_formWindow->core()->formWindowManager(), SIGNAL(formWindowRemoved(QDesignerFormWindowInterface*)),
            this, SLOT(slotFormWindowRemoved(QDesignerFormWindowInterface*)));
    connect(m_formWindow->commandHistory(), SIGNAL(indexChanged(int)),
            this, SLOT(setShouldAutoSave()));
    connect(m_formWindow, SIGNAL(changed()), SLOT(updateIsModified()));

    m_resourceHandler = new ResourceHandler(form);
    connect(this, SIGNAL(filePathChanged(QString,QString)),
            m_resourceHandler, SLOT(updateResources()));
}

bool FormWindowFile::save(QString *errorString, const QString &name, bool autoSave)
{
    const QString actualName = name.isEmpty() ? filePath() : name;

    if (Designer::Constants::Internal::debug)
        qDebug() << Q_FUNC_INFO << name << "->" << actualName;

    QTC_ASSERT(m_formWindow, return false);

    if (actualName.isEmpty())
        return false;

    const QFileInfo fi(actualName);
    const QString oldFormName = m_formWindow->fileName();
    if (!autoSave)
        m_formWindow->setFileName(fi.absoluteFilePath());
    const bool writeOK = writeFile(actualName, errorString);
    m_shouldAutoSave = false;
    if (autoSave)
        return writeOK;

    if (!writeOK) {
        m_formWindow->setFileName(oldFormName);
        return false;
    }

    m_formWindow->setDirty(false);
    setFilePath(fi.absoluteFilePath());
    emit changed();

    return true;
}

bool FormWindowFile::setContents(const QByteArray &contents)
{
    if (Designer::Constants::Internal::debug)
        qDebug() << Q_FUNC_INFO << contents.size();

    document()->clear();

    QTC_ASSERT(m_formWindow, return false);

    if (contents.isEmpty())
        return false;

    // If we have an override cursor, reset it over Designer loading,
    // should it pop up messages about missing resources or such.
    const bool hasOverrideCursor = QApplication::overrideCursor();
    QCursor overrideCursor;
    if (hasOverrideCursor) {
        overrideCursor = QCursor(*QApplication::overrideCursor());
        QApplication::restoreOverrideCursor();
    }

    const bool success = m_formWindow->setContents(QString::fromUtf8(contents));

    if (hasOverrideCursor)
        QApplication::setOverrideCursor(overrideCursor);

    if (!success)
        return false;

    syncXmlFromFormWindow();
    setShouldAutoSave(false);
    return true;
}

void FormWindowFile::setFilePath(const QString &newName)
{
    m_formWindow->setFileName(newName);
    IDocument::setFilePath(newName);
}

void FormWindowFile::updateIsModified()
{
    bool value = m_formWindow && m_formWindow->isDirty();
    if (value == m_isModified)
        return;
    m_isModified = value;
    emit changed();
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
        emit reloadRequested(errorString, filePath());
        const bool success = errorString->isEmpty();
        emit reloadFinished(success);
        return success;
    }
    return true;
}

QString FormWindowFile::defaultPath() const
{
    return QString();
}

void FormWindowFile::setSuggestedFileName(const QString &fn)
{
    if (Designer::Constants::Internal::debug)
        qDebug() << Q_FUNC_INFO << filePath() << fn;

    m_suggestedName = fn;
}

QString FormWindowFile::suggestedFileName() const
{
    return m_suggestedName;
}

bool FormWindowFile::writeFile(const QString &fn, QString *errorString) const
{
    if (Designer::Constants::Internal::debug)
        qDebug() << Q_FUNC_INFO << filePath() << fn;
    return write(fn, format(), m_formWindow->contents(), errorString);
}

QDesignerFormWindowInterface *FormWindowFile::formWindow() const
{
    return m_formWindow;
}

void FormWindowFile::syncXmlFromFormWindow()
{
    document()->setPlainText(formWindowContents());
}

QString FormWindowFile::formWindowContents() const
{
    // TODO: No warnings about spacers here
    QTC_ASSERT(m_formWindow, return QString());
    return m_formWindow->contents();
}

ResourceHandler *FormWindowFile::resourceHandler() const
{
    return m_resourceHandler;
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
