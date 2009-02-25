/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#ifndef BINEDITORPLUGIN_H
#define BINEDITORPLUGIN_H

#include <extensionsystem/iplugin.h>
#include <coreplugin/editormanager/ieditorfactory.h>

#include <QtCore/qplugin.h>
#include <QtCore/QPointer>
#include <QtCore/QStringList>
#include <QtGui/QAction>

namespace Core {
class IWizard;
}

namespace BINEditor {
class BinEditor;
namespace Internal {
class BinEditorFactory;


class BinEditorPlugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT

public:
    BinEditorPlugin();
    ~BinEditorPlugin();

    bool initialize(const QStringList &arguments, QString *error_message = 0);
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
    QList<int> m_context;
    QAction *registerNewAction(const QString &id, const QString &title = QString());
    QAction *registerNewAction(const QString &id, QObject *receiver, const char *slot,
                               const QString &title = QString());
    QAction *m_undoAction;
    QAction *m_redoAction;
    QAction *m_copyAction;
    QAction *m_selectAllAction;

    friend class BinEditorFactory;
    Core::IEditor *createEditor(QWidget *parent);

    typedef QList<Core::IWizard *> WizardList;
    WizardList m_wizards;
    BinEditorFactory *m_factory;
    QPointer<BinEditor> m_currentEditor;
};

class BinEditorFactory : public Core::IEditorFactory
{
    Q_OBJECT

public:
    explicit BinEditorFactory(BinEditorPlugin *owner);

    virtual QStringList mimeTypes() const;

    Core::IEditor *createEditor(QWidget *parent);
    QString kind() const;
    Core::IFile *open(const QString &fileName);

private:
    const QString m_kind;
    const QStringList m_mimeTypes;
    BinEditorPlugin *m_owner;
};

} // namespace Internal
} // namespace BINEditor

#endif // BINEDITORPLUGIN_H
