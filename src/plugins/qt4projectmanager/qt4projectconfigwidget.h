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

#ifndef QT4PROJECTCONFIGWIDGET_H
#define QT4PROJECTCONFIGWIDGET_H

#include <projectexplorer/namedwidget.h>

QT_BEGIN_NAMESPACE
class QAbstractButton;
QT_END_NAMESPACE

namespace Utils {
    class DetailsWidget;
}

namespace Qt4ProjectManager {
class Qt4BuildConfiguration;
class Qt4ProFileNode;

namespace Internal {
namespace Ui {
class Qt4ProjectConfigWidget;
}

class Qt4ProjectConfigWidget : public ProjectExplorer::NamedWidget
{
    Q_OBJECT
public:
    Qt4ProjectConfigWidget(Qt4BuildConfiguration *bc);
    ~Qt4ProjectConfigWidget();

private slots:
    // User changes in our widgets
    void shadowBuildClicked(bool checked);
    void onBeforeBeforeShadowBuildDirBrowsed();
    void shadowBuildEdited();

    // Changes triggered from creator
    void buildDirectoryChanged();
    void updateProblemLabel();
    void environmentChanged();

private:
    void updateDetails();
    void setProblemLabel(const QString &text);

    Ui::Qt4ProjectConfigWidget *m_ui;
    QAbstractButton *m_browseButton;
    Qt4BuildConfiguration *m_buildConfiguration;
    Utils::DetailsWidget *m_detailsContainer;
    bool m_ignoreChange;
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // QT4PROJECTCONFIGWIDGET_H
