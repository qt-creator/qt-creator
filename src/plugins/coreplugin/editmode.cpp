// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "editmode.h"

#include "coreconstants.h"
#include "coreicons.h"
#include "coreplugintr.h"
#include "editormanager/editormanager.h"
#include "editormanager/ieditor.h"
#include "icore.h"
#include "minisplitter.h"
#include "modemanager.h"
#include "navigationwidget.h"
#include "outputpane.h"
#include "rightpane.h"

#include <QVBoxLayout>
#include <QWidget>
#include <QIcon>

namespace Core::Internal {

class EditModeWidget final : public MiniSplitter
{
public:
    EditModeWidget()
    {
        auto editorPlaceHolder = new EditorManagerPlaceHolder;

        QWidget *rightSplitWidget = new QWidget;
        auto rightSplitWidgetLayout = new QVBoxLayout(rightSplitWidget);
        rightSplitWidgetLayout->setSpacing(0);
        rightSplitWidgetLayout->setContentsMargins(0, 0, 0, 0);
        rightSplitWidgetLayout->insertWidget(0, editorPlaceHolder);

        auto rightPaneSplitter = new MiniSplitter;
        rightPaneSplitter->insertWidget(0, rightSplitWidget);
        rightPaneSplitter->insertWidget(1, new RightPanePlaceHolder(Constants::MODE_EDIT));
        rightPaneSplitter->setStretchFactor(0, 1);
        rightPaneSplitter->setStretchFactor(1, 0);

        auto splitter = new MiniSplitter;
        splitter->setOrientation(Qt::Vertical);
        splitter->insertWidget(0, rightPaneSplitter);
        QWidget *outputPane = new OutputPanePlaceHolder(Constants::MODE_EDIT, splitter);
        outputPane->setObjectName(QLatin1String("EditModeOutputPanePlaceHolder"));
        splitter->insertWidget(1, outputPane);
        splitter->setStretchFactor(0, 3);
        splitter->setStretchFactor(1, 0);

        insertWidget(0, new NavigationWidgetPlaceHolder(Constants::MODE_EDIT, Side::Left));
        insertWidget(1, splitter);
        insertWidget(2, new NavigationWidgetPlaceHolder(Constants::MODE_EDIT, Side::Right));
        setStretchFactor(0, 0);
        setStretchFactor(1, 1);
        setStretchFactor(2, 0);

        setFocusProxy(editorPlaceHolder);

        IContext::attach(this, Context(Constants::C_EDITORMANAGER));
    }
};

EditMode::EditMode()
{
    setObjectName(QLatin1String("EditMode"));
    setDisplayName(Tr::tr("Edit"));
    setIcon(Utils::Icon::modeIcon(Icons::MODE_EDIT_CLASSIC,
                                  Icons::MODE_EDIT_FLAT, Icons::MODE_EDIT_FLAT_ACTIVE));
    setPriority(Constants::P_MODE_EDIT);
    setId(Constants::MODE_EDIT);

    connect(ModeManager::instance(), &ModeManager::currentModeChanged,
            this, &EditMode::grabEditorManager);

    setWidget(new EditModeWidget);
    setContext(Context(Constants::C_EDIT_MODE, Constants::C_NAVIGATION_PANE));
}

EditMode::~EditMode()
{
    delete widget();
}

void EditMode::grabEditorManager(Utils::Id mode)
{
    if (mode != id())
        return;

    if (EditorManager::currentEditor())
        EditorManager::currentEditor()->widget()->setFocus();
}

} // namespace Core::Internal
