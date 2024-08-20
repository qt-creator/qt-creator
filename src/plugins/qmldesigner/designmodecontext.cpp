// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "designmodecontext.h"
#include "assetslibrarywidget.h"
#include "designmodewidget.h"
#include "edit3dwidget.h"
#include "formeditorwidget.h"
#include "materialbrowserwidget.h"
#include "navigatorwidget.h"
#include "qmldesignerconstants.h"
#include "texteditorwidget.h"

namespace QmlDesigner::Internal {

DesignModeContext::DesignModeContext(QWidget *widget)
    : IContext(widget)
{
    setWidget(widget);
    setContext(Core::Context(Constants::qmlDesignerContextId, Constants::qtQuickToolsMenuContextId));
}

void DesignModeContext::contextHelp(const HelpCallback &callback) const
{
    qobject_cast<DesignModeWidget *>(m_widget)->contextHelp(callback);
}

FormEditorContext::FormEditorContext(QWidget *widget)
  : IContext(widget)
{
    setWidget(widget);
    setContext(Core::Context(Constants::qmlFormEditorContextId, Constants::qtQuickToolsMenuContextId));
}

void FormEditorContext::contextHelp(const HelpCallback &callback) const
{
    qobject_cast<FormEditorWidget *>(m_widget)->contextHelp(callback);
}

Editor3DContext::Editor3DContext(QWidget *widget)
  : IContext(widget)
{
    setWidget(widget);
    setContext(Core::Context(Constants::qml3DEditorContextId, Constants::qtQuickToolsMenuContextId));
}

void Editor3DContext::contextHelp(const HelpCallback &callback) const
{
    qobject_cast<Edit3DWidget *>(m_widget)->contextHelp(callback);
}

MaterialBrowserContext::MaterialBrowserContext(QWidget *widget)
  : IContext(widget)
{
    setWidget(widget);
    setContext(Core::Context(Constants::qmlMaterialBrowserContextId,
                             Constants::qtQuickToolsMenuContextId));
}

void MaterialBrowserContext::contextHelp(const HelpCallback &callback) const
{
    qobject_cast<MaterialBrowserWidget *>(m_widget)->contextHelp(callback);
}

AssetsLibraryContext::AssetsLibraryContext(QWidget *widget)
    : IContext(widget)
{
    setWidget(widget);
    setContext(
        Core::Context(Constants::qmlAssetsLibraryContextId, Constants::qtQuickToolsMenuContextId));
}

void AssetsLibraryContext::contextHelp(const HelpCallback &callback) const
{
    qobject_cast<AssetsLibraryWidget *>(m_widget)->contextHelp(callback);
}

NavigatorContext::NavigatorContext(QWidget *widget)
  : IContext(widget)
{
    setWidget(widget);
    setContext(Core::Context(Constants::qmlNavigatorContextId, Constants::qtQuickToolsMenuContextId));
}

void NavigatorContext::contextHelp(const HelpCallback &callback) const
{
    qobject_cast<NavigatorWidget *>(m_widget)->contextHelp(callback);
}

TextEditorContext::TextEditorContext(TextEditorWidget *parent)
    : IContext(parent)
    , m_parent(parent)
{
    static constexpr char qmlTextEditorContextId[] = "QmlDesigner::TextEditor";

    setContext(Core::Context(qmlTextEditorContextId, Constants::qtQuickToolsMenuContextId));
}

void TextEditorContext::contextHelp(const HelpCallback &callback) const
{
    m_parent->contextHelp(callback);
}

} // namespace QmlDesigner::Internal
