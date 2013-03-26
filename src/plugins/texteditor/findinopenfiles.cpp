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

#include "findinopenfiles.h"
#include "itexteditor.h"

#include <utils/filesearch.h>
#include <coreplugin/icore.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/openeditorsmodel.h>

#include <QSettings>

using namespace Find;
using namespace TextEditor;
using namespace TextEditor::Internal;

FindInOpenFiles::FindInOpenFiles()
{
    connect(Core::ICore::instance()->editorManager(), SIGNAL(editorOpened(Core::IEditor*)),
            this, SLOT(updateEnabledState()));
    connect(Core::ICore::instance()->editorManager(), SIGNAL(editorsClosed(QList<Core::IEditor*>)),
            this, SLOT(updateEnabledState()));
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
                                              const QVariant &additionalParameters) const
{
    Q_UNUSED(nameFilters)
    Q_UNUSED(additionalParameters)
    QMap<QString, QTextCodec *> openEditorEncodings = ITextEditor::openedTextEditorsEncodings();
    QStringList fileNames;
    QList<QTextCodec *> codecs;
    foreach (const Core::OpenEditorsModel::Entry &entry,
             Core::EditorManager::instance()->openedEditorsModel()->entries()) {
        QString fileName = entry.fileName();
        if (!fileName.isEmpty()) {
            fileNames.append(fileName);
            QTextCodec *codec = openEditorEncodings.value(fileName);
            if (!codec)
                codec = Core::EditorManager::instance()->defaultTextCodec();
            codecs.append(codec);
        }
    }

    return new Utils::FileIterator(fileNames, codecs);
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
    return Core::EditorManager::instance()->openedEditors().count() > 0;
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
    readCommonSettings(settings, QLatin1String("*"));
    settings->endGroup();
}

void FindInOpenFiles::updateEnabledState()
{
    emit enabledChanged(isEnabled());
}
