/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef QT4PROJECTCONFIGWIDGET_H
#define QT4PROJECTCONFIGWIDGET_H

#include <projectexplorer/buildstep.h>

QT_BEGIN_NAMESPACE
class QAbstractButton;
QT_END_NAMESPACE

namespace Utils {
    class DetailsWidget;
}

namespace Qt4ProjectManager {
class Qt4BaseTarget;
class Qt4BuildConfiguration;
class Qt4Target;

namespace Internal {
namespace Ui {
class Qt4ProjectConfigWidget;
}

class Qt4ProjectConfigWidget : public ProjectExplorer::BuildConfigWidget
{
    Q_OBJECT
public:
    explicit Qt4ProjectConfigWidget(Qt4BaseTarget *target);
    ~Qt4ProjectConfigWidget();

    QString displayName() const;
    void init(ProjectExplorer::BuildConfiguration *bc);

private slots:
    // User changes in our widgets
    void shadowBuildClicked(bool checked);
    void onBeforeBeforeShadowBuildDirBrowsed();
    void shadowBuildEdited();
    void qtVersionSelected(const QString &);
    void toolChainSelected(int index);
    void manageQtVersions();
    void manageToolChains();
    void importLabelClicked();

    // Changes triggered from creator
    void qtVersionsChanged();
    void qtVersionChanged();
    void buildDirectoryChanged();
    void toolChainChanged();
    void updateImportLabel();
    void environmentChanged();
    void updateToolChainCombo();

private:
    void updateDetails();
    void updateShadowBuildUi();

    Ui::Qt4ProjectConfigWidget *m_ui;
    QAbstractButton *m_browseButton;
    Qt4BuildConfiguration *m_buildConfiguration;
    Utils::DetailsWidget *m_detailsContainer;
    bool m_ignoreChange;
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // QT4PROJECTCONFIGWIDGET_H
