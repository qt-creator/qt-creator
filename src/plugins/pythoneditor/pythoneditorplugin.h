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

#ifndef PYTHONEDITOR_PLUGIN_H
#define PYTHONEDITOR_PLUGIN_H

#include <extensionsystem/iplugin.h>
#include <texteditor/texteditoractionhandler.h>
#include <QSet>
#include <QScopedPointer>

namespace PythonEditor {

class EditorFactory;
class EditorWidget;

/**
  \class PyEditor::Plugin implements ExtensionSystem::IPlugin
  Singletone object - PyEditor plugin
  */
class PythonEditorPlugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "PythonEditor.json")

public:
    PythonEditorPlugin();
    virtual ~PythonEditorPlugin();

    virtual bool initialize(const QStringList &arguments, QString *errorMessage);
    virtual void extensionsInitialized();
    static PythonEditorPlugin *instance() { return m_instance; }
    static void initializeEditor(EditorWidget *widget);

    static QSet<QString> keywords();
    static QSet<QString> magics();
    static QSet<QString> builtins();

private:
    static PythonEditorPlugin *m_instance;
    EditorFactory *m_factory;
    QScopedPointer<TextEditor::TextEditorActionHandler> m_actionHandler;
    QSet<QString> m_keywords;
    QSet<QString> m_magics;
    QSet<QString> m_builtins;
};

} // namespace PythonEditor

#endif // PYTHONEDITOR_PLUGIN_H
