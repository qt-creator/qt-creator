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

#ifndef GENERICEDITORPLUGIN_H
#define GENERICEDITORPLUGIN_H

#include <extensionsystem/iplugin.h>
#include <texteditor/texteditoractionhandler.h>

#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QLatin1String>
#include <QtCore/QHash>
#include <QtCore/QMultiHash>
#include <QtCore/QSet>
#include <QtCore/QSharedPointer>

QT_BEGIN_NAMESPACE
class QFileInfo;
class QStringList;
QT_END_NAMESPACE

namespace GenericEditor {
namespace Internal {

class HighlightDefinition;
class HighlightDefinitionMetadata;
class Editor;
class EditorFactory;

class GenericEditorPlugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT

public:
    GenericEditorPlugin();
    virtual ~GenericEditorPlugin();

    static GenericEditorPlugin *instance();

    virtual bool initialize(const QStringList &arguments, QString *errorString);
    virtual void extensionsInitialized();

    void initializeEditor(Editor *editor);

    QString definitionIdByName(const QString &name) const;
    QString definitionIdByMimeType(const QString &mimeType) const;
    bool isBuildingDefinition(const QString &id) const;
    const QSharedPointer<HighlightDefinition> &definition(const QString &id);

private slots:
    void lookforAvailableDefinitions();

private:
    Q_DISABLE_COPY(GenericEditorPlugin)

    static GenericEditorPlugin *m_instance;

    void parseDefinitionMetadata(const QFileInfo &fileInfo);
    void registerMimeTypes(const QString &comment,
                           const QStringList &types,
                           const QStringList &patterns);

    struct PriorityCompare
    {
        bool operator()(const QString &a, const QString &b) const
        { return m_priorityById.value(a) < m_priorityById.value(b); }

        QHash<QString, int> m_priorityById;
    };
    PriorityCompare m_priorityComp;

    TextEditor::TextEditorActionHandler *m_actionHandler;

    EditorFactory *m_factory;

    QHash<QString, QString> m_idByName;
    QMultiHash<QString, QString> m_idByMimeType;
    QHash<QString, QSharedPointer<HighlightDefinition> > m_definitions;
    QSet<QString> m_isBuilding;
};

} // namespace Internal
} // namespace GenericEditor

#endif // GENERICEDITORPLUGIN_H
