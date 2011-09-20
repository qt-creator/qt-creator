/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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

#ifndef MAEMOPACKAGECREATIONWIDGET_H
#define MAEMOPACKAGECREATIONWIDGET_H

#include <projectexplorer/buildstep.h>

QT_BEGIN_NAMESPACE
namespace Ui { class MaemoPackageCreationWidget; }
QT_END_NAMESPACE

namespace Madde {
namespace Internal {
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
    void editSpecFile();
    void versionInfoChanged();
    void initGui();
    void updateDebianFileList();
    void updateVersionInfo();
    void handleControlFileUpdate();
    void handleSpecFileUpdate();
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
