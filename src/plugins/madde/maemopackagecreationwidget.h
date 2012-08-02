/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
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

#ifndef MAEMOPACKAGECREATIONWIDGET_H
#define MAEMOPACKAGECREATIONWIDGET_H

#include <projectexplorer/buildstep.h>
#include <utils/fileutils.h>

namespace Madde {
namespace Internal {

namespace Ui { class MaemoPackageCreationWidget; }
class AbstractMaemoPackageCreationStep;

class MaemoPackageCreationWidget : public ProjectExplorer::BuildStepConfigWidget
{
    Q_OBJECT

public:
    MaemoPackageCreationWidget(AbstractMaemoPackageCreationStep *step);
    ~MaemoPackageCreationWidget();

    virtual QString summaryText() const;
    virtual QString displayName() const;

private slots:
    void editDebianFile();
    void versionInfoChanged();
    void initGui();
    void updateDebianFileList(const Utils::FileName &debianDir);
    void updateVersionInfo();
    void handleControlFileUpdate(const Utils::FileName &debianDir);
    void setPackageManagerIcon();
    void setPackageManagerName();
    void setPackageName();
    void setShortDescription();

private:
    void updatePackageManagerIcon();
    void updatePackageName();
    void updatePackageManagerName();
    void updateShortDescription();
    void editFile(const QString &filePath);

    AbstractMaemoPackageCreationStep * const m_step;
    Ui::MaemoPackageCreationWidget * const m_ui;
};

} // namespace Internal
} // namespace Madde

#endif // MAEMOPACKAGECREATIONWIDGET_H
