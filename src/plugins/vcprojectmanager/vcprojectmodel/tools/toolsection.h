/**************************************************************************
**
** Copyright (c) 2013 Bojan Petrovic
** Copyright (c) 2013 Radovan Zivkovic
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
#ifndef VCPROJECTMANAGER_INTERNAL_TOOLSECTION_H
#define VCPROJECTMANAGER_INTERNAL_TOOLSECTION_H

#include <QList>
#include "../../interfaces/itoolsection.h"

class QWidget;

namespace VcProjectManager {
namespace Internal {

class ToolSectionDescription;
class IToolAttribute;
class ToolSectionSettingsWidget;
class GeneralToolAttributeContainer;

class ToolSection : public IToolSection
{
public:
    ToolSection(const ToolSectionDescription *toolSectionDesc);
    ToolSection(const ToolSection &toolSec);
    ~ToolSection();

    IToolAttributeContainer *attributeContainer() const;
    const IToolSectionDescription *sectionDescription() const;
    VcNodeWidget* createSettingsWidget();
    IToolSection* clone() const;

private:
    const ToolSectionDescription * m_toolDesc;
    GeneralToolAttributeContainer *m_attributeContainer;
};

} // namespace Internal
} // namespace VcProjectManager

#endif // VCPROJECTMANAGER_INTERNAL_TOOLSECTION_H
