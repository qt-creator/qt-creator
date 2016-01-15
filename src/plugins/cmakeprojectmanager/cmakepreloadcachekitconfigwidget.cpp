/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "cmakepreloadcachekitconfigwidget.h"
#include "cmakepreloadcachekitinformation.h"

#include <projectexplorer/kit.h>

#include <QLineEdit>

namespace CMakeProjectManager {
namespace Internal {

CMakePreloadCacheKitConfigWidget::CMakePreloadCacheKitConfigWidget(ProjectExplorer::Kit *k, const ProjectExplorer::KitInformation *ki) :
    ProjectExplorer::KitConfigWidget(k, ki),
    m_lineEdit(new QLineEdit)
{
    refresh();
    m_lineEdit->setToolTip(toolTip());
    connect(m_lineEdit, &QLineEdit::textEdited,
            this, &CMakePreloadCacheKitConfigWidget::preloadFileWasChanged);
}

CMakePreloadCacheKitConfigWidget::~CMakePreloadCacheKitConfigWidget()
{
    delete m_lineEdit;
}

QWidget *CMakePreloadCacheKitConfigWidget::mainWidget() const
{
    return m_lineEdit;
}

QString CMakePreloadCacheKitConfigWidget::displayName() const
{
    return tr("CMake preload file:");
}

QString CMakePreloadCacheKitConfigWidget::toolTip() const
{
    return tr("The preload cache file to use when running cmake on the project.<br>"
              "This setting is ignored when using other build systems.");
}

void CMakePreloadCacheKitConfigWidget::makeReadOnly()
{
    m_lineEdit->setEnabled(false);
}

void CMakePreloadCacheKitConfigWidget::refresh()
{
    if (!m_ignoreChange)
        m_lineEdit->setText(CMakePreloadCacheKitInformation::preloadCacheFile(m_kit).toUserOutput());
}

void CMakePreloadCacheKitConfigWidget::preloadFileWasChanged(const QString &text)
{
    m_ignoreChange = true;
    m_kit->setValue(CMakePreloadCacheKitInformation::id(), text);
    m_ignoreChange = false;
}

} // namespace Internal
} // namespace CMakeProjectManager
