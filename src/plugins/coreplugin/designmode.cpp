/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "designmode.h"

#include <coreplugin/icore.h>
#include <coreplugin/idocument.h>
#include <coreplugin/modemanager.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/coreicons.h>

#include <coreplugin/editormanager/ieditor.h>

#include <QPointer>
#include <QStringList>
#include <QDebug>

#include <QStackedWidget>

static Core::DesignMode *m_instance = 0;

namespace Core {

struct DesignEditorInfo
{
    int widgetIndex;
    QStringList mimeTypes;
    Context context;
    QWidget *widget;
};

class DesignModePrivate
{
public:
    DesignModePrivate();
    ~DesignModePrivate();

public:
    QPointer<IEditor> m_currentEditor;
    bool m_isActive;
    bool m_isRequired;
    QList<DesignEditorInfo*> m_editors;
    QStackedWidget *m_stackWidget;
    Context m_activeContext;
};

DesignModePrivate::DesignModePrivate()
    : m_isActive(false),
      m_isRequired(false),
      m_stackWidget(new QStackedWidget)
{}

DesignModePrivate::~DesignModePrivate()
{
    delete m_stackWidget;
}

DesignMode::DesignMode()
    : d(new DesignModePrivate)
{
    m_instance = this;

    ICore::addPreCloseListener([]() -> bool {
        m_instance->currentEditorChanged(0);
        return true;
    });

    setObjectName(QLatin1String("DesignMode"));
    setEnabled(false);
    setContext(Context(Constants::C_DESIGN_MODE));
    setWidget(d->m_stackWidget);
    setDisplayName(tr("Design"));
    setIcon(Utils::Icon::modeIcon(Icons::MODE_DESIGN_CLASSIC,
                                  Icons::MODE_DESIGN_FLAT, Icons::MODE_DESIGN_FLAT_ACTIVE));
    setPriority(Constants::P_MODE_DESIGN);
    setId(Constants::MODE_DESIGN);

    connect(EditorManager::instance(), &EditorManager::currentEditorChanged,
            this, &DesignMode::currentEditorChanged);

    connect(ModeManager::instance(), &ModeManager::currentModeChanged,
            this, &DesignMode::updateContext);
}

DesignMode::~DesignMode()
{
    qDeleteAll(d->m_editors);
    delete d;
}

DesignMode *DesignMode::instance()
{
    return m_instance;
}

void DesignMode::setDesignModeIsRequired()
{
    d->m_isRequired = true;
}

bool DesignMode::designModeIsRequired() const
{
    return d->m_isRequired;
}

QStringList DesignMode::registeredMimeTypes() const
{
    QStringList rc;
    foreach (const DesignEditorInfo *i, d->m_editors)
        rc += i->mimeTypes;
    return rc;
}

/**
  * Registers a widget to be displayed when an editor with a file specified in
  * mimeTypes is opened. This also appends the additionalContext in ICore to
  * the context, specified here.
  */
void DesignMode::registerDesignWidget(QWidget *widget,
                                      const QStringList &mimeTypes,
                                      const Context &context)
{
    setDesignModeIsRequired();
    int index = d->m_stackWidget->addWidget(widget);

    DesignEditorInfo *info = new DesignEditorInfo;
    info->mimeTypes = mimeTypes;
    info->context = context;
    info->widgetIndex = index;
    info->widget = widget;
    d->m_editors.append(info);
}

void DesignMode::unregisterDesignWidget(QWidget *widget)
{
    d->m_stackWidget->removeWidget(widget);
    foreach (DesignEditorInfo *info, d->m_editors) {
        if (info->widget == widget) {
            d->m_editors.removeAll(info);
            delete info;
            break;
        }
    }
}

// if editor changes, check if we have valid mimetype registered.
void DesignMode::currentEditorChanged(IEditor *editor)
{
    if (editor && (d->m_currentEditor.data() == editor))
        return;

    bool mimeEditorAvailable = false;

    if (editor) {
        const QString mimeType = editor->document()->mimeType();
        if (!mimeType.isEmpty()) {
            foreach (DesignEditorInfo *editorInfo, d->m_editors) {
                foreach (const QString &mime, editorInfo->mimeTypes) {
                    if (mime == mimeType) {
                        d->m_stackWidget->setCurrentIndex(editorInfo->widgetIndex);
                        setActiveContext(editorInfo->context);
                        mimeEditorAvailable = true;
                        setEnabled(true);
                        break;
                    }
                } // foreach mime
                if (mimeEditorAvailable)
                    break;
            } // foreach editorInfo
        }
    }
    if (d->m_currentEditor)
        disconnect(d->m_currentEditor.data()->document(), &IDocument::changed, this, &DesignMode::updateActions);

    if (!mimeEditorAvailable) {
        setActiveContext(Context());
        if (ModeManager::currentMode() == id())
            ModeManager::activateMode(Constants::MODE_EDIT);
        setEnabled(false);
        d->m_currentEditor = 0;
        emit actionsUpdated(d->m_currentEditor.data());
    } else {
        d->m_currentEditor = editor;

        if (d->m_currentEditor)
            connect(d->m_currentEditor.data()->document(), &IDocument::changed, this, &DesignMode::updateActions);

        emit actionsUpdated(d->m_currentEditor.data());
    }
}

void DesignMode::updateActions()
{
    emit actionsUpdated(d->m_currentEditor.data());
}

void DesignMode::updateContext(Id newMode, Id oldMode)
{
    if (newMode == id())
        ICore::addAdditionalContext(d->m_activeContext);
    else if (oldMode == id())
        ICore::removeAdditionalContext(d->m_activeContext);
}

void DesignMode::setActiveContext(const Context &context)
{
    if (d->m_activeContext == context)
        return;

    if (ModeManager::currentMode() == id())
        ICore::updateAdditionalContexts(d->m_activeContext, context);

    d->m_activeContext = context;
}

} // namespace Core
