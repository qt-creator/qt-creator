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

#include "findinopenfiles.h"
#include "textdocument.h"
#include "texteditor.h"

#include <utils/filesearch.h>
#include <coreplugin/icore.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/documentmodel.h>

#include <QSettings>

using namespace TextEditor;
using namespace TextEditor::Internal;

FindInOpenFiles::FindInOpenFiles()
{
    connect(Core::EditorManager::instance(), &Core::EditorManager::editorOpened,
            this, &FindInOpenFiles::updateEnabledState);
    connect(Core::EditorManager::instance(), &Core::EditorManager::editorsClosed,
            this, &FindInOpenFiles::updateEnabledState);
}

QString FindInOpenFiles::id() const
{
    return QLatin1String("Open Files");
}

QString FindInOpenFiles::displayName() const
{
    return tr("Open Documents");
}

Utils::FileIterator *FindInOpenFiles::files(const QStringList &nameFilters,
                                            const QStringList &exclusionFilters,
                                            const QVariant &additionalParameters) const
{
    Q_UNUSED(nameFilters)
    Q_UNUSED(exclusionFilters)
    Q_UNUSED(additionalParameters)
    QMap<QString, QTextCodec *> openEditorEncodings
            = TextDocument::openedTextDocumentEncodings();
    QStringList fileNames;
    QList<QTextCodec *> codecs;
    foreach (Core::DocumentModel::Entry *entry,
             Core::DocumentModel::entries()) {
        QString fileName = entry->fileName().toString();
        if (!fileName.isEmpty()) {
            fileNames.append(fileName);
            QTextCodec *codec = openEditorEncodings.value(fileName);
            if (!codec)
                codec = Core::EditorManager::defaultTextCodec();
            codecs.append(codec);
        }
    }

    return new Utils::FileListIterator(fileNames, codecs);
}

QVariant FindInOpenFiles::additionalParameters() const
{
    return QVariant();
}

QString FindInOpenFiles::label() const
{
    return tr("Open documents:");
}

QString FindInOpenFiles::toolTip() const
{
    // %1 is filled by BaseFileFind::runNewSearch
    return tr("Open Documents\n%1");
}

bool FindInOpenFiles::isEnabled() const
{
    return Core::DocumentModel::entryCount() > 0;
}

void FindInOpenFiles::writeSettings(QSettings *settings)
{
    settings->beginGroup(QLatin1String("FindInOpenFiles"));
    writeCommonSettings(settings);
    settings->endGroup();
}

void FindInOpenFiles::readSettings(QSettings *settings)
{
    settings->beginGroup(QLatin1String("FindInOpenFiles"));
    readCommonSettings(settings, "*", "");
    settings->endGroup();
}

void FindInOpenFiles::updateEnabledState()
{
    emit enabledChanged(isEnabled());
}
