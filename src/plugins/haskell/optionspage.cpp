// Copyright (c) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "optionspage.h"

#include "haskellconstants.h"
#include "haskellmanager.h"
#include "haskelltr.h"

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
    setDisplayName(Tr::tr("General"));
    setCategory("J.Z.Haskell");
    setDisplayCategory(Tr::tr("Haskell"));
    setCategoryIcon(Utils::Icon(":/haskell/images/category_haskell.png"));
}

QWidget *OptionsPage::widget()
{
    using namespace Utils;
    if (!m_widget) {
        m_widget = new QWidget;
        auto topLayout = new QVBoxLayout;
        m_widget->setLayout(topLayout);
        auto generalBox = new QGroupBox(Tr::tr("General"));
        topLayout->addWidget(generalBox);
        topLayout->addStretch(10);
        auto boxLayout = new QHBoxLayout;
        generalBox->setLayout(boxLayout);
        boxLayout->addWidget(new QLabel(Tr::tr("Stack executable:")));
        m_stackPath = new PathChooser();
        m_stackPath->setExpectedKind(PathChooser::ExistingCommand);
        m_stackPath->setPromptDialogTitle(Tr::tr("Choose Stack Executable"));
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
