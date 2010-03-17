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
#include <coreplugin/icorelistener.h>
#include <coreplugin/editormanager/ieditor.h>
#include <extensionsystem/pluginmanager.h>

#include <QtCore/QPair>
#include <QtCore/QFileInfo>
#include <QtCore/QStringList>
#include <QtCore/QDebug>

#include <QtGui/QAction>
#include <QtGui/QPlainTextEdit>
#include <QtGui/QStackedWidget>

namespace Core {

class EditorManager;

enum {
    debug = false
};

namespace Internal {

class DesignModeCoreListener : public Core::ICoreListener
{
public:
    DesignModeCoreListener(DesignMode* mode);
    bool coreAboutToClose();
private:
    DesignMode *m_mode;
};

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

struct DesignEditorInfo {
    int widgetIndex;
    QStringList mimeTypes;
    bool preferredMode;
    QWidget *widget;
};

struct DesignModePrivate {
    explicit DesignModePrivate(DesignMode *q, EditorManager *editorManager);
    Internal::DesignModeCoreListener *m_coreListener;
    QWeakPointer<Core::IEditor> m_currentEditor;
    bool m_isActive;

    QList<DesignEditorInfo*> m_editors;

    EditorManager *m_editorManager;
    QStackedWidget *m_stackWidget;
};

DesignModePrivate::DesignModePrivate(DesignMode *q, EditorManager *editorManager) :
    m_coreListener(new Internal::DesignModeCoreListener(q)),
    m_isActive(false),
    m_editorManager(editorManager),
    m_stackWidget(new QStackedWidget)
{
}

DesignMode::DesignMode(EditorManager *editorManager) :
        IMode(), d(new DesignModePrivate(this, editorManager))
{
    setEnabled(false);
    ExtensionSystem::PluginManager::instance()->addObject(d->m_coreListener);

    connect(editorManager, SIGNAL(currentEditorChanged(Core::IEditor*)),
            this, SLOT(currentEditorChanged(Core::IEditor*)));
}

DesignMode::~DesignMode()
{
    ExtensionSystem::PluginManager::instance()->removeObject(d->m_coreListener);
    delete d->m_coreListener;

    qDeleteAll(d->m_editors);
    delete d;
}

QList<int> DesignMode::context() const
{
    static QList<int> contexts = QList<int>() <<
        Core::UniqueIDManager::instance()->uniqueIdentifier(Constants::C_DESIGN_MODE);
    return contexts;
}

QWidget *DesignMode::widget()
{
    return d->m_stackWidget;
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
    foreach(const DesignEditorInfo *i, d->m_editors)
        rc += i->mimeTypes;
    return rc;
}

void DesignMode::registerDesignWidget(QWidget *widget, const QStringList &mimeTypes, bool preferDesignMode)
{
    int index = d->m_stackWidget->addWidget(widget);

    DesignEditorInfo *info = new DesignEditorInfo;
    info->preferredMode = preferDesignMode;
    info->mimeTypes = mimeTypes;
    info->widgetIndex = index;
    info->widget = widget;
    d->m_editors.append(info);
}

void DesignMode::unregisterDesignWidget(QWidget *widget)
{
    d->m_stackWidget->removeWidget(widget);
    foreach(DesignEditorInfo *info, d->m_editors) {
        if (info->widget == widget) {
            d->m_editors.removeAll(info);
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



        foreach(DesignEditorInfo *editorInfo, d->m_editors) {
            foreach(QString mime, editorInfo->mimeTypes) {
                if (mime == mimeType) {
                    d->m_stackWidget->setCurrentIndex(editorInfo->widgetIndex);
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

    if (d->m_currentEditor.data() == editor)
        return;

    if (d->m_currentEditor)
        disconnect(d->m_currentEditor.data(), SIGNAL(changed()), this, SLOT(updateActions()));

    d->m_currentEditor = QWeakPointer<Core::IEditor>(editor);

    if (d->m_currentEditor)
        connect(d->m_currentEditor.data(), SIGNAL(changed()), this, SLOT(updateActions()));

    emit actionsUpdated(d->m_currentEditor.data());
}

void DesignMode::updateActions()
{
    emit actionsUpdated(d->m_currentEditor.data());
}

} // namespace Core
