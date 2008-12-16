/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.3, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

#ifndef RRESOURCEEDITORFACTORY_H
#define RRESOURCEEDITORFACTORY_H

#include <extensionsystem/ExtensionSystemInterfaces>
#include <coreplugin/editormanager/ieditorfactory.h>

#include <QtCore/QStringList>

namespace Core {
class ICore;
class IEditor;
class IFile;
}

namespace ResourceEditor {
namespace Internal {

class ResourceEditorPlugin;

class ResourceEditorFactory : public Core::IEditorFactory
{
    Q_OBJECT

public:
    typedef QList<int> Context;

    ResourceEditorFactory(Core::ICore *core, ResourceEditorPlugin *plugin);

    virtual QStringList mimeTypes() const;

    //EditorFactory
    QString kind() const;
    Core::IFile *open(const QString &fileName);
    Core::IEditor *createEditor(QWidget *parent);

private:
    const QStringList m_mimeTypes;
    const QString m_kind;
    Context m_context;

    Core::ICore *m_core;
    ResourceEditorPlugin *m_plugin;
};

} // namespace Internal
} // namespace ResourceEditor

#endif // RRESOURCEEDITORFACTORY_H
