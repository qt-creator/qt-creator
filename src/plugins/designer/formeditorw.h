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

#ifndef FORMEDITORW_H
#define FORMEDITORW_H

#include <QtDesigner/QDesignerFormEditorInterface>

#include <QtCore/QList>
#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QStringList>

#include "designerconstants.h"

QT_BEGIN_NAMESPACE

class QDesignerIntegrationInterface;
class QDesignerFormEditorInterface;
class QDesignerFormWindowInterface;

class QAction;
class QActionGroup;
class QFocusEvent;

class QWidget;
class QSignalMapper;
class QSettings;
class QToolBar;

namespace qdesigner_internal {
    class QDesignerFormWindowManager;
}

QT_END_NAMESPACE

namespace Core {
class ActionManager;
class ActionContainer;
class ICore;
class IEditor;
}

namespace Designer {
namespace Internal {

class FormWindowEditor;
class SettingsPage;


/** FormEditorW is a singleton that stores the Designer CoreInterface and
  * performs centralized operations. The instance() method will return an
  * instance. However, it must be manually deleted when unloading the
  * plugin. Since fully initializing Designer at startup is expensive, the
  * class has an internal partial initialisation stage "RegisterPlugins"
  * which is there to register the Creator plugin objects
  * that must be present at startup (settings pages, actions).
  * The plugin uses this stage at first by calling ensureInitStage().
  * Requesting an editor via instance() will fully initialize the class.
  * This is based on the assumption that the Designer settings work with
  * no plugins loaded. If that does not work, full initialization can be
  * triggered by connection to the ICore::optionsDialogRequested() signal.
  */
class FormEditorW : public QObject
{
    Q_OBJECT
public:
    enum InitializationStage {
        // Register Creator plugins (settings pages, actions)
        RegisterPlugins,
        // Fully initialized for handling editor requests
        FullyInitialized
    };

    virtual ~FormEditorW();

    // Create an instance and initialize up to stage s
    static void ensureInitStage(InitializationStage s);
    // Returns fully initialized instance
    static FormEditorW *instance();
    // Deletes an existing instance if there is one.
    static void deleteInstance();

    inline QDesignerFormEditorInterface *designerEditor() const { return m_formeditor; }
    inline QWidget * const*designerSubWindows() const { return m_designerSubWindows; }
    QToolBar *createEditorToolBar() const;

    FormWindowEditor *createFormWindowEditor(QWidget* parentWidget);

    FormWindowEditor *activeFormWindow();

private slots:
    void activateEditMode(int id);
    void activateEditMode(QAction*);
    void activeFormWindowChanged(QDesignerFormWindowInterface *);
    void currentEditorChanged(Core::IEditor *editor);
    void toolChanged(int);
    void print();

    void editorDestroyed();

private:
    FormEditorW();
    void fullInit();

    void saveSettings(QSettings *s);
    void restoreSettings(const QSettings *s);

    void initDesignerSubWindows();

    typedef QList<FormWindowEditor *> EditorList;

    void setupActions();
    Core::ActionContainer *createPreviewStyleMenu(Core::ActionManager *am,
                                                   QActionGroup *actionGroup);

    void critical(const QString &errorMessage);

    static FormEditorW *m_self;

    QDesignerFormEditorInterface *m_formeditor;
    QDesignerIntegrationInterface *m_integration;
    qdesigner_internal::QDesignerFormWindowManager *m_fwm;
    Core::ICore *m_core;
    InitializationStage m_initStage;

    QWidget *m_designerSubWindows[Designer::Constants::DesignerSubWindowCount];

    QList<SettingsPage *> m_settingsPages;
    QActionGroup *m_actionGroupEditMode;
    QAction *m_actionPrint;
    QAction *m_actionPreview;
    QActionGroup *m_actionGroupPreviewInStyle;

    QList<int> m_context;

    EditorList m_formWindows;
    QStringList m_toolActionIds;
};

} // namespace Internal
} // namespace Designer

#endif // FORMEDITORW_H
