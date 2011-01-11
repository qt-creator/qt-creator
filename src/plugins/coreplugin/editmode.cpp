/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "editmode.h"
#include "editormanager.h"
#include "coreconstants.h"
#include "modemanager.h"
#include "minisplitter.h"
#include "findplaceholder.h"
#include "outputpane.h"
#include "navigationwidget.h"
#include "rightpane.h"
#include "ieditor.h"
#include "ifile.h"

#include <QtCore/QLatin1String>
#include <QtGui/QHBoxLayout>
#include <QtGui/QWidget>
#include <QtGui/QSplitter>
#include <QtGui/QIcon>

using namespace Core;
using namespace Core::Internal;

EditMode::EditMode(EditorManager *editorManager) :
    m_editorManager(editorManager),
    m_splitter(new MiniSplitter),
    m_rightSplitWidgetLayout(new QVBoxLayout)
{
    setObjectName(QLatin1String("EditMode"));
    m_rightSplitWidgetLayout->setSpacing(0);
    m_rightSplitWidgetLayout->setMargin(0);
    QWidget *rightSplitWidget = new QWidget;
    rightSplitWidget->setLayout(m_rightSplitWidgetLayout);
    m_rightSplitWidgetLayout->insertWidget(0, new Core::EditorManagerPlaceHolder(this));

    MiniSplitter *rightPaneSplitter = new MiniSplitter;
    rightPaneSplitter->insertWidget(0, rightSplitWidget);
    rightPaneSplitter->insertWidget(1, new RightPanePlaceHolder(this));
    rightPaneSplitter->setStretchFactor(0, 1);
    rightPaneSplitter->setStretchFactor(1, 0);

    MiniSplitter *splitter = new MiniSplitter;
    splitter->setOrientation(Qt::Vertical);
    splitter->insertWidget(0, rightPaneSplitter);
    QWidget *outputPane = new Core::OutputPanePlaceHolder(this, splitter);
    outputPane->setObjectName(QLatin1String("EditModeOutputPanePlaceHolder"));
    splitter->insertWidget(1, outputPane);
    splitter->setStretchFactor(0, 3);
    splitter->setStretchFactor(1, 0);

    m_splitter->insertWidget(0, new NavigationWidgetPlaceHolder(this));
    m_splitter->insertWidget(1, splitter);
    m_splitter->setStretchFactor(0, 0);
    m_splitter->setStretchFactor(1, 1);

    ModeManager *modeManager = ModeManager::instance();
    connect(modeManager, SIGNAL(currentModeChanged(Core::IMode*)),
            this, SLOT(grabEditorManager(Core::IMode*)));
    m_splitter->setFocusProxy(m_editorManager);
}

EditMode::~EditMode()
{
    // Make sure the editor manager does not get deleted
    m_editorManager->setParent(0);
    delete m_splitter;
}

QString EditMode::displayName() const
{
    return tr("Edit");
}

QIcon EditMode::icon() const
{
    return QIcon(QLatin1String(":/fancyactionbar/images/mode_Edit.png"));
}

int EditMode::priority() const
{
    return Constants::P_MODE_EDIT;
}

QWidget* EditMode::widget()
{
    return m_splitter;
}

QString EditMode::id() const
{
    return QLatin1String(Constants::MODE_EDIT);
}

QString EditMode::type() const
{
    return QLatin1String(Constants::MODE_EDIT_TYPE);
}

Context EditMode::context() const
{
    static Context contexts(Constants::C_EDIT_MODE,
                            Constants::C_EDITORMANAGER,
                            Constants::C_NAVIGATION_PANE);
    return contexts;
}

void EditMode::grabEditorManager(Core::IMode *mode)
{
    if (mode != this)
        return;

    if (m_editorManager->currentEditor())
        m_editorManager->currentEditor()->widget()->setFocus();
}
