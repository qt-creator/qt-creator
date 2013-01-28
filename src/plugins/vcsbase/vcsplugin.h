/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef VCSPLUGIN_H
#define VCSPLUGIN_H

#include <extensionsystem/iplugin.h>

#include <QObject>

QT_BEGIN_NAMESPACE
class QStandardItemModel;
QT_END_NAMESPACE

namespace VcsBase {
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
    ~VcsPlugin();

    bool initialize(const QStringList &arguments, QString *errorMessage);

    void extensionsInitialized();

    static VcsPlugin *instance();

    CoreListener *coreListener() const;

    CommonVcsSettings settings() const;

    // Model of user nick names used for the submit
    // editor. Stored centrally here to achieve delayed
    // initialization and updating on settings change.
    QStandardItemModel *nickNameModel();

signals:
    void settingsChanged(const VcsBase::Internal::CommonVcsSettings &s);

private slots:
    void slotSettingsChanged();

private:
    void populateNickNameModel();

    static VcsPlugin *m_instance;
    CommonOptionsPage *m_settingsPage;
    QStandardItemModel *m_nickNameModel;
    CoreListener *m_coreListener;
};

} // namespace Internal
} // namespace VcsBase

#endif // VCSPLUGIN_H
