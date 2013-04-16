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
#ifndef VCPROJECTMANAGER_INTERNAL_BROWSEINFORMATIONTOOL_H
#define VCPROJECTMANAGER_INTERNAL_BROWSEINFORMATIONTOOL_H

#include "tool.h"
#include "../../widgets/vcnodewidget.h"

class QComboBox;
class QLineEdit;

namespace VcProjectManager {
namespace Internal {

class BrowseInformationTool : public Tool
{
public:
    typedef QSharedPointer<BrowseInformationTool>   Ptr;

    BrowseInformationTool();
    BrowseInformationTool(const BrowseInformationTool& tool);
    ~BrowseInformationTool();

    QString nodeWidgetName() const;
    VcNodeWidget *createSettingsWidget();
    Tool::Ptr clone() const;

    bool suppressStartupBanner() const;
    bool suppressStartupBannerDefault() const;
    void setSuppressStartupBanner(bool suppress);

    QString outputFile() const;
    QString outputFileDefault() const;
    void setOutputFile(const QString &outFile);
};

class BrowseInformationToolWidget : public VcNodeWidget
{
public:
    explicit BrowseInformationToolWidget(BrowseInformationTool::Ptr browseTool);
    ~BrowseInformationToolWidget();
    void saveData();

private:
    BrowseInformationTool::Ptr m_tool;

    QComboBox *m_suppresStartUpBannerComboBox;
    QLineEdit *m_outputFileLineEdit;
};

} // namespace Internal
} // namespace VcProjectManager

#endif // VCPROJECTMANAGER_INTERNAL_BROWSEINFORMATIONTOOL_H
