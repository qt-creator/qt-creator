/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include "preprocessormacrowidget.h"
#include "clangindexingprojectsettings.h"

#include <utils/detailswidget.h>
#include <utils/headerviewstretcher.h>
#include <utils/itemviews.h>
#include <utils/namevaluedictionary.h>
#include <utils/namevalueitem.h>
#include <utils/namevaluemodel.h>
#include <utils/namevaluesdialog.h>
#include <utils/namevaluevalidator.h>

#include <coreplugin/find/itemviewfind.h>

#include <QLineEdit>
#include <QStyledItemDelegate>
#include <QVBoxLayout>

namespace ClangPchManager {

class ProcessorMacroDelegate : public QStyledItemDelegate
{
public:
    ProcessorMacroDelegate(Utils::NameValueModel *model, QTreeView *view)
        : QStyledItemDelegate(view)
        , m_model(model)
        , m_view(view)
    {}

    QWidget *createEditor(QWidget *parent,
                          const QStyleOptionViewItem &option,
                          const QModelIndex &index) const override
    {
        QWidget *w = QStyledItemDelegate::createEditor(parent, option, index);
        if (index.column() != 0)
            return w;

        if (auto edit = qobject_cast<QLineEdit *>(w))
            edit->setValidator(new Utils::NameValueValidator(
                edit, m_model, m_view, index, PreprocessorMacroWidget::tr("Macro already exists.")));
        return w;
    }

private:
    Utils::NameValueModel *m_model;
    QTreeView *m_view;
};

PreprocessorMacroWidget::PreprocessorMacroWidget(QWidget *parent) : QWidget(parent)
{
    m_model = std::make_unique<Utils::NameValueModel>();
    connect(m_model.get(),
            &Utils::NameValueModel::userChangesChanged,
            this,
            &PreprocessorMacroWidget::userChangesChanged);
    connect(m_model.get(),
            &QAbstractItemModel::modelReset,
            this,
            &PreprocessorMacroWidget::invalidateCurrentIndex);

    connect(m_model.get(), &Utils::NameValueModel::focusIndex, this, &PreprocessorMacroWidget::focusIndex);

    auto vbox = new QVBoxLayout(this);
    vbox->setContentsMargins(0, 0, 0, 0);

    m_detailsContainer = new Utils::DetailsWidget(this);

    auto details = new QWidget(m_detailsContainer);
    m_detailsContainer->setWidget(details);
    details->setVisible(false);

    auto vbox2 = new QVBoxLayout(details);
    vbox2->setContentsMargins(0, 0, 0, 0);

    auto horizontalLayout = new QHBoxLayout;
    horizontalLayout->setContentsMargins(0, 0, 0, 0);
    auto tree = new Utils::TreeView(this);
    connect(tree, &QAbstractItemView::activated, tree, [tree](const QModelIndex &idx) {
        tree->edit(idx);
    });
    m_preprocessorMacrosView = tree;
    m_preprocessorMacrosView->setModel(m_model.get());
    m_preprocessorMacrosView->setItemDelegate(
        new ProcessorMacroDelegate(m_model.get(), m_preprocessorMacrosView));
    m_preprocessorMacrosView->setMinimumHeight(400);
    m_preprocessorMacrosView->setRootIsDecorated(false);
    m_preprocessorMacrosView->setUniformRowHeights(true);
    new Utils::HeaderViewStretcher(m_preprocessorMacrosView->header(), 1);
    m_preprocessorMacrosView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_preprocessorMacrosView->setSelectionBehavior(QAbstractItemView::SelectItems);
    m_preprocessorMacrosView->setFrameShape(QFrame::NoFrame);
    QFrame *findWrapper = Core::ItemViewFind::createSearchableWrapper(m_preprocessorMacrosView,
                                                                      Core::ItemViewFind::LightColored);
    findWrapper->setFrameStyle(QFrame::StyledPanel);
    horizontalLayout->addWidget(findWrapper);

    auto buttonLayout = new QVBoxLayout();

    m_editButton = new QPushButton(this);
    m_editButton->setText(tr("Ed&it"));
    buttonLayout->addWidget(m_editButton);

    m_addButton = new QPushButton(this);
    m_addButton->setText(tr("&Add"));
    buttonLayout->addWidget(m_addButton);

    m_resetButton = new QPushButton(this);
    m_resetButton->setEnabled(false);
    m_resetButton->setText(tr("&Reset"));
    buttonLayout->addWidget(m_resetButton);

    m_unsetButton = new QPushButton(this);
    m_unsetButton->setEnabled(false);
    m_unsetButton->setText(tr("&Unset"));
    buttonLayout->addWidget(m_unsetButton);

    buttonLayout->addStretch();

    horizontalLayout->addLayout(buttonLayout);
    vbox2->addLayout(horizontalLayout);

    vbox->addWidget(m_detailsContainer);

    connect(m_model.get(),
            &QAbstractItemModel::dataChanged,
            this,
            &PreprocessorMacroWidget::updateButtons);

    connect(m_editButton, &QAbstractButton::clicked, this, &PreprocessorMacroWidget::editButtonClicked);
    connect(m_addButton, &QAbstractButton::clicked, this, &PreprocessorMacroWidget::addButtonClicked);
    connect(m_resetButton, &QAbstractButton::clicked, this, &PreprocessorMacroWidget::removeButtonClicked);
    connect(m_unsetButton, &QAbstractButton::clicked, this, &PreprocessorMacroWidget::unsetButtonClicked);
    connect(m_preprocessorMacrosView->selectionModel(),
            &QItemSelectionModel::currentChanged,
            this,
            &PreprocessorMacroWidget::currentIndexChanged);
    connect(m_detailsContainer,
            &Utils::DetailsWidget::linkActivated,
            this,
            &PreprocessorMacroWidget::linkActivated);
    connect(m_model.get(),
            &Utils::NameValueModel::userChangesChanged,
            this,
            &PreprocessorMacroWidget::updateSummaryText);
    connect(m_model.get(),
            &Utils::NameValueModel::userChangesChanged,
            this,
            &PreprocessorMacroWidget::saveSettings);
}

void PreprocessorMacroWidget::setBasePreprocessorMacros(const PreprocessorMacros &macros)
{
    m_model->setUserChanges(m_settings->readMacros());
    m_model->setBaseNameValueDictionary(Utils::NameValueDictionary{macros});
}

void PreprocessorMacroWidget::setSettings(ClangIndexingProjectSettings *settings)
{
    m_settings = settings;
}

PreprocessorMacroWidget::~PreprocessorMacroWidget() = default;

void PreprocessorMacroWidget::updateButtons()
{
    currentIndexChanged(m_preprocessorMacrosView->currentIndex());
}

void PreprocessorMacroWidget::focusIndex(const QModelIndex &index)
{
    m_preprocessorMacrosView->setCurrentIndex(index);
    m_preprocessorMacrosView->setFocus();

    m_preprocessorMacrosView->scrollTo(index, QAbstractItemView::PositionAtTop);
}

void PreprocessorMacroWidget::invalidateCurrentIndex()
{
    currentIndexChanged(QModelIndex());
}

void PreprocessorMacroWidget::editButtonClicked()
{
    m_preprocessorMacrosView->edit(m_preprocessorMacrosView->currentIndex());
}

void PreprocessorMacroWidget::addButtonClicked()
{
    QModelIndex index = m_model->addVariable();
    m_preprocessorMacrosView->setCurrentIndex(index);
    m_preprocessorMacrosView->edit(index);
}

void PreprocessorMacroWidget::removeButtonClicked()
{
    const QString &name = m_model->indexToVariable(m_preprocessorMacrosView->currentIndex());
    m_model->resetVariable(name);
}

void PreprocessorMacroWidget::unsetButtonClicked()
{
    const QString &name = m_model->indexToVariable(m_preprocessorMacrosView->currentIndex());
    if (!m_model->canReset(name))
        m_model->resetVariable(name);
    else
        m_model->unsetVariable(name);
}

void PreprocessorMacroWidget::currentIndexChanged(const QModelIndex &current)
{
    if (current.isValid()) {
        m_editButton->setEnabled(true);
        const QString &name = m_model->indexToVariable(current);
        bool modified = m_model->canReset(name) && m_model->changes(name);
        bool unset = m_model->isUnset(name);
        m_resetButton->setEnabled(modified || unset);
        m_unsetButton->setEnabled(!unset);
    } else {
        m_editButton->setEnabled(false);
        m_resetButton->setEnabled(false);
        m_unsetButton->setEnabled(false);
    }
}

void PreprocessorMacroWidget::linkActivated(const QString &link)
{
    m_detailsContainer->setState(Utils::DetailsWidget::Expanded);
    QModelIndex idx = m_model->variableToIndex(link);
    focusIndex(idx);
}

void PreprocessorMacroWidget::updateSummaryText()
{
    Utils::NameValueItems items = m_model->userChanges();
    Utils::NameValueItem::sort(&items);

    QString text;
    for (const Utils::EnvironmentItem &item : items) {
        if (item.name != Utils::NameValueModel::tr("<VARIABLE>")) {
            text.append(QLatin1String("<br>"));
            if (item.operation == Utils::NameValueItem::Unset)
                text.append(tr("Unset <a href=\"%1\"><b>%1</b></a>").arg(item.name.toHtmlEscaped()));
            else
                text.append(tr("Set <a href=\"%1\"><b>%1</b></a> to <b>%2</b>")
                                .arg(item.name.toHtmlEscaped(), item.value.toHtmlEscaped()));
        }
    }

    m_detailsContainer->setSummaryText(text);
}

void PreprocessorMacroWidget::saveSettings()
{
    m_settings->saveMacros(m_model->userChanges());
}

} // namespace ClangPchManager
