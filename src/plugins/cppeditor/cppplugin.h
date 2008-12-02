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
** version 1.2, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/
#ifndef CPPPLUGIN_H
#define CPPPLUGIN_H

#include <QtCore/qplugin.h>
#include <QtCore/QStringList>

#include <extensionsystem/iplugin.h>
#include <coreplugin/editormanager/ieditorfactory.h>

namespace Core {
class ICore;
class IWizard;
}

namespace TextEditor {
class TextEditorActionHandler;
} // namespace TextEditor

namespace CppEditor {
namespace Internal {

class CPPEditor;
class CPPEditorActionHandler;
class CppPluginEditorFactory;

class CppPlugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT

public:
    CppPlugin();
    ~CppPlugin();

    static CppPlugin *instance();
    static Core::ICore *core();

    bool initialize(const QStringList &arguments, QString *error_message = 0);
    void extensionsInitialized();

    // Connect editor to settings changed signals.
    void initializeEditor(CPPEditor *editor);

private slots:
    void switchDeclarationDefinition();
    void jumpToDefinition();

private:
    friend class CppPluginEditorFactory;
    Core::IEditor *createEditor(QWidget *parent);

    static CppPlugin *m_instance;

    Core::ICore *m_core;
    CPPEditorActionHandler *m_actionHandler;
    CppPluginEditorFactory *m_factory;
};

class CppPluginEditorFactory : public Core::IEditorFactory
{
    Q_OBJECT

public:
    CppPluginEditorFactory(CppPlugin *owner);

    virtual QStringList mimeTypes() const;

    Core::IEditor *createEditor(QWidget *parent);

    virtual QString kind() const;
    Core::IFile *open(const QString &fileName);

private:
    const QString m_kind;
    CppPlugin *m_owner;
    QStringList m_mimeTypes;
};

} // namespace Internal
} // namespace CppEditor

#endif // CPPPLUGIN_H
