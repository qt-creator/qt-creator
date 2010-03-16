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

#include "designmode.h"

#include <coreplugin/icore.h>
#include <coreplugin/modemanager.h>
#include <coreplugin/uniqueidmanager.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/openeditorsmodel.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/mimedatabase.h>
#include <extensionsystem/pluginmanager.h>

#include <QtCore/QPair>
#include <QtCore/QFileInfo>
#include <QtGui/QAction>

#include <QtGui/QPlainTextEdit>
#include <QtGui/QStackedWidget>

#include <QtCore/QDebug>

namespace Core {

class EditorManager;

enum {
    debug = false
};

namespace Internal {

DesignModeCoreListener::DesignModeCoreListener(DesignMode *mode) :
        m_mode(mode)
{
}

bool DesignModeCoreListener::coreAboutToClose()
{
    m_mode->currentEditorChanged(0);
    return true;
}

} // namespace Internal

DesignMode::DesignMode(EditorManager *editorManager) :
        IMode(),
        m_coreListener(new Internal::DesignModeCoreListener(this)),
        m_isActive(false),
        m_editorManager(editorManager),
        m_stackWidget(new QStackedWidget)
{
    setEnabled(false);
    ExtensionSystem::PluginManager::instance()->addObject(m_coreListener);

    connect(editorManager, SIGNAL(currentEditorChanged(Core::IEditor*)),
            this, SLOT(currentEditorChanged(Core::IEditor*)));
    //updateActions();
}

DesignMode::~DesignMode()
{
    ExtensionSystem::PluginManager::instance()->removeObject(m_coreListener);
    delete m_coreListener;

    qDeleteAll(m_editors);
}

QList<int> DesignMode::context() const
{
    static QList<int> contexts = QList<int>() <<
        Core::UniqueIDManager::instance()->uniqueIdentifier(Constants::C_DESIGN_MODE);
    return contexts;
}

QWidget *DesignMode::widget()
{
    return m_stackWidget;
}

QString DesignMode::displayName() const
{
    return tr("Design");
}

QIcon DesignMode::icon() const
{
    return QIcon(QLatin1String(":/fancyactionbar/images/mode_Design.png"));
}

int DesignMode::priority() const
{
    return Constants::P_MODE_DESIGN;
}

QString DesignMode::id() const
{
    return QLatin1String(Constants::MODE_DESIGN);
}

QStringList DesignMode::registeredMimeTypes() const
{
    QStringList rc;
    foreach(const DesignEditorInfo *i, m_editors)
        rc += i->mimeTypes;
    return rc;
}

void DesignMode::registerDesignWidget(QWidget *widget, const QStringList &mimeTypes, bool preferDesignMode)
{
    int index = m_stackWidget->addWidget(widget);

    DesignEditorInfo *info = new DesignEditorInfo;
    info->preferredMode = preferDesignMode;
    info->mimeTypes = mimeTypes;
    info->widgetIndex = index;
    info->widget = widget;
    m_editors.append(info);
}

void DesignMode::unregisterDesignWidget(QWidget *widget)
{
    m_stackWidget->removeWidget(widget);
    foreach(DesignEditorInfo *info, m_editors) {
        if (info->widget == widget) {
            m_editors.removeAll(info);
            break;
        }
    }
}

// if editor changes, check if we have valid mimetype registered.
void DesignMode::currentEditorChanged(Core::IEditor *editor)
{
    bool mimeEditorAvailable = false;
    bool modeActivated = false;
    Core::ICore *core = Core::ICore::instance();

    if (editor && editor->file()) {
        MimeType type = core->mimeDatabase()->findByFile(QFileInfo(editor->file()->fileName()));
        QString mimeType = editor->file()->mimeType();

        if (type && !type.type().isEmpty())
            mimeType = type.type();



        foreach(DesignEditorInfo *editorInfo, m_editors) {
            foreach(QString mime, editorInfo->mimeTypes) {
                if (mime == mimeType) {
                    m_stackWidget->setCurrentIndex(editorInfo->widgetIndex);
                    mimeEditorAvailable = true;
                    setEnabled(true);
                    if (editorInfo->preferredMode && core->modeManager()->currentMode() != this) {
                        core->modeManager()->activateMode(Constants::MODE_DESIGN);
                        modeActivated = true;
                    }
                    break;
                }
            }
            if (mimeEditorAvailable)
                break;
        }
    }
    if (!mimeEditorAvailable)
        setEnabled(false);

    if (!mimeEditorAvailable && core->modeManager()->currentMode() == this)
    {
        // switch back to edit mode - we don't want to be here
        core->modeManager()->activateMode(Constants::MODE_EDIT);
    }

    if (m_currentEditor.data() == editor)
        return;

    if (m_currentEditor)
        disconnect(m_currentEditor.data(), SIGNAL(changed()), this, SLOT(updateActions()));

    m_currentEditor = QWeakPointer<Core::IEditor>(editor);

    if (m_currentEditor)
        connect(m_currentEditor.data(), SIGNAL(changed()), this, SLOT(updateActions()));

    emit actionsUpdated(m_currentEditor.data());
}

void DesignMode::updateActions()
{
    emit actionsUpdated(m_currentEditor.data());
}

} // namespace Core
