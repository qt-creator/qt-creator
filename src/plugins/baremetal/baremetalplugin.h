// Copyright (C) 2016 Tim Sander <tim@krieglstein.org>
// Copyright (C) 2016 Denis Shienkov <denis.shienkov@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <extensionsystem/iplugin.h>

namespace BareMetal::Internal {

class BareMetalPlugin final : public ExtensionSystem::IPlugin
{
   Q_OBJECT
   Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "BareMetal.json")

   ~BareMetalPlugin() final;

   void initialize() final;
   void extensionsInitialized() final;

   class BareMetalPluginPrivate *d = nullptr;

#ifdef WITH_TESTS
private slots:
   void testIarOutputParsers_data();
   void testIarOutputParsers();
   void testKeilOutputParsers_data();
   void testKeilOutputParsers();
   void testSdccOutputParsers_data();
   void testSdccOutputParsers();
#endif // WITH_TESTS
};

} // BareMetal::Internal
