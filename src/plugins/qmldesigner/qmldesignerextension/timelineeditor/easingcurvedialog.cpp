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

#include "easingcurvedialog.h"

#include "preseteditor.h"
#include "splineeditor.h"

#include <QApplication>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSizePolicy>
#include <QSpinBox>
#include <QTabBar>
#include <QTabWidget>
#include <QVBoxLayout>

#include <abstractview.h>
#include <bindingproperty.h>
#include <rewritingexception.h>
#include <theme.h>
#include <utils/qtcassert.h>

namespace QmlDesigner {

EasingCurveDialog::EasingCurveDialog(const QList<ModelNode> &frames, QWidget *parent)
    : QDialog(parent)
    , m_splineEditor(new SplineEditor(this))
    , m_text(new QPlainTextEdit(this))
    , m_presets(new PresetEditor(this))
    , m_durationLayout(new QHBoxLayout)
    , m_buttons(new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel
                                     | QDialogButtonBox::Ok))
    , m_label(new QLabel)
    , m_frames(frames)
{
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    auto tw = new QTabWidget;
    tw->setTabPosition(QTabWidget::East);
    tw->addTab(m_splineEditor, "Curve");
    tw->addTab(m_text, "Text");

    connect(tw, &QTabWidget::currentChanged, this, &EasingCurveDialog::tabClicked);
    connect(m_text, &QPlainTextEdit::textChanged, this, &EasingCurveDialog::textChanged);

    auto labelFont = m_label->font();
    labelFont.setPointSize(labelFont.pointSize() + 2);
    m_label->setFont(labelFont);

    auto hSpacing = qApp->style()->pixelMetric(QStyle::PM_LayoutHorizontalSpacing);
    auto vSpacing = qApp->style()->pixelMetric(QStyle::PM_LayoutVerticalSpacing);
    auto *vbox = new QVBoxLayout;
    vbox->setContentsMargins(2, 0, 0, vSpacing);
    vbox->addWidget(m_label);

    auto *presetBar = new QTabBar;

    auto smallFont = presetBar->font();
    smallFont.setPixelSize(Theme::instance()->smallFontPixelSize());

    presetBar->setFont(smallFont);
    presetBar->setExpanding(false);
    presetBar->setDrawBase(false);
    presetBar->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);

    auto *durationLabel = new QLabel("Duration (ms)");
    auto *durationEdit = new QSpinBox;
    durationEdit->setMaximum(std::numeric_limits<int>::max());
    durationEdit->setValue(1000);
    auto *animateButton = new QPushButton("Preview");

    m_durationLayout->setContentsMargins(0, vSpacing, 0, 0);
    m_durationLayout->addWidget(durationLabel);
    m_durationLayout->addWidget(durationEdit);
    m_durationLayout->addWidget(animateButton);

    m_durationLayout->insertSpacing(1, hSpacing);
    m_durationLayout->insertSpacing(2, hSpacing);
    m_durationLayout->insertSpacing(4, hSpacing);
    m_durationLayout->addStretch(hSpacing);

    m_buttons->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
    auto callButtonsClicked = [this](QAbstractButton *button) {
        buttonsClicked(m_buttons->standardButton(button));
    };

    connect(m_buttons, &QDialogButtonBox::clicked, this, callButtonsClicked);

    auto *buttonLayout = new QVBoxLayout;
    buttonLayout->setContentsMargins(0, vSpacing, 0, 0);
    buttonLayout->addWidget(m_buttons);

    auto *grid = new QGridLayout;
    grid->setVerticalSpacing(0);
    grid->addLayout(vbox, 0, 0);
    grid->addWidget(presetBar, 0, 1, Qt::AlignBottom);

    grid->addWidget(tw);
    grid->addWidget(m_presets, 1, 1);
    grid->addLayout(m_durationLayout, 2, 0);
    grid->addLayout(buttonLayout, 2, 1);

    auto *groupBox = new QGroupBox;
    groupBox->setLayout(grid);

    auto *tabWidget = new QTabWidget(this);
    tabWidget->addTab(groupBox, "Easing Curve Editor");

    auto *mainBox = new QVBoxLayout;
    mainBox->addWidget(tabWidget);
    setLayout(mainBox);

    connect(m_splineEditor,
            &SplineEditor::easingCurveChanged,
            this,
            &EasingCurveDialog::updateEasingCurve);

    connect(m_presets, &PresetEditor::presetChanged, m_splineEditor, &SplineEditor::setEasingCurve);

    connect(durationEdit,
            static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            m_splineEditor,
            &SplineEditor::setDuration);

    connect(animateButton, &QPushButton::clicked, m_splineEditor, &SplineEditor::animate);

    m_presets->initialize(presetBar);

    m_splineEditor->setDuration(durationEdit->value());

    resize(QSize(1421, 918));
}

void EasingCurveDialog::initialize(const QString &curveString)
{
    EasingCurve curve;
    if (curveString.isEmpty()) {
        QEasingCurve qcurve;
        qcurve.addCubicBezierSegment(QPointF(0.2, 0.2), QPointF(0.8, 0.8), QPointF(1.0, 1.0));
        curve = EasingCurve(qcurve);
    } else
        curve.fromString(curveString);

    m_splineEditor->setEasingCurve(curve);
}

void EasingCurveDialog::runDialog(const QList<ModelNode> &frames, QWidget *parent)
{
    if (frames.empty())
        return;

    EasingCurveDialog dialog(frames, parent);

    ModelNode current = frames.last();

    if (current.hasBindingProperty("easing.bezierCurve"))
        dialog.initialize(current.bindingProperty("easing.bezierCurve").expression());
    else
        dialog.initialize("");

    dialog.exec();
}

bool EasingCurveDialog::apply()
{
    QTC_ASSERT(!m_frames.empty(), return false);

    EasingCurve curve = m_splineEditor->easingCurve();
    if (!curve.isLegal()) {
        QMessageBox msgBox;
        msgBox.setText("Attempting to apply invalid curve to keyframe");
        msgBox.setInformativeText("Please solve the issue before proceeding.");
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.exec();
        return false;
    }

    try {
        AbstractView *view = m_frames.first().view();
        RewriterTransaction transaction(view->beginRewriterTransaction("EasingCurveDialog::apply"));

        auto expression = m_splineEditor->easingCurve().toString();
        for (const auto &frame : m_frames)
            frame.bindingProperty("easing.bezierCurve").setExpression(expression);

        transaction.commit();
        return true;
    } catch (const RewritingException &e) {
        e.showException();
    }

    return false;
}

void EasingCurveDialog::textChanged()
{
    auto curve = m_splineEditor->easingCurve();
    curve.fromString(m_text->toPlainText());
    m_splineEditor->setEasingCurve(curve);
}

void EasingCurveDialog::tabClicked(int id)
{
    if (auto tw = qobject_cast<const QTabWidget *>(sender())) {
        int seid = tw->indexOf(m_splineEditor);
        if (seid == id) {
            for (int i = 0; i < m_durationLayout->count(); ++i) {
                auto *item = m_durationLayout->itemAt(i);
                if (auto *widget = item->widget())
                    widget->show();
            }

            auto curve = m_splineEditor->easingCurve();
            curve.fromString(m_text->toPlainText());
            m_splineEditor->setEasingCurve(curve);

        } else {
            for (int i = 0; i < m_durationLayout->count(); ++i) {
                auto *item = m_durationLayout->itemAt(i);
                if (auto *widget = item->widget())
                    widget->hide();
            }

            auto curve = m_splineEditor->easingCurve();
            m_text->setPlainText(curve.toString());
        }
    }
}

void EasingCurveDialog::presetTabClicked(int id)
{
    m_presets->activate(id);
}

void EasingCurveDialog::updateEasingCurve(const EasingCurve &curve)
{
    if (!curve.isLegal()) {
        auto *save = m_buttons->button(QDialogButtonBox::Save);
        save->setEnabled(false);

        auto *ok = m_buttons->button(QDialogButtonBox::Ok);
        ok->setEnabled(false);

        m_label->setText("Invalid Curve!");
    } else {
        auto *save = m_buttons->button(QDialogButtonBox::Save);
        save->setEnabled(true);

        auto *ok = m_buttons->button(QDialogButtonBox::Ok);
        ok->setEnabled(true);

        m_label->setText("");
    }

    m_presets->update(curve);
}

void EasingCurveDialog::buttonsClicked(QDialogButtonBox::StandardButton button)
{
    switch (button) {
    case QDialogButtonBox::Ok:
        if (apply())
            close();
        break;

    case QDialogButtonBox::Cancel:
        close();
        break;

    case QDialogButtonBox::Save:
        m_presets->writePresets(m_splineEditor->easingCurve());
        break;

    default:
        break;
    }
}

} // namespace QmlDesigner
