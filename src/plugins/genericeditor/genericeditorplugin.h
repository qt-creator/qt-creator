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
#include <QtCore/QLatin1String>
#include <QtCore/QHash>
#include <QtCore/QSet>
#include <QtCore/QSharedPointer>

namespace GenericEditor {
namespace Internal {

class HighlightDefinition;
class Editor;

// Note: The general interface of this class is temporary. Still need discussing details about
// the definition files integration with Creator.

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

    static const QLatin1String kAlertDefinitionId;
    static const QLatin1String kCDefinitionId;
    static const QLatin1String kCppDefinitionId;
    static const QLatin1String kCssDefinitionId;
    static const QLatin1String kDoxygenDefinitionId;
    static const QLatin1String kFortranDefinitionId;
    static const QLatin1String kHtmlDefinitionId;
    static const QLatin1String kJavaDefinitionId;
    static const QLatin1String kJavadocDefinitionId;
    static const QLatin1String kJavascriptDefinitionId;
    static const QLatin1String kObjectiveCDefinitionId;
    static const QLatin1String kPerlDefinitionId;
    static const QLatin1String kPhpDefinitionId;
    static const QLatin1String kPythonDefinitionId;
    static const QLatin1String kRubyDefinitionId;
    static const QLatin1String kSqlDefinitionId;
    static const QLatin1String kTclDefinitionId;

private:
    GenericEditorPlugin(const GenericEditorPlugin &HighlighterPlugin);
    const GenericEditorPlugin &operator=(const GenericEditorPlugin &HighlighterPlugin);

    static GenericEditorPlugin *m_instance;

    QSet<QString> m_isBuilding;
    QHash<QString, QString> m_idByName;
    QHash<QString, QString> m_idByMimeType;
    QHash<QString, QSharedPointer<HighlightDefinition> > m_definitions;

    TextEditor::TextEditorActionHandler *m_actionHandler;
};

} // namespace Internal
} // namespace GenericEditor

#endif // GENERICEDITORPLUGIN_H
