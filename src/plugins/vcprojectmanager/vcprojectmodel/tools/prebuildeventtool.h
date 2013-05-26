/**************************************************************************
**
** Copyright (c) 2013 Bojan Petrovic
** Copyright (c) 2013 Radovan Zivkvoic
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
#ifndef VCPROJECTMANAGER_INTERNAL_PREBUILDEVENTTOOL_H
#define VCPROJECTMANAGER_INTERNAL_PREBUILDEVENTTOOL_H

#include "tool.h"
#include "../../widgets/vcnodewidget.h"

class QLineEdit;
class QComboBox;

namespace VcProjectManager {
namespace Internal {

class LineEdit;

class PreBuildEventTool : public Tool
{
public:
    typedef QSharedPointer<PreBuildEventTool>   Ptr;

    PreBuildEventTool();
    PreBuildEventTool(const PreBuildEventTool &tool);
    ~PreBuildEventTool();

    QString nodeWidgetName() const;
    VcNodeWidget* createSettingsWidget();
    Tool::Ptr clone() const;

    QStringList commandLine() const;
    QStringList commandLineDefault() const;
    void setCommandLine(const QStringList &commandList);
    QString description() const;
    QString descriptionDefault() const;
    void setDesription(const QString &desc);
    bool excludedFromBuild() const;
    bool excludedFromBuildDefault() const;
    void setExcludedFromBuild(bool excludedFromBuild);
};

class PreBuildEventToolWidget : public VcNodeWidget
{
public:
    explicit PreBuildEventToolWidget(PreBuildEventTool* tool);
    ~PreBuildEventToolWidget();
    void saveData();

private:
    PreBuildEventTool *m_tool;

    LineEdit *m_cmdLineLineEdit;
    QLineEdit *m_descriptionLineEdit;
    QComboBox *m_excludedFromBuildComboBox;
};

} // namespace Internal
} // namespace VcProjectManager

#endif // VCPROJECTMANAGER_INTERNAL_PREBUILDEVENTTOOL_H
