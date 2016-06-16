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

#include "environmentwidget.h"

#include <coreplugin/find/itemviewfind.h>

#include <utils/detailswidget.h>
#include <utils/environment.h>
#include <utils/environmentmodel.h>
#include <utils/environmentdialog.h>
#include <utils/headerviewstretcher.h>
#include <utils/itemviews.h>
#include <utils/tooltip/tooltip.h>

#include <QString>
#include <QPushButton>
#include <QTreeView>
#include <QVBoxLayout>
#include <QKeyEvent>
#include <QStyledItemDelegate>
#include <QLineEdit>
#include <QDebug>

namespace ProjectExplorer {

class EnvironmentValidator : public QValidator
{
    Q_OBJECT
public:
    EnvironmentValidator(QWidget *parent, Utils::EnvironmentModel *model, QTreeView *view,
                         const QModelIndex &index) :
        QValidator(parent), m_model(model), m_view(view), m_index(index)
    {
        m_hideTipTimer.setInterval(2000);
        m_hideTipTimer.setSingleShot(true);
        connect(&m_hideTipTimer, &QTimer::timeout,
                this, [](){Utils::ToolTip::hide();});
    }

    QValidator::State validate(QString &in, int &pos) const override
    {
        Q_UNUSED(pos)
        QModelIndex idx = m_model->variableToIndex(in);
        if (idx.isValid() && idx != m_index)
            return QValidator::Intermediate;
        Utils::ToolTip::hide();
        m_hideTipTimer.stop();
        return QValidator::Acceptable;
    }

    void fixup(QString &input) const override
    {
        Q_UNUSED(input)

        QPoint pos = m_view->mapToGlobal(m_view->visualRect(m_index).topLeft());
        pos -= Utils::ToolTip::offsetFromPosition();
        Utils::ToolTip::show(pos, tr("Variable already exists."));
        m_hideTipTimer.start();
        // do nothing
    }
private:
    Utils::EnvironmentModel *m_model;
    QTreeView *m_view;
    QModelIndex m_index;
    mutable QTimer m_hideTipTimer;
};

class EnvironmentDelegate : public QStyledItemDelegate
{
public:
    EnvironmentDelegate(Utils::EnvironmentModel *model,
                        QTreeView *view)
        : QStyledItemDelegate(view), m_model(model), m_view(view)
    {}

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
    {
        QWidget *w = QStyledItemDelegate::createEditor(parent, option, index);
        if (index.column() != 0)
            return w;

        if (auto edit = qobject_cast<QLineEdit *>(w))
            edit->setValidator(new EnvironmentValidator(edit, m_model, m_view, index));
        return w;
    }
private:
    Utils::EnvironmentModel *m_model;
    QTreeView *m_view;
};


////
// EnvironmentWidget::EnvironmentWidget
////

class EnvironmentWidgetPrivate
{
public:
    Utils::EnvironmentModel *m_model;

    QString m_baseEnvironmentText;
    Utils::DetailsWidget *m_detailsContainer;
    QTreeView *m_environmentView;
    QPushButton *m_editButton;
    QPushButton *m_addButton;
    QPushButton *m_resetButton;
    QPushButton *m_unsetButton;
    QPushButton *m_batchEditButton;
};

EnvironmentWidget::EnvironmentWidget(QWidget *parent, QWidget *additionalDetailsWidget)
    : QWidget(parent), d(new EnvironmentWidgetPrivate)
{
    d->m_model = new Utils::EnvironmentModel();
    connect(d->m_model, &Utils::EnvironmentModel::userChangesChanged,
            this, &EnvironmentWidget::userChangesChanged);
    connect(d->m_model, &QAbstractItemModel::modelReset,
            this, &EnvironmentWidget::invalidateCurrentIndex);

    connect(d->m_model, &Utils::EnvironmentModel::focusIndex,
            this, &EnvironmentWidget::focusIndex);

    auto vbox = new QVBoxLayout(this);
    vbox->setContentsMargins(0, 0, 0, 0);

    d->m_detailsContainer = new Utils::DetailsWidget(this);

    auto details = new QWidget(d->m_detailsContainer);
    d->m_detailsContainer->setWidget(details);
    details->setVisible(false);

    auto vbox2 = new QVBoxLayout(details);
    vbox2->setMargin(0);

    if (additionalDetailsWidget)
        vbox2->addWidget(additionalDetailsWidget);

    auto horizontalLayout = new QHBoxLayout();
    horizontalLayout->setMargin(0);
    auto tree = new Utils::TreeView(this);
    connect(tree, &QAbstractItemView::activated,
            tree, [tree](const QModelIndex &idx) { tree->edit(idx); });
    d->m_environmentView = tree;
    d->m_environmentView->setModel(d->m_model);
    d->m_environmentView->setItemDelegate(new EnvironmentDelegate(d->m_model, d->m_environmentView));
    d->m_environmentView->setMinimumHeight(400);
    d->m_environmentView->setRootIsDecorated(false);
    d->m_environmentView->setUniformRowHeights(true);
    new Utils::HeaderViewStretcher(d->m_environmentView->header(), 1);
    d->m_environmentView->setSelectionMode(QAbstractItemView::SingleSelection);
    d->m_environmentView->setSelectionBehavior(QAbstractItemView::SelectItems);
    d->m_environmentView->setFrameShape(QFrame::NoFrame);
    QFrame *findWrapper = Core::ItemViewFind::createSearchableWrapper(d->m_environmentView, Core::ItemViewFind::LightColored);
    findWrapper->setFrameStyle(QFrame::StyledPanel);
    horizontalLayout->addWidget(findWrapper);

    auto buttonLayout = new QVBoxLayout();

    d->m_editButton = new QPushButton(this);
    d->m_editButton->setText(tr("&Edit"));
    buttonLayout->addWidget(d->m_editButton);

    d->m_addButton = new QPushButton(this);
    d->m_addButton->setText(tr("&Add"));
    buttonLayout->addWidget(d->m_addButton);

    d->m_resetButton = new QPushButton(this);
    d->m_resetButton->setEnabled(false);
    d->m_resetButton->setText(tr("&Reset"));
    buttonLayout->addWidget(d->m_resetButton);

    d->m_unsetButton = new QPushButton(this);
    d->m_unsetButton->setEnabled(false);
    d->m_unsetButton->setText(tr("&Unset"));
    buttonLayout->addWidget(d->m_unsetButton);

    d->m_batchEditButton = new QPushButton(this);
    d->m_batchEditButton->setText(tr("&Batch Edit..."));
    buttonLayout->addWidget(d->m_batchEditButton);

    buttonLayout->addStretch();

    horizontalLayout->addLayout(buttonLayout);
    vbox2->addLayout(horizontalLayout);

    vbox->addWidget(d->m_detailsContainer);

    connect(d->m_model, &QAbstractItemModel::dataChanged,
            this, &EnvironmentWidget::updateButtons);

    connect(d->m_editButton, &QAbstractButton::clicked,
            this, &EnvironmentWidget::editEnvironmentButtonClicked);
    connect(d->m_addButton, &QAbstractButton::clicked,
            this, &EnvironmentWidget::addEnvironmentButtonClicked);
    connect(d->m_resetButton, &QAbstractButton::clicked,
            this, &EnvironmentWidget::removeEnvironmentButtonClicked);
    connect(d->m_unsetButton, &QAbstractButton::clicked,
            this, &EnvironmentWidget::unsetEnvironmentButtonClicked);
    connect(d->m_batchEditButton, &QAbstractButton::clicked,
            this, &EnvironmentWidget::batchEditEnvironmentButtonClicked);
    connect(d->m_environmentView->selectionModel(), &QItemSelectionModel::currentChanged,
            this, &EnvironmentWidget::environmentCurrentIndexChanged);

    connect(d->m_detailsContainer, &Utils::DetailsWidget::linkActivated,
            this, &EnvironmentWidget::linkActivated);

    connect(d->m_model, &Utils::EnvironmentModel::userChangesChanged,
            this, &EnvironmentWidget::updateSummaryText);
}

EnvironmentWidget::~EnvironmentWidget()
{
    delete d->m_model;
    d->m_model = nullptr;
    delete d;
}

void EnvironmentWidget::focusIndex(const QModelIndex &index)
{
    d->m_environmentView->setCurrentIndex(index);
    d->m_environmentView->setFocus();
}

void EnvironmentWidget::setBaseEnvironment(const Utils::Environment &env)
{
    d->m_model->setBaseEnvironment(env);
}

void EnvironmentWidget::setBaseEnvironmentText(const QString &text)
{
    d->m_baseEnvironmentText = text;
    updateSummaryText();
}

QList<Utils::EnvironmentItem> EnvironmentWidget::userChanges() const
{
    return d->m_model->userChanges();
}

void EnvironmentWidget::setUserChanges(const QList<Utils::EnvironmentItem> &list)
{
    d->m_model->setUserChanges(list);
    updateSummaryText();
}

void EnvironmentWidget::updateSummaryText()
{
    QList<Utils::EnvironmentItem> list = d->m_model->userChanges();
    Utils::EnvironmentItem::sort(&list);

    QString text;
    foreach (const Utils::EnvironmentItem &item, list) {
        if (item.name != Utils::EnvironmentModel::tr("<VARIABLE>")) {
            text.append(QLatin1String("<br>"));
            if (item.unset)
                text.append(tr("Unset <a href=\"%1\"><b>%1</b></a>").arg(item.name.toHtmlEscaped()));
            else
                text.append(tr("Set <a href=\"%1\"><b>%1</b></a> to <b>%2</b>").arg(item.name.toHtmlEscaped(), item.value.toHtmlEscaped()));
        }
    }

    if (text.isEmpty()) {
        //: %1 is "System Environment" or some such.
        text.prepend(tr("Use <b>%1</b>").arg(d->m_baseEnvironmentText));
    } else {
        //: Yup, word puzzle. The Set/Unset phrases above are appended to this.
        //: %1 is "System Environment" or some such.
        text.prepend(tr("Use <b>%1</b> and").arg(d->m_baseEnvironmentText));
    }

    d->m_detailsContainer->setSummaryText(text);
}

void EnvironmentWidget::linkActivated(const QString &link)
{
    d->m_detailsContainer->setState(Utils::DetailsWidget::Expanded);
    QModelIndex idx = d->m_model->variableToIndex(link);
    focusIndex(idx);
}

void EnvironmentWidget::updateButtons()
{
    environmentCurrentIndexChanged(d->m_environmentView->currentIndex());
}

void EnvironmentWidget::editEnvironmentButtonClicked()
{
    d->m_environmentView->edit(d->m_environmentView->currentIndex());
}

void EnvironmentWidget::addEnvironmentButtonClicked()
{
    QModelIndex index = d->m_model->addVariable();
    d->m_environmentView->setCurrentIndex(index);
    d->m_environmentView->edit(index);
}

void EnvironmentWidget::removeEnvironmentButtonClicked()
{
    const QString &name = d->m_model->indexToVariable(d->m_environmentView->currentIndex());
    d->m_model->resetVariable(name);
}

// unset in Merged Environment Mode means, unset if it comes from the base environment
// or remove when it is just a change we added
void EnvironmentWidget::unsetEnvironmentButtonClicked()
{
    const QString &name = d->m_model->indexToVariable(d->m_environmentView->currentIndex());
    if (!d->m_model->canReset(name))
        d->m_model->resetVariable(name);
    else
        d->m_model->unsetVariable(name);
}

void EnvironmentWidget::batchEditEnvironmentButtonClicked()
{
    const QList<Utils::EnvironmentItem> changes = d->m_model->userChanges();

    bool ok;
    const QList<Utils::EnvironmentItem> newChanges = Utils::EnvironmentDialog::getEnvironmentItems(&ok, this, changes);
    if (!ok)
        return;

    d->m_model->setUserChanges(newChanges);
}

void EnvironmentWidget::environmentCurrentIndexChanged(const QModelIndex &current)
{
    if (current.isValid()) {
        d->m_editButton->setEnabled(true);
        const QString &name = d->m_model->indexToVariable(current);
        bool modified = d->m_model->canReset(name) && d->m_model->changes(name);
        bool unset = d->m_model->canUnset(name);
        d->m_resetButton->setEnabled(modified || unset);
        d->m_unsetButton->setEnabled(!unset);
    } else {
        d->m_editButton->setEnabled(false);
        d->m_resetButton->setEnabled(false);
        d->m_unsetButton->setEnabled(false);
    }
}

void EnvironmentWidget::invalidateCurrentIndex()
{
    environmentCurrentIndexChanged(QModelIndex());
}

} // namespace ProjectExplorer

#include "environmentwidget.moc"
