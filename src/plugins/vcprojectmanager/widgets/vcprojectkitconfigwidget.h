/**************************************************************************
**
** Copyright (c) 2014 Bojan Petrovic
** Copyright (c) 2014 Radovan Zivkovic
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
#ifndef VCPROJECTMANAGER_INTERNAL_VCPROJECTKITCONFIGWIDGET_H
#define VCPROJECTMANAGER_INTERNAL_VCPROJECTKITCONFIGWIDGET_H

#include <projectexplorer/kitconfigwidget.h>
#include <coreplugin/id.h>

QT_BEGIN_NAMESPACE
class QComboBox;
QT_END_NAMESPACE

namespace VcProjectManager {
namespace Internal {

class MsBuildInformation;

class VcProjectKitConfigWidget : public ProjectExplorer::KitConfigWidget
{
    Q_OBJECT

public:
    VcProjectKitConfigWidget(ProjectExplorer::Kit *k);
    ~VcProjectKitConfigWidget();

    QString displayName() const;
    QString toolTip() const { return QString(); }
    void makeReadOnly();
    void refresh();
    bool visibleInKit();

    QWidget *mainWidget() const;
    QWidget *buttonWidget() const { return 0; }

private slots:
    void currentMsBuildChanged(int index);
    void onMsBuildAdded(Core::Id msBuildId);
    void onMsBuildReplaced(Core::Id oldMsBuildId, Core::Id newMsBuildId);
    void onMsBuildRemoved(Core::Id msBuildId);

private:
    int indexOf(MsBuildInformation *msBuild) const;
    void insertMsBuild(MsBuildInformation *msBuild);
    void removeMsBuild(MsBuildInformation *msBuild);

    QComboBox *m_comboBox;
};

} // namespace Internal
} // namespace VcProjectManager

#endif // VCPROJECTMANAGER_INTERNAL_VCPROJECTKITCONFIGWIDGET_H
