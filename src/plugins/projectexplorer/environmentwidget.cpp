/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "environmentwidget.h"
#include "environmentitemswidget.h"

#include <coreplugin/find/itemviewfind.h>

#include <utils/detailswidget.h>
#include <utils/environment.h>
#include <utils/environmentmodel.h>
#include <utils/headerviewstretcher.h>
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
    EnvironmentValidator(QWidget *parent, Utils::EnvironmentModel *model,
                         QTreeView *view,
                         const QModelIndex &index)
        : QValidator(parent), m_model(model), m_view(view), m_index(index)
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

        if (QLineEdit *edit = qobject_cast<QLineEdit *>(w))
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
    connect(d->m_model, SIGNAL(userChangesChanged()),
            this, SIGNAL(userChangesChanged()));
    connect(d->m_model, SIGNAL(modelReset()),
            this, SLOT(invalidateCurrentIndex()));

    connect(d->m_model, SIGNAL(focusIndex(QModelIndex)),
            this, SLOT(focusIndex(QModelIndex)));

    QVBoxLayout *vbox = new QVBoxLayout(this);
    vbox->setContentsMargins(0, 0, 0, 0);

    d->m_detailsContainer = new Utils::DetailsWidget(this);

    QWidget *details = new QWidget(d->m_detailsContainer);
    d->m_detailsContainer->setWidget(details);
    details->setVisible(false);

    QVBoxLayout *vbox2 = new QVBoxLayout(details);
    vbox2->setMargin(0);

    if (additionalDetailsWidget)
        vbox2->addWidget(additionalDetailsWidget);

    QHBoxLayout *horizontalLayout = new QHBoxLayout();
    horizontalLayout->setMargin(0);
    d->m_environmentView = new Internal::EnvironmentTreeView(this);
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

    QVBoxLayout *buttonLayout = new QVBoxLayout();

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

    connect(d->m_model, SIGNAL(dataChanged(QModelIndex,QModelIndex)),
            this, SLOT(updateButtons()));

    connect(d->m_editButton, SIGNAL(clicked(bool)),
            this, SLOT(editEnvironmentButtonClicked()));
    connect(d->m_addButton, SIGNAL(clicked(bool)),
            this, SLOT(addEnvironmentButtonClicked()));
    connect(d->m_resetButton, SIGNAL(clicked(bool)),
            this, SLOT(removeEnvironmentButtonClicked()));
    connect(d->m_unsetButton, SIGNAL(clicked(bool)),
            this, SLOT(unsetEnvironmentButtonClicked()));
    connect(d->m_batchEditButton, SIGNAL(clicked(bool)),
            this, SLOT(batchEditEnvironmentButtonClicked()));
    connect(d->m_environmentView->selectionModel(), SIGNAL(currentChanged(QModelIndex,QModelIndex)),
            this, SLOT(environmentCurrentIndexChanged(QModelIndex)));

    connect(d->m_detailsContainer, SIGNAL(linkActivated(QString)),
            this, SLOT(linkActivated(QString)));

    connect(d->m_model, SIGNAL(userChangesChanged()), this, SLOT(updateSummaryText()));
}

EnvironmentWidget::~EnvironmentWidget()
{
    delete d->m_model;
    d->m_model = 0;
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
    const QList<Utils::EnvironmentItem> newChanges = EnvironmentItemsDialog::getEnvironmentItems(this, changes, &ok);
    if (ok)
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

Internal::EnvironmentTreeView::EnvironmentTreeView(QWidget *parent)
    : QTreeView(parent)
{

}

QModelIndex Internal::EnvironmentTreeView::moveCursor(QAbstractItemView::CursorAction cursorAction, Qt::KeyboardModifiers modifiers)
{
    QModelIndex idx = currentIndex();
    int column = idx.column();
    int row = idx.row();

    if (cursorAction == QAbstractItemView::MoveNext) {
        if (column == 0)
            return idx.sibling(row, 1);
        else if (row + 1 < model()->rowCount())
            return idx.sibling(row + 1, 0);
        else // On last column in last row
            return idx;
    } else if (cursorAction == QAbstractItemView::MovePrevious) {
        if (column == 1)
            return idx.sibling(row, 0);
        else if (row - 1 >= 0)
            return idx.sibling(row - 1, 1);
        else // On first column in first row
            return idx;
    }
    return QTreeView::moveCursor(cursorAction, modifiers);
}

void Internal::EnvironmentTreeView::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return) {
        if (!edit(currentIndex(), EditKeyPressed, event))
            event->ignore();
        return;
    }

    QTreeView::keyPressEvent(event);
}


} // namespace ProjectExplorer

#include "environmentwidget.moc"
