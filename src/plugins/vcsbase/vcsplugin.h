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

QT_BEGIN_NAMESPACE
class QStandardItemModel;
QT_END_NAMESPACE

namespace VcsBase {

class VcsBaseSubmitEditor;

namespace Internal {

class CommonVcsSettings;
class CommonOptionsPage;
class CoreListener;

class VcsPlugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "VcsBase.json")

public:
    VcsPlugin();
    ~VcsPlugin() override;

    bool initialize(const QStringList &arguments, QString *errorMessage) override;

    void extensionsInitialized() override;

    static VcsPlugin *instance();

    CommonVcsSettings settings() const;

    // Model of user nick names used for the submit
    // editor. Stored centrally here to achieve delayed
    // initialization and updating on settings change.
    QStandardItemModel *nickNameModel();

signals:
    void settingsChanged(const VcsBase::Internal::CommonVcsSettings &s);
    void submitEditorAboutToClose(VcsBase::VcsBaseSubmitEditor *e, bool *result);

private:
    void slotSettingsChanged();

    void populateNickNameModel();

    static VcsPlugin *m_instance;
    CommonOptionsPage *m_settingsPage;
    QStandardItemModel *m_nickNameModel;
};

} // namespace Internal
} // namespace VcsBase
