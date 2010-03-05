/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "variablemanager.h"
#include "ifile.h"

#include <QtCore/QFileInfo>

using namespace Core;

VariableManager *VariableManager::m_instance = 0;

VariableManager::VariableManager(QObject *parent) : QObject(parent)
{
    m_instance = this;
}

VariableManager::~VariableManager()
{
    m_instance = 0;
}

void VariableManager::insert(const QString &variable, const QString &value)
{
    m_map.insert(variable, value);
}

void VariableManager::insertFileInfo(const QString &tag, const QFileInfo &file)
{
    insert(tag, file.filePath());
    insert(tag  + QLatin1String(":absoluteFilePath"), file.absoluteFilePath());
    insert(tag + QLatin1String(":absolutePath"), file.absolutePath());
    insert(tag + QLatin1String(":baseName"), file.baseName());
    insert(tag + QLatin1String(":canonicalPath"), file.canonicalPath());
    insert(tag + QLatin1String(":canonicalFilePath"), file.canonicalFilePath());
    insert(tag + QLatin1String(":completeBaseName"), file.completeBaseName());
    insert(tag + QLatin1String(":completeSuffix"), file.completeSuffix());
    insert(tag + QLatin1String(":fileName"), file.fileName());
    insert(tag + QLatin1String(":filePath"), file.filePath());
    insert(tag + QLatin1String(":path"), file.path());
    insert(tag + QLatin1String(":suffix"), file.suffix());
}

void VariableManager::removeFileInfo(const QString &tag)
{
    if (remove(tag)) {
        remove(tag + QLatin1String(":absoluteFilePath"));
        remove(tag + QLatin1String(":absolutePath"));
        remove(tag + QLatin1String(":baseName"));
        remove(tag + QLatin1String(":canonicalPath"));
        remove(tag + QLatin1String(":canonicalFilePath"));
        remove(tag + QLatin1String(":completeBaseName"));
        remove(tag + QLatin1String(":completeSuffix"));
        remove(tag + QLatin1String(":fileName"));
        remove(tag + QLatin1String(":filePath"));
        remove(tag + QLatin1String(":path"));
        remove(tag + QLatin1String(":suffix"));
    }
}

void VariableManager::updateCurrentDocument(Core::IEditor *editor)
{
    const QString currentDocumentTag = QLatin1String("CURRENT_DOCUMENT");
    removeFileInfo(currentDocumentTag);
    if (editor) {
        if (const Core::IFile *file = editor->file()) {
            const QString fileName = file->fileName();
            if (!fileName.isEmpty())
                insertFileInfo(currentDocumentTag, fileName);
        }
    }
}

QString VariableManager::value(const QString &variable) const
{
    return m_map.value(variable);
}

QString VariableManager::value(const QString &variable, const QString &defaultValue) const
{
    return m_map.value(variable, defaultValue);
}

bool VariableManager::remove(const QString &variable)
{
    return m_map.remove(variable) > 0;
}

QString VariableManager::resolve(const QString &stringWithVariables) const
{
    QString result = stringWithVariables;
    QMapIterator<QString, QString> i(m_map);
    while (i.hasNext()) {
        i.next();
        QString key = QLatin1String("${");
        key += i.key();
        key += QLatin1Char('}');
        result.replace(key, i.value());
    }
    return result;
}
