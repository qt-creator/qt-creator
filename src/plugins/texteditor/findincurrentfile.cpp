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

#include "findincurrentfile.h"
#include "texteditor.h"
#include "textdocument.h"

#include <utils/filesearch.h>
#include <utils/fileutils.h>
#include <coreplugin/icore.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/editormanager/editormanager.h>

#include <QSettings>

using namespace TextEditor;
using namespace TextEditor::Internal;

FindInCurrentFile::FindInCurrentFile()
  : m_currentDocument(0)
{
    connect(Core::EditorManager::instance(), &Core::EditorManager::currentEditorChanged,
            this, &FindInCurrentFile::handleFileChange);
    handleFileChange(Core::EditorManager::currentEditor());
}

QString FindInCurrentFile::id() const
{
    return QLatin1String("Current File");
}

QString FindInCurrentFile::displayName() const
{
    return tr("Current File");
}

Utils::FileIterator *FindInCurrentFile::files(const QStringList &nameFilters,
                                              const QVariant &additionalParameters) const
{
    Q_UNUSED(nameFilters)
    QString fileName = additionalParameters.toString();
    QMap<QString, QTextCodec *> openEditorEncodings = TextDocument::openedTextDocumentEncodings();
    QTextCodec *codec = openEditorEncodings.value(fileName);
    if (!codec)
        codec = Core::EditorManager::defaultTextCodec();
    return new Utils::FileListIterator(QStringList(fileName), QList<QTextCodec *>() << codec);
}

QVariant FindInCurrentFile::additionalParameters() const
{
    return qVariantFromValue(m_currentDocument->filePath().toString());
}

QString FindInCurrentFile::label() const
{
    return tr("File \"%1\":").arg(m_currentDocument->filePath().fileName());
}

QString FindInCurrentFile::toolTip() const
{
    // %2 is filled by BaseFileFind::runNewSearch
    return tr("File path: %1\n%2").arg(m_currentDocument->filePath().toUserOutput());
}

bool FindInCurrentFile::isEnabled() const
{
    return m_currentDocument && !m_currentDocument->filePath().isEmpty();
}

void FindInCurrentFile::handleFileChange(Core::IEditor *editor)
{
    if (!editor) {
        if (m_currentDocument) {
            m_currentDocument = 0;
            emit enabledChanged(isEnabled());
        }
    } else {
        Core::IDocument *document = editor->document();
        if (document != m_currentDocument) {
            m_currentDocument = document;
            emit enabledChanged(isEnabled());
        }
    }
}


void FindInCurrentFile::writeSettings(QSettings *settings)
{
    settings->beginGroup(QLatin1String("FindInCurrentFile"));
    writeCommonSettings(settings);
    settings->endGroup();
}

void FindInCurrentFile::readSettings(QSettings *settings)
{
    settings->beginGroup(QLatin1String("FindInCurrentFile"));
    readCommonSettings(settings, QLatin1String("*"));
    settings->endGroup();
}
