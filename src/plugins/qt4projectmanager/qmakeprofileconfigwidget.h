/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
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
**
**************************************************************************/

#ifndef QT4PM_QMAKEPROFILECONFIGWIDGET_H
#define QT4PM_QMAKEPROFILECONFIGWIDGET_H

#include <projectexplorer/profileconfigwidget.h>

QT_FORWARD_DECLARE_CLASS(QLineEdit)

namespace ProjectExplorer { class Profile; }

namespace Qt4ProjectManager {
class BaseQtVersion;

namespace Internal {

class QmakeProfileConfigWidget : public ProjectExplorer::ProfileConfigWidget
{
    Q_OBJECT

public:
    explicit QmakeProfileConfigWidget(ProjectExplorer::Profile *p, QWidget *parent = 0);

    QString displayName() const;

    void makeReadOnly();

    void apply();
    void discard();
    bool isDirty() const;

private:
    int findQtVersion(const int id) const;

    ProjectExplorer::Profile *m_profile;
    QLineEdit *m_lineEdit;
};

} // namespace Internal
} // namespace Debugger

#endif // QT4PM_QMAKEPROFILECONFIGWIDGET_H
