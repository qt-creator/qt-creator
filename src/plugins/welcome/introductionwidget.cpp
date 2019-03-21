/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include "introductionwidget.h"

#include <utils/algorithm.h>
#include <utils/checkablemessagebox.h>
#include <utils/qtcassert.h>
#include <utils/stylehelper.h>

#include <QEvent>
#include <QGuiApplication>
#include <QKeyEvent>
#include <QLabel>
#include <QPainter>
#include <QPushButton>
#include <QVBoxLayout>

using namespace Utils;

const char kTakeTourSetting[] = "TakeUITour";

namespace Welcome {
namespace Internal {

void IntroductionWidget::askUserAboutIntroduction(QWidget *parent, QSettings *settings)
{
    if (!CheckableMessageBox::shouldAskAgain(settings, kTakeTourSetting))
        return;
    auto messageBox = new CheckableMessageBox(parent);
    messageBox->setWindowTitle(tr("Take a UI Tour"));
    messageBox->setText(
        tr("Do you want to take a quick UI tour? This shows where the most important user "
           "interface elements are, and how they are used, and will only take a minute. You can "
           "also take the tour later by selecting Help > UI Tour."));
    messageBox->setCheckBoxVisible(true);
    messageBox->setCheckBoxText(CheckableMessageBox::msgDoNotAskAgain());
    messageBox->setChecked(true);
    messageBox->setStandardButtons(QDialogButtonBox::Cancel);
    QPushButton *tourButton = messageBox->addButton(tr("Take UI Tour"), QDialogButtonBox::AcceptRole);
    connect(messageBox, &QDialog::finished, parent, [parent, settings, messageBox, tourButton]() {
        if (messageBox->isChecked())
            CheckableMessageBox::doNotAskAgain(settings, kTakeTourSetting);
        if (messageBox->clickedButton() == tourButton) {
            auto intro = new IntroductionWidget(parent);
            intro->show();
        }
        messageBox->deleteLater();
    });
    messageBox->show();
}

IntroductionWidget::IntroductionWidget(QWidget *parent)
    : QWidget(parent),
      m_borderImage(":/welcome/images/border.png")
{
    setFocusPolicy(Qt::StrongFocus);
    setFocus();
    parent->installEventFilter(this);

    QPalette p = palette();
    p.setColor(QPalette::Foreground, QColor(220, 220, 220));
    setPalette(p);

    m_textWidget = new QWidget(this);
    auto layout = new QVBoxLayout;
    m_textWidget->setLayout(layout);

    m_stepText = new QLabel(this);
    m_stepText->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_stepText->setWordWrap(true);
    m_stepText->setTextFormat(Qt::RichText);
    // why is palette not inherited???
    m_stepText->setPalette(palette());
    m_stepText->setOpenExternalLinks(true);
    m_stepText->installEventFilter(this);
    layout->addWidget(m_stepText);

    m_continueLabel = new QLabel(this);
    m_continueLabel->setAlignment(Qt::AlignCenter);
    m_continueLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    m_continueLabel->setWordWrap(true);
    auto fnt = font();
    fnt.setPointSizeF(fnt.pointSizeF() * 1.5);
    m_continueLabel->setFont(fnt);
    m_continueLabel->setPalette(palette());
    layout->addWidget(m_continueLabel);
    m_bodyCss = "font-size: 16px;";
    m_items = {
        {QLatin1String("ModeSelector"),
         tr("Mode Selector"),
         tr("Select different modes depending on the task at hand."),
         tr("<p style=\"margin-top: 30px\"><table>"
            "<tr><td style=\"padding-right: 20px\">Welcome:</td><td>Open examples, tutorials, and "
            "recent sessions and projects.</td></tr>"
            "<tr><td>Edit:</td><td>Work with code and navigate your project.</td></tr>"
            "<tr><td>Design:</td><td>Work with UI designs for Qt Widgets or Qt Quick.</td></tr>"
            "<tr><td>Debug:</td><td>Analyze your application with a debugger or other "
            "analyzers.</td></tr>"
            "<tr><td>Projects:</td><td>Manage project settings.</td></tr>"
            "<tr><td>Help:</td><td>Browse the help database.</td></tr>"
            "</table></p>")},
        {QLatin1String("KitSelector.Button"),
         tr("Kit Selector"),
         tr("Select the active project or project configuration."),
         {}},
        {QLatin1String("Run.Button"),
         tr("Run Button"),
         tr("Run the active project. By default this builds the project first."),
         {}},
        {QLatin1String("Debug.Button"),
         tr("Debug Button"),
         tr("Run the active project in a debugger."),
         {}},
        {QLatin1String("Build.Button"), tr("Build Button"), tr("Build the active project."), {}},
        {QLatin1String("LocatorInput"),
         tr("Locator"),
         tr("Type here to open a file from any open project."),
         tr("Or:"
            "<ul>"
            "<li>type <code>c&lt;space&gt;&lt;pattern&gt;</code> to jump to a class definition</li>"
            "<li>type <code>f&lt;space&gt;&lt;pattern&gt;</code> to open a file from the file "
            "system</li>"
            "<li>click on the magnifier icon for a complete list of possible options</li>"
            "</ul>")},
        {QLatin1String("OutputPaneButtons"),
         tr("Output Panes"),
         tr("Find compile and application output here, "
            "as well as a list of configuration and build issues, "
            "and the panel for global searches."),
         {}},
        {QLatin1String("ProgressInfo"),
         tr("Progress Indicator"),
         tr("Progress information about running tasks is shown here."),
         {}},
        {{},
         tr("Escape to Editor"),
         tr("Pressing the Escape key brings you back to the editor. Press it "
            "multiple times to also hide output panes and context help, giving the editor more "
            "space."),
         {}},
        {{},
         tr("The End"),
         tr("You have now completed the UI tour. To learn more about the highlighted "
            "controls, see <a style=\"color: #41CD52\" "
            "href=\"qthelp://org.qt-project.qtcreator/doc/creator-quick-tour.html\">User "
            "Interface</a>."),
         {}}};
    setStep(0);
    resizeToParent();
}

bool IntroductionWidget::event(QEvent *e)
{
    if (e->type() == QEvent::ShortcutOverride) {
        e->accept();
        return true;
    }
    return QWidget::event(e);
}

bool IntroductionWidget::eventFilter(QObject *obj, QEvent *ev)
{
    if (obj == parent() && ev->type() == QEvent::Resize)
        resizeToParent();
    else if (obj == m_stepText && ev->type() == QEvent::MouseButtonRelease)
        step();
    return QWidget::eventFilter(obj, ev);
}

const int SPOTLIGHTMARGIN = 18;
const int POINTER_SIZE = 30;
const int POINTER_WIDTH = 3;

static int margin(const QRect &small, const QRect &big, Qt::Alignment side)
{
    switch (side) {
    case Qt::AlignRight:
        return qMax(0, big.right() - small.right());
    case Qt::AlignTop:
        return qMax(0, small.top() - big.top());
    case Qt::AlignBottom:
        return qMax(0, big.bottom() - small.bottom());
    case Qt::AlignLeft:
        return qMax(0, small.x() - big.x());
    default:
        QTC_ASSERT(false, return 0);
    }
}

static int oppositeMargin(const QRect &small, const QRect &big, Qt::Alignment side)
{
    switch (side) {
    case Qt::AlignRight:
        return margin(small, big, Qt::AlignLeft);
    case Qt::AlignTop:
        return margin(small, big, Qt::AlignBottom);
    case Qt::AlignBottom:
        return margin(small, big, Qt::AlignTop);
    case Qt::AlignLeft:
        return margin(small, big, Qt::AlignRight);
    default:
        QTC_ASSERT(false, return 100000);
    }
}

static const QVector<QPolygonF> pointerPolygon(const QRect &anchorRect, const QRect &fullRect)
{
    // Put the arrow opposite to the smallest margin,
    // with priority right, top, bottom, left.
    // Not very sophisticated but sufficient for current use cases.
    QVector<Qt::Alignment> alignments{Qt::AlignRight, Qt::AlignTop, Qt::AlignBottom, Qt::AlignLeft};
    Utils::sort(alignments, [anchorRect, fullRect](Qt::Alignment a, Qt::Alignment b) {
        return oppositeMargin(anchorRect, fullRect, a) < oppositeMargin(anchorRect, fullRect, b);
    });
    const auto alignIt = std::find_if(alignments.cbegin(),
                                      alignments.cend(),
                                      [anchorRect, fullRect](Qt::Alignment align) {
                                          return margin(anchorRect, fullRect, align) > POINTER_SIZE;
                                      });
    if (alignIt == alignments.cend())
        return {{}}; // no side with sufficient space found
    const qreal arrowHeadWidth = POINTER_SIZE/3.;
    if (*alignIt == Qt::AlignRight) {
        const qreal middleY = anchorRect.center().y();
        const qreal startX = anchorRect.right() + POINTER_SIZE;
        const qreal endX = anchorRect.right() + POINTER_WIDTH;
        return {{QVector<QPointF>{{startX, middleY}, {endX, middleY}}},
                QVector<QPointF>{{endX + arrowHeadWidth, middleY - arrowHeadWidth},
                                 {endX, middleY},
                                 {endX + arrowHeadWidth, middleY + arrowHeadWidth}}};
    }
    if (*alignIt == Qt::AlignTop) {
        const qreal middleX = anchorRect.center().x();
        const qreal startY = anchorRect.y() - POINTER_SIZE;
        const qreal endY = anchorRect.y() - POINTER_WIDTH;
        return {{QVector<QPointF>{{middleX, startY}, {middleX, endY}}},
                QVector<QPointF>{{middleX - arrowHeadWidth, endY - arrowHeadWidth},
                                 {middleX, endY},
                                 {middleX + arrowHeadWidth, endY - arrowHeadWidth}}};
    }
    if (*alignIt == Qt::AlignBottom) {
        const qreal middleX = anchorRect.center().x();
        const qreal startY = anchorRect.y() + POINTER_WIDTH;
        const qreal endY = anchorRect.y() + POINTER_SIZE;
        return {{QVector<QPointF>{{middleX, startY}, {middleX, endY}}},
                QVector<QPointF>{{middleX - arrowHeadWidth, endY + arrowHeadWidth},
                                 {middleX, endY},
                                 {middleX + arrowHeadWidth, endY + arrowHeadWidth}}};
    }

    // Qt::AlignLeft
    const qreal middleY = anchorRect.center().y();
    const qreal startX = anchorRect.x() - POINTER_WIDTH;
    const qreal endX = anchorRect.x() - POINTER_SIZE;
    return {{QVector<QPointF>{{startX, middleY}, {endX, middleY}}},
            QVector<QPointF>{{endX - arrowHeadWidth, middleY - arrowHeadWidth},
                             {endX, middleY},
                             {endX - arrowHeadWidth, middleY + arrowHeadWidth}}};
}

void IntroductionWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setOpacity(.87);
    const QColor backgroundColor = Qt::black;
    if (m_stepPointerAnchor) {
        const QPoint anchorPos = m_stepPointerAnchor->mapTo(parentWidget(), {0, 0});
        const QRect anchorRect(anchorPos, m_stepPointerAnchor->size());
        const QRect spotlightRect = anchorRect.adjusted(-SPOTLIGHTMARGIN,
                                                        -SPOTLIGHTMARGIN,
                                                        SPOTLIGHTMARGIN,
                                                        SPOTLIGHTMARGIN);

        // darken the background to create a spotlighted area
        if (spotlightRect.left() > 0) {
            p.fillRect(0, 0, spotlightRect.left(), height(), backgroundColor);
        }
        if (spotlightRect.top() > 0) {
            p.fillRect(spotlightRect.left(),
                       0,
                       width() - spotlightRect.left(),
                       spotlightRect.top(),
                       backgroundColor);
        }
        if (spotlightRect.right() < width() - 1) {
            p.fillRect(spotlightRect.right() + 1,
                       spotlightRect.top(),
                       width() - spotlightRect.right() - 1,
                       height() - spotlightRect.top(),
                       backgroundColor);
        }
        if (spotlightRect.bottom() < height() - 1) {
            p.fillRect(spotlightRect.left(),
                       spotlightRect.bottom() + 1,
                       spotlightRect.width(),
                       height() - spotlightRect.bottom() - 1,
                       backgroundColor);
        }

        // smooth borders of the spotlighted area by gradients
        StyleHelper::drawCornerImage(m_borderImage,
                                     &p,
                                     spotlightRect,
                                     SPOTLIGHTMARGIN,
                                     SPOTLIGHTMARGIN,
                                     SPOTLIGHTMARGIN,
                                     SPOTLIGHTMARGIN);

        // draw pointer
        const QColor qtGreen(65, 205, 82);
        p.setOpacity(1.);
        p.setPen(QPen(QBrush(qtGreen),
                      POINTER_WIDTH,
                      Qt::SolidLine,
                      Qt::RoundCap,
                      Qt::MiterJoin));
        p.setRenderHint(QPainter::Antialiasing);
        for (const QPolygonF &poly : pointerPolygon(spotlightRect, rect()))
            p.drawPolyline(poly);
    } else {
        p.fillRect(rect(), backgroundColor);
    }
}

void IntroductionWidget::keyPressEvent(QKeyEvent *ke)
{
    if (ke->key() == Qt::Key_Escape)
        finish();
    else if ((ke->modifiers()
              & (Qt::ControlModifier | Qt::AltModifier | Qt::ShiftModifier | Qt::MetaModifier))
             == Qt::NoModifier) {
        const Qt::Key backKey = QGuiApplication::isLeftToRight() ? Qt::Key_Left : Qt::Key_Right;
        if (ke->key() == backKey) {
            if (m_step > 0)
                setStep(m_step - 1);
        } else {
            step();
        }
    }
}

void IntroductionWidget::mouseReleaseEvent(QMouseEvent *me)
{
    me->accept();
    step();
}

void IntroductionWidget::finish()
{
    hide();
    deleteLater();
}

void IntroductionWidget::step()
{
    if (m_step >= m_items.size() - 1)
        finish();
    else
        setStep(m_step + 1);
}

void IntroductionWidget::setStep(uint index)
{
    QTC_ASSERT(index < m_items.size(), return);
    m_step = index;
    m_continueLabel->setText(tr("UI Introduction %1/%2 >").arg(m_step + 1).arg(m_items.size()));
    const Item &item = m_items.at(m_step);
    m_stepText->setText("<html><body style=\"" + m_bodyCss + "\">" + "<h1>" + item.title
                        + "</h1><p>" + item.brief + "</p>" + item.description + "</body></html>");
    const QString anchorObjectName = m_items.at(m_step).pointerAnchorObjectName;
    if (!anchorObjectName.isEmpty()) {
        m_stepPointerAnchor = parentWidget()->findChild<QWidget *>(anchorObjectName);
        QTC_CHECK(m_stepPointerAnchor);
    } else {
        m_stepPointerAnchor.clear();
    }
    update();
}

void IntroductionWidget::resizeToParent()
{
    QTC_ASSERT(parentWidget(), return);
    setGeometry(QRect(QPoint(0, 0), parentWidget()->size()));
    m_textWidget->setGeometry(QRect(width()/4, height()/4, width()/2, height()/2));
}

} // namespace Internal
} // namespace Welcome
