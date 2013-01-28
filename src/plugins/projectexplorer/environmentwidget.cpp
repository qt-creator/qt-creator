/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "environmentwidget.h"
#include "environmentitemswidget.h"

#include <utils/detailswidget.h>
#include <utils/environment.h>
#include <utils/environmentmodel.h>

#include <QString>
#include <QHeaderView>
#include <QPushButton>
#include <QTableView>
#include <QTextDocument> // for Qt::escape
#include <QVBoxLayout>

namespace ProjectExplorer {

////
// EnvironmentWidget::EnvironmentWidget
////

class EnvironmentWidgetPrivate
{
public:
    Utils::EnvironmentModel *m_model;

    QString m_baseEnvironmentText;
    Utils::DetailsWidget *m_detailsContainer;
    QTableView *m_environmentView;
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
    d->m_environmentView = new QTableView(this);
    d->m_environmentView->setModel(d->m_model);
    d->m_environmentView->setMinimumHeight(400);
    d->m_environmentView->setGridStyle(Qt::NoPen);
    d->m_environmentView->horizontalHeader()->setStretchLastSection(true);
    d->m_environmentView->horizontalHeader()->setResizeMode(0, QHeaderView::ResizeToContents);
    d->m_environmentView->horizontalHeader()->setHighlightSections(false);
    d->m_environmentView->verticalHeader()->hide();
    QFontMetrics fm(font());
    d->m_environmentView->verticalHeader()->setDefaultSectionSize(qMax(static_cast<int>(fm.height() * 1.2), fm.height() + 4));
    d->m_environmentView->setSelectionMode(QAbstractItemView::SingleSelection);
    horizontalLayout->addWidget(d->m_environmentView);

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

    QSpacerItem *verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);
    buttonLayout->addItem(verticalSpacer);

    d->m_batchEditButton = new QPushButton(this);
    d->m_batchEditButton->setText(tr("&Batch Edit..."));
    buttonLayout->addWidget(d->m_batchEditButton);

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
                text.append(tr("Unset <a href=\"%1\"><b>%1</b></a>").arg(Qt::escape(item.name)));
            else
                text.append(tr("Set <a href=\"%1\"><b>%1</b></a> to <b>%2</b>").arg(Qt::escape(item.name), Qt::escape(item.value)));
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

} // namespace ProjectExplorer
