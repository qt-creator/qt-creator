/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "formwindowfile.h"
#include "designerconstants.h"
#include "resourcehandler.h"

#include <utils/fileutils.h>
#include <utils/qtcassert.h>

#include <QApplication>
#include <QBuffer>
#include <QDesignerFormWindowInterface>
#include <QDesignerFormWindowManagerInterface>
#include <QDesignerFormEditorInterface>
#include <QTextDocument>
#include <QUndoStack>
#include <QFileInfo>
#include <QDebug>
#include <QTextCodec>

using namespace Utils;

namespace Designer {
namespace Internal {

FormWindowFile::FormWindowFile(QDesignerFormWindowInterface *form, QObject *parent)
  : m_formWindow(form)
{
    setMimeType(QLatin1String(Designer::Constants::FORM_MIMETYPE));
    setParent(parent);
    setId(Core::Id(Designer::Constants::K_DESIGNER_XML_EDITOR_ID));
    // Designer needs UTF-8 regardless of settings.
    setCodec(QTextCodec::codecForName("UTF-8"));
    connect(m_formWindow->core()->formWindowManager(), &QDesignerFormWindowManagerInterface::formWindowRemoved,
            this, &FormWindowFile::slotFormWindowRemoved);
    connect(m_formWindow->commandHistory(), &QUndoStack::indexChanged,
            this, &FormWindowFile::setShouldAutoSave);
    connect(m_formWindow.data(), &QDesignerFormWindowInterface::changed, this, &FormWindowFile::updateIsModified);

    m_resourceHandler = new ResourceHandler(form);
    connect(this, &FormWindowFile::filePathChanged,
            m_resourceHandler, &ResourceHandler::updateResources);
}

Core::IDocument::OpenResult FormWindowFile::open(QString *errorString, const QString &fileName,
                                                 const QString &realFileName)
{
    if (Designer::Constants::Internal::debug)
        qDebug() << "FormWindowFile::open" << fileName;

    QDesignerFormWindowInterface *form = formWindow();
    QTC_ASSERT(form, return OpenResult::CannotHandle);

    if (fileName.isEmpty())
        return OpenResult::ReadError;

    const QFileInfo fi(fileName);
    const QString absfileName = fi.absoluteFilePath();

    QString contents;
    Utils::TextFileFormat::ReadResult readResult = read(absfileName, &contents, errorString);
    if (readResult == Utils::TextFileFormat::ReadEncodingError)
        return OpenResult::CannotHandle;
    else if (readResult != Utils::TextFileFormat::ReadSuccess)
        return OpenResult::ReadError;

    form->setFileName(absfileName);
    const QByteArray contentsBA = contents.toUtf8();
    QBuffer str;
    str.setData(contentsBA);
    str.open(QIODevice::ReadOnly);
    if (!form->setContents(&str, errorString))
        return OpenResult::CannotHandle;
    form->setDirty(fileName != realFileName);

    syncXmlFromFormWindow();
    setFilePath(Utils::FileName::fromString(absfileName));
    setShouldAutoSave(false);
    resourceHandler()->updateProjectResources();

    return OpenResult::Success;
}

bool FormWindowFile::save(QString *errorString, const QString &name, bool autoSave)
{
    const FileName actualName = name.isEmpty() ? filePath() : FileName::fromString(name);

    if (Designer::Constants::Internal::debug)
        qDebug() << Q_FUNC_INFO << name << "->" << actualName;

    QTC_ASSERT(m_formWindow, return false);

    if (actualName.isEmpty())
        return false;

    const QString oldFormName = m_formWindow->fileName();
    if (!autoSave)
        m_formWindow->setFileName(actualName.toString());
    const bool writeOK = writeFile(actualName.toString(), errorString);
    m_shouldAutoSave = false;
    if (autoSave)
        return writeOK;

    if (!writeOK) {
        m_formWindow->setFileName(oldFormName);
        return false;
    }

    m_formWindow->setDirty(false);
    setFilePath(actualName);
    updateIsModified();

    return true;
}

QByteArray FormWindowFile::contents() const
{
    return formWindowContents().toUtf8();
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

void FormWindowFile::setFilePath(const FileName &newName)
{
    m_formWindow->setFileName(newName.toString());
    IDocument::setFilePath(newName);
}

void FormWindowFile::updateIsModified()
{
    if (m_modificationChangedGuard.isLocked())
        return;

    bool value = m_formWindow && m_formWindow->isDirty();
    if (value)
        emit contentsChanged();
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
    if (flag == FlagIgnore) {
        if (!m_formWindow || type != TypeContents)
            return true;
        const bool wasModified = m_formWindow->isDirty();
        {
            Utils::GuardLocker locker(m_modificationChangedGuard);
            // hack to ensure we clean the clear state in form window
            m_formWindow->setDirty(false);
            m_formWindow->setDirty(true);
        }
        if (!wasModified)
            updateIsModified();
        return true;
    }
    if (type == TypePermissions) {
        emit changed();
    } else {
        emit aboutToReload();
        const bool success
                = (open(errorString, filePath().toString(), filePath().toString()) == OpenResult::Success);
        emit reloadFinished(success);
        return success;
    }
    return true;
}

void FormWindowFile::setFallbackSaveAsFileName(const QString &fn)
{
    if (Designer::Constants::Internal::debug)
        qDebug() << Q_FUNC_INFO << filePath() << fn;

    m_suggestedName = fn;
}

QString FormWindowFile::fallbackSaveAsFileName() const
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
