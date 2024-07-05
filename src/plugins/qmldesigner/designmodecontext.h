// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/icontext.h>

namespace QmlDesigner {
namespace Internal {

/**
  * Bauhaus Design mode context object
  */
class DesignModeContext : public Core::IContext
{
    Q_OBJECT

public:
    DesignModeContext(QWidget *widget);
};

class FormEditorContext : public Core::IContext
{
    Q_OBJECT

public:
    FormEditorContext(QWidget *widget);
};

class Editor3DContext : public Core::IContext
{
    Q_OBJECT

public:
    Editor3DContext(QWidget *widget);
};

class MaterialBrowserContext : public Core::IContext
{
    Q_OBJECT

public:
    MaterialBrowserContext(QWidget *widget);
};

class AssetsLibraryContext : public Core::IContext
{
    Q_OBJECT

public:
    AssetsLibraryContext(QWidget *widget);
};

class NavigatorContext : public Core::IContext
{
    Q_OBJECT

public:
    NavigatorContext(QWidget *widget);
};

class TextEditorContext : public Core::IContext
{
    Q_OBJECT

public:
    TextEditorContext(QWidget *widget);
};
} // namespace Internal
} // namespace QmlDesigner
