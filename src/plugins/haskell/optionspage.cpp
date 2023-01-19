/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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

#include "optionspage.h"

#include "haskellconstants.h"
#include "haskellmanager.h"

#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>
#include <QWidget>

namespace Haskell {
namespace Internal {

OptionsPage::OptionsPage()
{
    setId(Constants::OPTIONS_GENERAL);
    setDisplayName(tr("General"));
    setCategory("J.Z.Haskell");
    setDisplayCategory(tr("Haskell"));
    setCategoryIcon(Utils::Icon(":/haskell/images/category_haskell.png"));
}

QWidget *OptionsPage::widget()
{
    using namespace Utils;
    if (!m_widget) {
        m_widget = new QWidget;
        auto topLayout = new QVBoxLayout;
        m_widget->setLayout(topLayout);
        auto generalBox = new QGroupBox(tr("General"));
        topLayout->addWidget(generalBox);
        topLayout->addStretch(10);
        auto boxLayout = new QHBoxLayout;
        generalBox->setLayout(boxLayout);
        boxLayout->addWidget(new QLabel(tr("Stack executable:")));
        m_stackPath = new PathChooser();
        m_stackPath->setExpectedKind(PathChooser::ExistingCommand);
        m_stackPath->setPromptDialogTitle(tr("Choose Stack Executable"));
        m_stackPath->setFilePath(HaskellManager::stackExecutable());
        m_stackPath->setCommandVersionArguments({"--version"});
        boxLayout->addWidget(m_stackPath);
    }
    return m_widget;
}

void OptionsPage::apply()
{
    if (!m_widget)
        return;
    HaskellManager::setStackExecutable(m_stackPath->rawFilePath());
}

void OptionsPage::finish()
{
}

} // namespace Internal
} // namespace Haskell
