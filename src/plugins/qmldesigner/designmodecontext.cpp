// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "designmodecontext.h"
#include "assetslibrarywidget.h"
#include "collectionwidget.h"
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
    setContext(Core::Context(Constants::C_QMLDESIGNER, Constants::C_QT_QUICK_TOOLS_MENU));
}

void DesignModeContext::contextHelp(const HelpCallback &callback) const
{
    qobject_cast<DesignModeWidget *>(m_widget)->contextHelp(callback);
}

FormEditorContext::FormEditorContext(QWidget *widget)
  : IContext(widget)
{
    setWidget(widget);
    setContext(Core::Context(Constants::C_QMLFORMEDITOR, Constants::C_QT_QUICK_TOOLS_MENU));
}

void FormEditorContext::contextHelp(const HelpCallback &callback) const
{
    qobject_cast<FormEditorWidget *>(m_widget)->contextHelp(callback);
}

Editor3DContext::Editor3DContext(QWidget *widget)
  : IContext(widget)
{
    setWidget(widget);
    setContext(Core::Context(Constants::C_QMLEDITOR3D, Constants::C_QT_QUICK_TOOLS_MENU));
}

void Editor3DContext::contextHelp(const HelpCallback &callback) const
{
    qobject_cast<Edit3DWidget *>(m_widget)->contextHelp(callback);
}

MaterialBrowserContext::MaterialBrowserContext(QWidget *widget)
  : IContext(widget)
{
    setWidget(widget);
    setContext(Core::Context(Constants::C_QMLMATERIALBROWSER, Constants::C_QT_QUICK_TOOLS_MENU));
}

void MaterialBrowserContext::contextHelp(const HelpCallback &callback) const
{
    qobject_cast<MaterialBrowserWidget *>(m_widget)->contextHelp(callback);
}

AssetsLibraryContext::AssetsLibraryContext(QWidget *widget)
    : IContext(widget)
{
    setWidget(widget);
    setContext(Core::Context(Constants::C_QMLASSETSLIBRARY, Constants::C_QT_QUICK_TOOLS_MENU));
}

void AssetsLibraryContext::contextHelp(const HelpCallback &callback) const
{
    qobject_cast<AssetsLibraryWidget *>(m_widget)->contextHelp(callback);
}

NavigatorContext::NavigatorContext(QWidget *widget)
  : IContext(widget)
{
    setWidget(widget);
    setContext(Core::Context(Constants::C_QMLNAVIGATOR, Constants::C_QT_QUICK_TOOLS_MENU));
}

void NavigatorContext::contextHelp(const HelpCallback &callback) const
{
    qobject_cast<NavigatorWidget *>(m_widget)->contextHelp(callback);
}

TextEditorContext::TextEditorContext(QWidget *widget)
  : IContext(widget)
{
    setWidget(widget);
    setContext(Core::Context(Constants::C_QMLTEXTEDITOR, Constants::C_QT_QUICK_TOOLS_MENU));
}

void TextEditorContext::contextHelp(const HelpCallback &callback) const
{
    qobject_cast<TextEditorWidget *>(m_widget)->contextHelp(callback);
}

CollectionEditorContext::CollectionEditorContext(QWidget *widget)
    : IContext(widget)
{
    setWidget(widget);
    setContext(Core::Context(Constants::C_QMLCOLLECTIONEDITOR, Constants::C_QT_QUICK_TOOLS_MENU));
}

void CollectionEditorContext::contextHelp(const HelpCallback &callback) const
{
    qobject_cast<CollectionWidget *>(m_widget)->contextHelp(callback);
}
} // namespace QmlDesigner::Internal
