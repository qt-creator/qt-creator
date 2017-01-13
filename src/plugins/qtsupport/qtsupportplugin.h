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

#pragma once

#include <extensionsystem/iplugin.h>

namespace QtSupport {
namespace Internal {

class ExamplesWelcomePage;

class QtSupportPlugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "QtSupport.json")

public:
    bool initialize(const QStringList &arguments, QString *errorMessage);
    void extensionsInitialized();

#ifdef WITH_TESTS
private slots:
    void testQtOutputParser_data();
    void testQtOutputParser();
    void testQtOutputFormatter_data();
    void testQtOutputFormatter();
    void testQtOutputFormatter_appendMessage_data();
    void testQtOutputFormatter_appendMessage();
    void testQtOutputFormatter_appendMixedAssertAndAnsi();

    void testQtProjectImporter_oneProject_data();
    void testQtProjectImporter_oneProject();

    void testQtBuildStringParsing_data();
    void testQtBuildStringParsing();

#if 0
    void testQtProjectImporter_oneProjectExistingKit();
    void testQtProjectImporter_oneProjectNewKitExistingQt();
    void testQtProjectImporter_oneProjectNewKitNewQt();
    void testQtProjectImporter_oneProjectTwoNewKitSameNewQt_pc();
    void testQtProjectImporter_oneProjectTwoNewKitSameNewQt_cp();
    void testQtProjectImporter_oneProjectTwoNewKitSameNewQt_cc();
#endif
#endif
};

} // namespace Internal
} // namespace QtSupport
