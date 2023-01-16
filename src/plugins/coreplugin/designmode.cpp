// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "designmode.h"

#include "coreconstants.h"
#include "coreicons.h"
#include "coreplugintr.h"
#include "editormanager/editormanager.h"
#include "editormanager/ieditor.h"
#include "icore.h"
#include "idocument.h"
#include "modemanager.h"

#include <extensionsystem/pluginmanager.h>

#include <QDebug>
#include <QPointer>
#include <QStackedWidget>
#include <QStringList>

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
    bool m_isActive = false;
    QList<DesignEditorInfo*> m_editors;
    QStackedWidget *m_stackWidget;
    Context m_activeContext;
};

DesignModePrivate::DesignModePrivate()
    : m_stackWidget(new QStackedWidget)
{}

DesignModePrivate::~DesignModePrivate()
{
    delete m_stackWidget;
}

static DesignMode *m_instance = nullptr;
static DesignModePrivate *d = nullptr;

DesignMode::DesignMode()
{
    ICore::addPreCloseListener([] {
        m_instance->currentEditorChanged(nullptr);
        return true;
    });

    setObjectName(QLatin1String("DesignMode"));
    setEnabled(false);
    setContext(Context(Constants::C_DESIGN_MODE));
    setWidget(d->m_stackWidget);
    setDisplayName(Tr::tr("Design"));
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
}

DesignMode *DesignMode::instance()
{
    return m_instance;
}

void DesignMode::setDesignModeIsRequired()
{
    // d != nullptr indicates "isRequired".
    if (!d)
        d = new DesignModePrivate;
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

    auto info = new DesignEditorInfo;
    info->mimeTypes = mimeTypes;
    info->context = context;
    info->widgetIndex = index;
    info->widget = widget;
    d->m_editors.append(info);
}

void DesignMode::unregisterDesignWidget(QWidget *widget)
{
    d->m_stackWidget->removeWidget(widget);
    for (DesignEditorInfo *info : std::as_const(d->m_editors)) {
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
            for (const DesignEditorInfo *editorInfo : std::as_const(d->m_editors)) {
                for (const QString &mime : editorInfo->mimeTypes) {
                    if (mime == mimeType) {
                        d->m_stackWidget->setCurrentIndex(editorInfo->widgetIndex);
                        setActiveContext(editorInfo->context);
                        mimeEditorAvailable = true;
                        setEnabled(true);
                        break;
                    }
                }
                if (mimeEditorAvailable)
                    break;
            }
        }
    }
    if (d->m_currentEditor)
        disconnect(d->m_currentEditor.data()->document(), &IDocument::changed, this, &DesignMode::updateActions);

    if (!mimeEditorAvailable) {
        setActiveContext(Context());
        if (ModeManager::currentModeId() == id())
            ModeManager::activateMode(Constants::MODE_EDIT);
        setEnabled(false);
        d->m_currentEditor = nullptr;
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

void DesignMode::updateContext(Utils::Id newMode, Utils::Id oldMode)
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

    if (ModeManager::currentModeId() == id())
        ICore::updateAdditionalContexts(d->m_activeContext, context);

    d->m_activeContext = context;
}

void DesignMode::createModeIfRequired()
{
    if (d) {
        m_instance = new DesignMode;
        ExtensionSystem::PluginManager::addObject(m_instance);
    }
}

void DesignMode::destroyModeIfRequired()
{
    if (m_instance) {
        ExtensionSystem::PluginManager::removeObject(m_instance);
        delete m_instance;
    }
    delete d;
}

} // namespace Core
