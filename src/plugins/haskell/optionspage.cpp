// Copyright (c) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "optionspage.h"

#include "haskellconstants.h"
#include "haskellmanager.h"
#include "haskelltr.h"

#include <utils/pathchooser.h>

#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>

using namespace Utils;

namespace Haskell::Internal {

class HaskellOptionsPageWidget : public Core::IOptionsPageWidget
{
public:
    HaskellOptionsPageWidget()
    {
        auto topLayout = new QVBoxLayout;
        setLayout(topLayout);
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

    void apply() final
    {
        HaskellManager::setStackExecutable(m_stackPath->rawFilePath());
    }

    QPointer<Utils::PathChooser> m_stackPath;
};

OptionsPage::OptionsPage()
{
    setId(Constants::OPTIONS_GENERAL);
    setDisplayName(Tr::tr("General"));
    setCategory("J.Z.Haskell");
    setDisplayCategory(Tr::tr("Haskell"));
    setCategoryIconPath(":/haskell/images/settingscategory_haskell.png");
    setWidgetCreator([] { return new HaskellOptionsPageWidget; });
}

} // Haskell::Internal
