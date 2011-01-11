/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "variablemanager.h"
#include "ifile.h"
#include "editormanager/ieditor.h"
#include "editormanager/editormanager.h"

#include <utils/stringutils.h>
#include <utils/qtcassert.h>

#include <QtCore/QFileInfo>
#include <QtCore/QObject>
#include <QtCore/QMap>
#include <QtCore/QDebug>

namespace Core {

class VMMapExpander : public Utils::AbstractQtcMacroExpander {
public:
    virtual bool resolveMacro(const QString &name, QString *ret)
    {
        *ret = Core::VariableManager::instance()->value(name);
        return !ret->isEmpty();
    }
};

class VariableManagerPrivate : public QObject
{
    Q_OBJECT
public:
    void insert(const QString &variable, const QString &value);
    bool remove(const QString &variable);
    void insertFileInfo(const QString &tag, const QFileInfo &file);
    void removeFileInfo(const QString &tag);

public slots:
    void updateCurrentDocument(Core::IEditor *editor);

public:
    QHash<QString, QString> m_map;
    VMMapExpander m_macroExpander;
    static VariableManager *m_instance;
};

VariableManager *VariableManagerPrivate::m_instance = 0;

void VariableManagerPrivate::updateCurrentDocument(Core::IEditor *editor)
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

void VariableManagerPrivate::insert(const QString &variable, const QString &value)
{
    m_map.insert(variable, value);
}

bool VariableManagerPrivate::remove(const QString &variable)
{
    return m_map.remove(variable) > 0;
}

void VariableManagerPrivate::insertFileInfo(const QString &tag, const QFileInfo &file)
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

void VariableManagerPrivate::removeFileInfo(const QString &tag)
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

VariableManager::VariableManager() : d(new VariableManagerPrivate)
{
    VariableManagerPrivate::m_instance = this;
}

VariableManager::~VariableManager()
{
    VariableManagerPrivate::m_instance = 0;
}

void VariableManager::initEditorManagerConnections()
{
    QTC_ASSERT(VariableManagerPrivate::m_instance && Core::EditorManager::instance(), return; )

    QObject::connect(Core::EditorManager::instance(), SIGNAL(currentEditorChanged(Core::IEditor*)),
                     VariableManagerPrivate::m_instance->d.data(), SLOT(updateCurrentDocument(Core::IEditor*)));
}

QString VariableManager::value(const QString &variable) const
{
    return d->m_map.value(variable);
}

QString VariableManager::value(const QString &variable, const QString &defaultValue) const
{
    return d->m_map.value(variable, defaultValue);
}

void VariableManager::insert(const QString &variable, const QString &value)
{
    d->insert(variable, value);
}

void VariableManager::insertFileInfo(const QString &tag, const QFileInfo &file)
{
    d->insertFileInfo(tag, file);
}

void VariableManager::removeFileInfo(const QString &tag)
{
    d->removeFileInfo(tag);
}

bool VariableManager::remove(const QString &variable)
{
    return d->remove(variable);
}

Utils::AbstractMacroExpander *VariableManager::macroExpander()
{
    return &d->m_macroExpander;
}

VariableManager* VariableManager::instance()
{
    return VariableManagerPrivate::m_instance;
}
} // namespace Core

#include "variablemanager.moc"
