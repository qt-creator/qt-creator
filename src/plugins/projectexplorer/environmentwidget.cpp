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

#include <coreplugin/fileutils.h>
#include <coreplugin/find/itemviewfind.h>

#include <utils/detailswidget.h>
#include <utils/environment.h>
#include <utils/environmentdialog.h>
#include <utils/environmentmodel.h>
#include <utils/headerviewstretcher.h>
#include <utils/hostosinfo.h>
#include <utils/itemviews.h>
#include <utils/namevaluevalidator.h>
#include <utils/tooltip/tooltip.h>

#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QString>
#include <QPushButton>
#include <QTreeView>
#include <QVBoxLayout>
#include <QKeyEvent>
#include <QStyledItemDelegate>
#include <QLineEdit>
#include <QDebug>

namespace ProjectExplorer {

class EnvironmentDelegate : public QStyledItemDelegate
{
public:
    EnvironmentDelegate(Utils::EnvironmentModel *model,
                        QTreeView *view)
        : QStyledItemDelegate(view), m_model(model), m_view(view)
    {}

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        QWidget *w = QStyledItemDelegate::createEditor(parent, option, index);
        if (index.column() != 0)
            return w;

        if (auto edit = qobject_cast<QLineEdit *>(w))
            edit->setValidator(new Utils::NameValueValidator(
                edit, m_model, m_view, index, EnvironmentWidget::tr("Variable already exists.")));
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
    EnvironmentWidget::OpenTerminalFunc m_openTerminalFunc;
    Utils::DetailsWidget *m_detailsContainer;
    QTreeView *m_environmentView;
    QPushButton *m_editButton;
    QPushButton *m_addButton;
    QPushButton *m_resetButton;
    QPushButton *m_unsetButton;
    QPushButton *m_toggleButton;
    QPushButton *m_batchEditButton;
    QPushButton *m_appendPathButton = nullptr;
    QPushButton *m_prependPathButton = nullptr;
    QPushButton *m_terminalButton;
};

EnvironmentWidget::EnvironmentWidget(QWidget *parent, Type type, QWidget *additionalDetailsWidget)
    : QWidget(parent), d(std::make_unique<EnvironmentWidgetPrivate>())
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
    vbox2->setContentsMargins(0, 0, 0, 0);

    if (additionalDetailsWidget)
        vbox2->addWidget(additionalDetailsWidget);

    auto horizontalLayout = new QHBoxLayout();
    horizontalLayout->setContentsMargins(0, 0, 0, 0);
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
    d->m_editButton->setText(tr("Ed&it"));
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

    d->m_toggleButton = new QPushButton(tr("Disable"), this);
    buttonLayout->addWidget(d->m_toggleButton);
    connect(d->m_toggleButton, &QPushButton::clicked, this, [this] {
        d->m_model->toggleVariable(d->m_environmentView->currentIndex());
        updateButtons();
    });

    if (type == TypeLocal) {
        d->m_appendPathButton = new QPushButton(this);
        d->m_appendPathButton->setEnabled(false);
        d->m_appendPathButton->setText(tr("Append Path..."));
        buttonLayout->addWidget(d->m_appendPathButton);
        d->m_prependPathButton = new QPushButton(this);
        d->m_prependPathButton->setEnabled(false);
        d->m_prependPathButton->setText(tr("Prepend Path..."));
        buttonLayout->addWidget(d->m_prependPathButton);
        connect(d->m_appendPathButton, &QAbstractButton::clicked,
                this, &EnvironmentWidget::appendPathButtonClicked);
        connect(d->m_prependPathButton, &QAbstractButton::clicked,
                this, &EnvironmentWidget::prependPathButtonClicked);
    }

    d->m_batchEditButton = new QPushButton(this);
    d->m_batchEditButton->setText(tr("&Batch Edit..."));
    buttonLayout->addWidget(d->m_batchEditButton);

    d->m_terminalButton = new QPushButton(this);
    d->m_terminalButton->setText(tr("Open &Terminal"));
    d->m_terminalButton->setToolTip(tr("Open a terminal with this environment set up."));
    d->m_terminalButton->setEnabled(type == TypeLocal);
    buttonLayout->addWidget(d->m_terminalButton);
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
    connect(d->m_terminalButton, &QAbstractButton::clicked,
            this, [this] {
        Utils::Environment env = d->m_model->baseEnvironment();
        env.modify(d->m_model->userChanges());
        if (d->m_openTerminalFunc)
            d->m_openTerminalFunc(env);
        else
            Core::FileUtils::openTerminal(QDir::currentPath(), env);
    });
    connect(d->m_detailsContainer, &Utils::DetailsWidget::linkActivated,
            this, &EnvironmentWidget::linkActivated);

    connect(d->m_model, &Utils::EnvironmentModel::userChangesChanged,
            this, &EnvironmentWidget::updateSummaryText);
}

EnvironmentWidget::~EnvironmentWidget()
{
    delete d->m_model;
    d->m_model = nullptr;
}

void EnvironmentWidget::focusIndex(const QModelIndex &index)
{
    d->m_environmentView->setCurrentIndex(index);
    d->m_environmentView->setFocus();
    // When the current item changes as a result of the call above,
    // QAbstractItemView::currentChanged() is called. That calls scrollTo(current),
    // using the default EnsureVisible scroll hint, whereas we want PositionAtTop,
    // because it ensures that the user doesn't have to scroll down when they've
    // added a new environment variable and want to edit its value; they'll be able
    // to see its value as they're typing it.
    // This only helps to a certain degree - variables whose names start with letters
    // later in the alphabet cause them fall within the "end" of the view's range,
    // making it impossible to position them at the top of the view.
    d->m_environmentView->scrollTo(index, QAbstractItemView::PositionAtTop);
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

Utils::EnvironmentItems EnvironmentWidget::userChanges() const
{
    return d->m_model->userChanges();
}

void EnvironmentWidget::setUserChanges(const Utils::EnvironmentItems &list)
{
    d->m_model->setUserChanges(list);
    updateSummaryText();
}

void EnvironmentWidget::setOpenTerminalFunc(const EnvironmentWidget::OpenTerminalFunc &func)
{
    d->m_openTerminalFunc = func;
    d->m_terminalButton->setEnabled(bool(func));
}

void EnvironmentWidget::updateSummaryText()
{
    Utils::EnvironmentItems list = d->m_model->userChanges();
    Utils::EnvironmentItem::sort(&list);

    QString text;
    foreach (const Utils::EnvironmentItem &item, list) {
        if (item.name != Utils::EnvironmentModel::tr("<VARIABLE>")) {
            text.append(QLatin1String("<br>"));
            switch (item.operation) {
            case Utils::EnvironmentItem::Unset:
                text.append(tr("Unset <a href=\"%1\"><b>%1</b></a>").arg(item.name.toHtmlEscaped()));
                break;
            case Utils::EnvironmentItem::SetEnabled:
            case Utils::EnvironmentItem::Append:
            case Utils::EnvironmentItem::Prepend:
                text.append(tr("Set <a href=\"%1\"><b>%1</b></a> to <b>%2</b>").arg(item.name.toHtmlEscaped(), item.value.toHtmlEscaped()));
                break;
            case Utils::EnvironmentItem::SetDisabled:
                text.append(tr("Set <a href=\"%1\"><b>%1</b></a> to <b>%2</b> [disabled]").arg(item.name.toHtmlEscaped(), item.value.toHtmlEscaped()));
                break;
            }
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

bool EnvironmentWidget::currentEntryIsPathList(const QModelIndex &current) const
{
    if (!current.isValid())
        return false;

    // Look at the name first and check it against some well-known path variables. Extend as needed.
    const QString varName = d->m_model->indexToVariable(current);
    if (varName.compare("PATH", Utils::HostOsInfo::fileNameCaseSensitivity()) == 0)
        return true;
    if (Utils::HostOsInfo::isMacHost() && varName == "DYLD_LIBRARY_PATH")
        return true;
    if (Utils::HostOsInfo::isAnyUnixHost() && varName == "LD_LIBRARY_PATH")
        return true;

    // Now check the value: If it's a list of strings separated by the platform's path separator
    // and at least one of the strings is an existing directory, then that's enough proof for us.
    QModelIndex valueIndex = current;
    if (valueIndex.column() == 0)
        valueIndex = valueIndex.siblingAtColumn(1);
    const QStringList entries = d->m_model->data(valueIndex).toString()
            .split(Utils::HostOsInfo::pathListSeparator(), QString::SkipEmptyParts);
    if (entries.length() < 2)
        return false;
    for (const QString &potentialDir : entries) {
        if (QFileInfo(potentialDir).isDir())
            return true;
    }
    return false;
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

void EnvironmentWidget::amendPathList(const PathListModifier &modifier)
{
    const QString varName = d->m_model->indexToVariable(d->m_environmentView->currentIndex());
    const QString dir = QDir::toNativeSeparators(
                QFileDialog::getExistingDirectory(this, tr("Choose Directory")));
    if (dir.isEmpty())
        return;
    QModelIndex index = d->m_model->variableToIndex(varName);
    if (!index.isValid())
        return;
    if (index.column() == 0)
        index = index.siblingAtColumn(1);
    const QString value = d->m_model->data(index).toString();
    d->m_model->setData(index, modifier(value, dir));
}

void EnvironmentWidget::appendPathButtonClicked()
{
    amendPathList([](const QString &pathList, const QString &dir) {
        QString newPathList = dir;
        if (!pathList.isEmpty())
            newPathList.prepend(Utils::HostOsInfo::pathListSeparator()).prepend(pathList);
        return newPathList;
    });
}

void EnvironmentWidget::prependPathButtonClicked()
{
    amendPathList([](const QString &pathList, const QString &dir) {
        QString newPathList = dir;
        if (!pathList.isEmpty())
            newPathList.append(Utils::HostOsInfo::pathListSeparator()).append(pathList);
        return newPathList;
    });
}

void EnvironmentWidget::batchEditEnvironmentButtonClicked()
{
    const Utils::EnvironmentItems changes = d->m_model->userChanges();

    const auto newChanges = Utils::EnvironmentDialog::getEnvironmentItems(this, changes);

    if (newChanges)
        d->m_model->setUserChanges(*newChanges);
}

void EnvironmentWidget::environmentCurrentIndexChanged(const QModelIndex &current)
{
    if (current.isValid()) {
        d->m_editButton->setEnabled(true);
        const QString &name = d->m_model->indexToVariable(current);
        bool modified = d->m_model->canReset(name) && d->m_model->changes(name);
        bool unset = d->m_model->isUnset(name);
        d->m_resetButton->setEnabled(modified || unset);
        d->m_unsetButton->setEnabled(!unset);
        d->m_toggleButton->setEnabled(!unset);
        d->m_toggleButton->setText(d->m_model->isEnabled(name) ? tr("Disable") : tr("Enable"));
    } else {
        d->m_editButton->setEnabled(false);
        d->m_resetButton->setEnabled(false);
        d->m_unsetButton->setEnabled(false);
        d->m_toggleButton->setEnabled(false);
        d->m_toggleButton->setText(tr("Disable"));
    }
    if (d->m_appendPathButton) {
        d->m_appendPathButton->setEnabled(currentEntryIsPathList(current));
        d->m_prependPathButton->setEnabled(currentEntryIsPathList(current));
    }
}

void EnvironmentWidget::invalidateCurrentIndex()
{
    environmentCurrentIndexChanged(QModelIndex());
}

} // namespace ProjectExplorer
