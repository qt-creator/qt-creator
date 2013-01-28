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

#ifndef BINEDITORPLUGIN_H
#define BINEDITORPLUGIN_H

#include <extensionsystem/iplugin.h>
#include <coreplugin/editormanager/ieditorfactory.h>
#include <coreplugin/icontext.h>

#include <QtPlugin>
#include <QPointer>
#include <QStringList>
#include <QAction>

namespace BINEditor {
class BinEditor;

class BinEditorWidgetFactory : public QObject
{
    Q_OBJECT
public:
    explicit BinEditorWidgetFactory(QObject *parent = 0);

    Q_INVOKABLE QWidget *createWidget(QWidget *parent);
};

namespace Internal {
class BinEditorFactory;

class BinEditorPlugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "BinEditor.json")

public:
    BinEditorPlugin();
    ~BinEditorPlugin();

    bool initialize(const QStringList &arguments, QString *errorMessage = 0);
    void extensionsInitialized();

    // Connect editor to settings changed signals.
    void initializeEditor(BinEditor *editor);

private slots:
    void undoAction();
    void redoAction();
    void copyAction();
    void selectAllAction();
    void updateActions();

    void updateCurrentEditor(Core::IContext *object);

private:
    Core::Context m_context;
    QAction *registerNewAction(Core::Id id, const QString &title = QString());
    QAction *registerNewAction(Core::Id id, QObject *receiver, const char *slot,
                               const QString &title = QString());
    QAction *m_undoAction;
    QAction *m_redoAction;
    QAction *m_copyAction;
    QAction *m_selectAllAction;

    friend class BinEditorFactory;
    Core::IEditor *createEditor(QWidget *parent);

    BinEditorFactory *m_factory;
    QPointer<BinEditor> m_currentEditor;
};

class BinEditorFactory : public Core::IEditorFactory
{
    Q_OBJECT

public:
    explicit BinEditorFactory(BinEditorPlugin *owner);

    QStringList mimeTypes() const;
    Core::IEditor *createEditor(QWidget *parent);
    Core::Id id() const;
    QString displayName() const;

private:
    const QStringList m_mimeTypes;
    BinEditorPlugin *m_owner;
};

} // namespace Internal
} // namespace BINEditor

#endif // BINEDITORPLUGIN_H
