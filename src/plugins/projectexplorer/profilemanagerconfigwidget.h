/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef PROFILEMANAGERWIDGET_H
#define PROFILEMANAGERWIDGET_H

#include "profileconfigwidget.h"

QT_BEGIN_NAMESPACE
class QHBoxLayout;
class QGridLayout;
class QToolButton;
QT_END_NAMESPACE

namespace ProjectExplorer {
class Profile;

namespace Internal {

class ProfileManagerConfigWidget : public ProjectExplorer::ProfileConfigWidget
{
    Q_OBJECT

public:
    ProfileManagerConfigWidget(Profile *p, QWidget *parent = 0);

    QString displayName() const;

    void apply();
    void discard();
    bool isDirty() const;
    void addConfigWidget(ProjectExplorer::ProfileConfigWidget *widget);
    void makeReadOnly();

private slots:
    void setIcon();

private:
    QGridLayout *m_layout;
    QToolButton *m_iconButton;
    QList<ProfileConfigWidget *> m_widgets;
    Profile *m_profile;
    QString m_iconPath;
};

} // namespace Internal
} // namespace ProjectExplorer

#endif // PROFILEMANAGERCONFIGWIDGET_H
