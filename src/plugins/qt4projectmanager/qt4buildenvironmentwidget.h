/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef QT4BUILDENVIRONMENTWIDGET_H
#define QT4BUILDENVIRONMENTWIDGET_H

#include <projectexplorer/buildstep.h>

QT_BEGIN_NAMESPACE
class QCheckBox;
QT_END_NAMESPACE

namespace ProjectExplorer {
class EnvironmentWidget;
}

namespace Qt4ProjectManager {

class Qt4Project;

namespace Internal {
class Qt4BuildConfiguration;

class Qt4BuildEnvironmentWidget : public ProjectExplorer::BuildConfigWidget
{
    Q_OBJECT

public:
    Qt4BuildEnvironmentWidget(Qt4Project *project);

    QString displayName() const;
    void init(ProjectExplorer::BuildConfiguration *bc);

private slots:
    void environmentModelUserChangesUpdated();
    void clearSystemEnvironmentCheckBoxClicked(bool checked);
    void environmentChanged();

private:
    ProjectExplorer::EnvironmentWidget *m_buildEnvironmentWidget;
    QCheckBox *m_clearSystemEnvironmentCheckBox;
    Qt4Project *m_pro;
    Qt4BuildConfiguration *m_buildConfiguration;
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // QT4BUILDENVIRONMENTWIDGET_H
