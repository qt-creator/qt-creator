/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of Qt Creator.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef MAEMOPACKAGECREATIONWIDGET_H
#define MAEMOPACKAGECREATIONWIDGET_H

#include <projectexplorer/buildstep.h>

QT_BEGIN_NAMESPACE
namespace Ui { class MaemoPackageCreationWidget; }
QT_END_NAMESPACE

namespace Qt4ProjectManager {
namespace Internal {

class MaemoPackageCreationStep;

class MaemoPackageCreationWidget : public ProjectExplorer::BuildStepConfigWidget
{
    Q_OBJECT
public:
    MaemoPackageCreationWidget(MaemoPackageCreationStep *step);

    virtual void init();
    virtual QString summaryText() const;
    virtual QString displayName() const;

private slots:
    void addFile();
    void removeFile();
    void enableOrDisableRemoveButton();
    void handleSkipButtonToggled(bool checked);
    void versionInfoChanged();

private:
    MaemoPackageCreationStep * const m_step;
    Ui::MaemoPackageCreationWidget * const m_ui;
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // MAEMOPACKAGECREATIONWIDGET_H
