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

#ifndef BUILDENVIRONMENTWIDGET_H
#define BUILDENVIRONMENTWIDGET_H

#include <projectexplorer/buildstep.h>

QT_BEGIN_NAMESPACE
class QCheckBox;
QT_END_NAMESPACE

namespace ProjectExplorer {

class EnvironmentWidget;
class BuildConfiguration;

class PROJECTEXPLORER_EXPORT BuildEnvironmentWidget : public BuildConfigWidget
{
    Q_OBJECT

public:
    BuildEnvironmentWidget();

    QString displayName() const;
    void init(BuildConfiguration *bc);

private slots:
    void environmentModelUserChangesChanged();
    void clearSystemEnvironmentCheckBoxClicked(bool checked);
    void environmentChanged();

private:
    EnvironmentWidget *m_buildEnvironmentWidget;
    QCheckBox *m_clearSystemEnvironmentCheckBox;
    BuildConfiguration *m_buildConfiguration;
};

} // namespace Qt4ProjectManager

#endif // BUILDENVIRONMENTWIDGET_H
