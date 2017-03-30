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

#include "diffeditor_global.h"

#include <coreplugin/diffservice.h>
#include <extensionsystem/iplugin.h>

QT_FORWARD_DECLARE_CLASS(QAction)

namespace Core { class IEditor; }

namespace DiffEditor {
namespace Internal {

class DiffEditorServiceImpl : public QObject, public Core::DiffService
{
    Q_OBJECT
    Q_INTERFACES(Core::DiffService)
public:
    explicit DiffEditorServiceImpl(QObject *parent = nullptr);

    void diffFiles(const QString &leftFileName, const QString &rightFileName) override;
    void diffModifiedFiles(const QStringList &fileNames) override;
};

class DiffEditorPlugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "DiffEditor.json")

public:
    bool initialize(const QStringList &arguments, QString *errorMessage = 0);
    void extensionsInitialized();

private:
    void updateDiffCurrentFileAction();
    void updateDiffOpenFilesAction();
    void diffCurrentFile();
    void diffOpenFiles();
    void diffExternalFiles();

    QAction *m_diffCurrentFileAction = nullptr;
    QAction *m_diffOpenFilesAction = nullptr;

#ifdef WITH_TESTS
private slots:
    void testMakePatch_data();
    void testMakePatch();
    void testReadPatch_data();
    void testReadPatch();
#endif // WITH_TESTS
};

} // namespace Internal
} // namespace DiffEditor
