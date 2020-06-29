/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#include "transitioneditorwidget.h"

#include "transitioneditorgraphicsscene.h"
#include "transitioneditorpropertyitem.h"
#include "transitioneditortoolbar.h"
#include "transitioneditorview.h"

#include <timelineeditor/easingcurvedialog.h>
#include <timelineeditor/timelineconstants.h>
#include <timelineeditor/timelineicons.h>

#include <bindingproperty.h>
#include <nodeabstractproperty.h>
#include <nodemetainfo.h>

#include <qmldesignerplugin.h>
#include <qmlstate.h>
#include <qmltimeline.h>

#include <coreplugin/icore.h>

#include <theme.h>
#include <utils/algorithm.h>
#include <utils/fileutils.h>

#include <QApplication>
#include <QComboBox>
#include <QGraphicsView>
#include <QHBoxLayout>
#include <QLabel>
#include <QMargins>
#include <QPushButton>
#include <QResizeEvent>
#include <QScrollBar>
#include <QShowEvent>
#include <QSlider>
#include <QSpacerItem>
#include <QVBoxLayout>
#include <QtGlobal>

namespace QmlDesigner {

class Eventfilter : public QObject
{
public:
    Eventfilter(QObject *parent)
        : QObject(parent)
    {}

    bool eventFilter(QObject *, QEvent *event) override
    {
        if (event->type() == QEvent::Wheel) {
            event->accept();
            return true;
        }
        return false;
    }
};

TransitionEditorWidget::TransitionEditorWidget(TransitionEditorView *view)
    : QWidget()
    , m_toolbar(new TransitionEditorToolBar(this))
    , m_rulerView(new QGraphicsView(this))
    , m_graphicsView(new QGraphicsView(this))
    , m_scrollbar(new QScrollBar(this))
    , m_statusBar(new QLabel(this))
    , m_transitionEditorView(view)
    , m_graphicsScene(new TransitionEditorGraphicsScene(this))
    , m_addButton(new QPushButton(this))
    , m_onboardingContainer(new QWidget(this))
{
    setWindowTitle(tr("Transition", "Title of transition view"));
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    const QString css = Theme::replaceCssColors(QString::fromUtf8(
        Utils::FileReader::fetchQrc(QLatin1String(":/qmldesigner/scrollbar.css"))));

    m_scrollbar->setStyleSheet(css);
    m_scrollbar->setOrientation(Qt::Horizontal);

    QSizePolicy sizePolicy1(QSizePolicy::Expanding, QSizePolicy::Preferred);
    sizePolicy1.setHorizontalStretch(0);
    sizePolicy1.setVerticalStretch(0);
    sizePolicy1.setHeightForWidth(m_graphicsView->sizePolicy().hasHeightForWidth());

    m_rulerView->setObjectName("RulerView");
    m_rulerView->setFixedHeight(TimelineConstants::rulerHeight);
    m_rulerView->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    m_rulerView->viewport()->installEventFilter(new Eventfilter(this));
    m_rulerView->viewport()->setFocusPolicy(Qt::NoFocus);
    m_rulerView->setStyleSheet(css);
    m_rulerView->setFrameShape(QFrame::NoFrame);
    m_rulerView->setFrameShadow(QFrame::Plain);
    m_rulerView->setLineWidth(0);
    m_rulerView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_rulerView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_rulerView->setScene(graphicsScene());

    m_graphicsView->setStyleSheet(css);
    m_graphicsView->setObjectName("SceneView");
    m_graphicsView->setFrameShape(QFrame::NoFrame);
    m_graphicsView->setFrameShadow(QFrame::Plain);
    m_graphicsView->setLineWidth(0);
    m_graphicsView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    m_graphicsView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    m_graphicsView->setSizePolicy(sizePolicy1);
    m_graphicsView->setScene(graphicsScene());
    m_graphicsView->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    m_graphicsView->setViewportUpdateMode(QGraphicsView::FullViewportUpdate);

    auto *scrollBarLayout = new QHBoxLayout;
    scrollBarLayout->addSpacing(TimelineConstants::sectionWidth);
    scrollBarLayout->addWidget(m_scrollbar);

    QMargins margins(0, 0, 0, QApplication::style()->pixelMetric(QStyle::PM_LayoutBottomMargin));

    auto *contentLayout = new QVBoxLayout;
    contentLayout->setContentsMargins(margins);
    contentLayout->addWidget(m_rulerView);
    contentLayout->addWidget(m_graphicsView);
    contentLayout->addLayout(scrollBarLayout);
    contentLayout->addWidget(m_statusBar);
    m_statusBar->setIndent(2);
    m_statusBar->setFixedHeight(TimelineConstants::rulerHeight);

    auto *widgetLayout = new QVBoxLayout;
    widgetLayout->setContentsMargins(0, 0, 0, 0);
    widgetLayout->setSpacing(0);
    widgetLayout->addWidget(m_toolbar);
    widgetLayout->addWidget(m_addButton);

    m_addButton->setIcon(TimelineIcons::ADD_TIMELINE_TOOLBAR.icon());
    m_addButton->setToolTip(tr("Add Transition"));
    m_addButton->setFlat(true);
    m_addButton->setFixedSize(32, 32);

    widgetLayout->addWidget(m_onboardingContainer);

    auto *onboardingTopLabel = new QLabel(m_onboardingContainer);
    auto *onboardingBottomLabel = new QLabel(m_onboardingContainer);
    auto *onboardingBottomIcon = new QLabel(m_onboardingContainer);

    auto *onboardingLayout = new QVBoxLayout;
    auto *onboardingSublayout = new QHBoxLayout;
    auto *leftSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
    auto *rightSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
    auto *topSpacer = new QSpacerItem(40, 20, QSizePolicy::Minimum, QSizePolicy::Expanding);
    auto *bottomSpacer = new QSpacerItem(40, 20, QSizePolicy::Minimum, QSizePolicy::Expanding);

    QString labelText = tr("This file does not contain transitions. <br><br> \
            To create an animation, add a transition by clicking the + button.");
    onboardingTopLabel->setText(labelText);
    onboardingTopLabel->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);

    m_onboardingContainer->setLayout(onboardingLayout);
    onboardingLayout->setContentsMargins(0, 0, 0, 0);
    onboardingLayout->setSpacing(0);
    onboardingLayout->addSpacerItem(topSpacer);
    onboardingLayout->addWidget(onboardingTopLabel);
    onboardingLayout->addLayout(onboardingSublayout);

    onboardingSublayout->setContentsMargins(0, 0, 0, 0);
    onboardingSublayout->setSpacing(0);
    onboardingSublayout->addSpacerItem(leftSpacer);

    onboardingBottomLabel->setAlignment(Qt::AlignRight | Qt::AlignTop);
    onboardingBottomLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    onboardingSublayout->addWidget(onboardingBottomLabel);
    onboardingBottomLabel->setText(tr("To edit the transition settings, click "));

    onboardingBottomIcon->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    onboardingBottomIcon->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    onboardingSublayout->addWidget(onboardingBottomIcon);
    onboardingBottomIcon->setPixmap(TimelineIcons::ANIMATION.pixmap());

    onboardingSublayout->addSpacerItem(rightSpacer);
    onboardingLayout->addSpacerItem(bottomSpacer);

    widgetLayout->addLayout(contentLayout);
    this->setLayout(widgetLayout);

    connectToolbar();

    auto setScrollOffset = [this]() { graphicsScene()->setScrollOffset(m_scrollbar->value()); };
    connect(m_scrollbar, &QSlider::valueChanged, this, setScrollOffset);

    connect(graphicsScene(),
            &TransitionEditorGraphicsScene::statusBarMessageChanged,
            this,
            [this](const QString &message) { m_statusBar->setText(message); });

    connect(m_addButton, &QPushButton::clicked, this, [this]() {
        m_transitionEditorView->addNewTransition();
    });
}

void TransitionEditorWidget::setTransitionActive(bool b)
{
    if (b) {
        m_toolbar->setVisible(true);
        m_graphicsView->setVisible(true);
        m_rulerView->setVisible(true);
        m_scrollbar->setVisible(true);
        m_addButton->setVisible(false);
        m_onboardingContainer->setVisible(false);
        m_graphicsView->update();
        m_rulerView->update();
    } else {
        m_toolbar->setVisible(false);
        m_graphicsView->setVisible(false);
        m_rulerView->setVisible(false);
        m_scrollbar->setVisible(false);
        m_addButton->setVisible(true);
        m_onboardingContainer->setVisible(true);
    }
}

void TransitionEditorWidget::connectToolbar()
{
    connect(graphicsScene(),
            &TransitionEditorGraphicsScene::selectionChanged,
            this,
            &TransitionEditorWidget::selectionChanged);

    connect(m_toolbar,
            &TransitionEditorToolBar::openEasingCurveEditor,
            this,
            &TransitionEditorWidget::openEasingCurveEditor);

    connect(graphicsScene(),
            &TransitionEditorGraphicsScene::scroll,
            this,
            &TransitionEditorWidget::scroll);

    auto setRulerScaling = [this](int val) { m_graphicsScene->setRulerScaling(val); };
    connect(m_toolbar, &TransitionEditorToolBar::scaleFactorChanged, setRulerScaling);

    auto setDuration = [this](int end) { graphicsScene()->setDuration(end); };
    connect(m_toolbar, &TransitionEditorToolBar::durationChanged, setDuration);

    connect(m_toolbar,
            &TransitionEditorToolBar::settingDialogClicked,
            transitionEditorView(),
            &TransitionEditorView::openSettingsDialog);

    connect(m_toolbar,
            &TransitionEditorToolBar::currentTransitionChanged,
            this,
            [this](const QString &transitionName) {
                const ModelNode transition = transitionEditorView()->modelNodeForId(transitionName);
                if (transition.isValid()) {
                    m_graphicsScene->setTransition(transition);
                }
            });
}

void TransitionEditorWidget::changeScaleFactor(int factor)
{
    m_toolbar->setScaleFactor(factor);
}

void TransitionEditorWidget::scroll(const TimelineUtils::Side &side)
{
    if (side == TimelineUtils::Side::Left)
        m_scrollbar->setValue(m_scrollbar->value() - m_scrollbar->singleStep());
    else if (side == TimelineUtils::Side::Right)
        m_scrollbar->setValue(m_scrollbar->value() + m_scrollbar->singleStep());
}

void TransitionEditorWidget::selectionChanged()
{
    if (graphicsScene()->selectedPropertyItem() != nullptr)
        m_toolbar->setActionEnabled("Curve Picker", true);
    else
        m_toolbar->setActionEnabled("Curve Picker", false);
}

void TransitionEditorWidget::contextHelp(const Core::IContext::HelpCallback &callback) const
{
    if (transitionEditorView())
        transitionEditorView()->contextHelp(callback);
    else
        callback({});
}

void TransitionEditorWidget::init()
{
    ModelNode root = transitionEditorView()->rootModelNode();
    ModelNode transition;

    if (root.isValid() && root.hasProperty("transitions")) {
        NodeAbstractProperty transitions = root.nodeAbstractProperty("transitions");
        if (transitions.isValid())
            transition = transitions.directSubNodes().first();
    }

    m_graphicsScene->setTransition(transition);
    setTransitionActive(transition.isValid());

    m_graphicsScene->setWidth(m_graphicsView->viewport()->width());

    m_toolbar->setScaleFactor(0);

    m_toolbar->setCurrentTransition(transition);

    qreal duration = 2000;
    if (transition.isValid() && transition.hasAuxiliaryData("transitionDuration"))
        duration = transition.auxiliaryData("transitionDuration").toDouble();

    m_toolbar->setDuration(duration);

    m_graphicsScene->setRulerScaling(0);
}

void TransitionEditorWidget::updateData(const ModelNode &transition)
{
    if (!transition.isValid()) {
        init();
        return;
    }

    if (transition.metaInfo().isValid()
        && transition.metaInfo().isSubclassOf("QtQuick.Transition")) {
        if (transition.id() == m_toolbar->currentTransitionId()) {
            m_graphicsScene->setTransition(transition);
        } else {
            m_toolbar->updateComboBox(transition.view()->rootModelNode());
        }
    }
}

void TransitionEditorWidget::reset()
{
    graphicsScene()->clearTransition();
    m_toolbar->reset();
    m_statusBar->clear();
}

TransitionEditorGraphicsScene *TransitionEditorWidget::graphicsScene() const
{
    return m_graphicsScene;
}

TransitionEditorToolBar *TransitionEditorWidget::toolBar() const
{
    return m_toolbar;
}

void TransitionEditorWidget::setupScrollbar(int min, int max, int current)
{
    bool b = m_scrollbar->blockSignals(true);
    m_scrollbar->setMinimum(min);
    m_scrollbar->setMaximum(max);
    m_scrollbar->setValue(current);
    m_scrollbar->setSingleStep((max - min) / 10);
    m_scrollbar->blockSignals(b);
}

void TransitionEditorWidget::showEvent(QShowEvent *event)
{
    Q_UNUSED(event)
    graphicsScene()->setWidth(m_graphicsView->viewport()->width());
    graphicsScene()->invalidateLayout();
    graphicsScene()->invalidate();
    graphicsScene()->onShow();
}

void TransitionEditorWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    graphicsScene()->setWidth(m_graphicsView->viewport()->width());
}

TransitionEditorView *TransitionEditorWidget::transitionEditorView() const
{
    return m_transitionEditorView;
}

void TransitionEditorWidget::openEasingCurveEditor()
{
    if (TransitionEditorPropertyItem *item = graphicsScene()->selectedPropertyItem()) {
        QList<ModelNode> animations;
        animations.append(item->propertyAnimation());
        EasingCurveDialog::runDialog(animations, Core::ICore::dialogParent());
    }
}

} // namespace QmlDesigner
