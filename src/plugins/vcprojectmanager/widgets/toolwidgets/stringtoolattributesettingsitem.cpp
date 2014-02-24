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
#include "stringtoolattributesettingsitem.h"
#include "../lineedit.h"
#include "../../vcprojectmodel/tools/toolattributes/stringtoolattribute.h"

#include <QVBoxLayout>

namespace VcProjectManager {
namespace Internal {

StringToolAttributeSettingsItem::StringToolAttributeSettingsItem(StringToolAttribute *toolAttribute)
    : m_toolAttribute(toolAttribute)
{
    m_lineEdit = new LineEdit;
    m_lineEdit->setText(toolAttribute->value());
    QVBoxLayout *layout = new QVBoxLayout;
    layout->setMargin(0);
    layout->addWidget(m_lineEdit);
    setLayout(layout);
}

StringToolAttributeSettingsItem::~StringToolAttributeSettingsItem()
{
    m_lineEdit->deleteLater();
}

void StringToolAttributeSettingsItem::saveData()
{
    m_toolAttribute->setValue(m_lineEdit->text().trimmed());
}

} // namespace Internal
} // namespace VcProjectManager
