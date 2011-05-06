/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#ifndef VCSPLUGIN_H
#define VCSPLUGIN_H

#include <extensionsystem/iplugin.h>

#include <QtCore/QObject>

QT_BEGIN_NAMESPACE
class QStandardItemModel;
QT_END_NAMESPACE

namespace VCSBase {
namespace Internal {

struct CommonVcsSettings;
class CommonOptionsPage;
class CoreListener;

class VCSPlugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT

public:
    VCSPlugin();
    ~VCSPlugin();

    bool initialize(const QStringList &arguments, QString *error_message);

    void extensionsInitialized();

    static VCSPlugin *instance();

    CoreListener *coreListener() const;

    CommonVcsSettings settings() const;

    // Model of user nick names used for the submit
    // editor. Stored centrally here to achieve delayed
    // initialization and updating on settings change.
    QStandardItemModel *nickNameModel();

signals:
    void settingsChanged(const VCSBase::Internal::CommonVcsSettings &s);

private slots:
    void slotSettingsChanged();

private:
    void populateNickNameModel();

    static VCSPlugin *m_instance;
    CommonOptionsPage *m_settingsPage;
    QStandardItemModel *m_nickNameModel;
    CoreListener *m_coreListener;
};

} // namespace Internal
} // namespace VCSBase

#endif // VCSPLUGIN_H
