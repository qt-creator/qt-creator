// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "environmentwidget.h"

#include "projectexplorertr.h"

#include <coreplugin/fileutils.h>
#include <coreplugin/find/itemviewfind.h>

#include <utils/algorithm.h>
#include <utils/detailswidget.h>
#include <utils/environment.h>
#include <utils/environmentdialog.h>
#include <utils/environmentmodel.h>
#include <utils/headerviewstretcher.h>
#include <utils/hostosinfo.h>
#include <utils/itemviews.h>
#include <utils/namevaluevalidator.h>
#include <utils/qtcassert.h>
#include <utils/stringutils.h>
#include <utils/tooltip/tooltip.h>
#include <utils/utilstr.h>

#include <QDialogButtonBox>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLineEdit>
#include <QPushButton>
#include <QString>
#include <QStyledItemDelegate>
#include <QTreeView>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>

using namespace Utils;

namespace ProjectExplorer {

class PathTreeWidget : public QTreeWidget
{
public:
    QSize sizeHint() const override
    {
        return QSize(800, 600);
    }
};

class PathListDialog : public QDialog
{
public:
    PathListDialog(const QString &varName, const QString &paths, QWidget *parent) : QDialog(parent)
    {
        const auto mainLayout = new QVBoxLayout(this);
        const auto viewLayout = new QHBoxLayout;
        const auto buttonsLayout = new QVBoxLayout;
        const auto addButton = new QPushButton(Tr::tr("Add..."));
        const auto removeButton = new QPushButton(Tr::tr("Remove"));
        const auto editButton = new QPushButton(Tr::tr("Edit..."));
        buttonsLayout->addWidget(addButton);
        buttonsLayout->addWidget(removeButton);
        buttonsLayout->addWidget(editButton);
        buttonsLayout->addStretch(1);
        const auto buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok
                                                    | QDialogButtonBox::Cancel);
        viewLayout->addWidget(&m_view);
        viewLayout->addLayout(buttonsLayout);
        mainLayout->addLayout(viewLayout);
        mainLayout->addWidget(buttonBox);

        m_view.setHeaderLabel(varName);
        m_view.setDragDropMode(QAbstractItemView::InternalMove);
        const QStringList pathList = paths.split(Utils::HostOsInfo::pathListSeparator(),
                                                 Qt::SkipEmptyParts);
        for (const QString &path : pathList)
            addPath(path);

        connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
        connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
        connect(addButton, &QPushButton::clicked, this, [this] {
            const FilePath dir = FileUtils::getExistingDirectory(this, Tr::tr("Choose Directory"));
            if (!dir.isEmpty())
                addPath(dir.toUserOutput());
        });
        connect(removeButton, &QPushButton::clicked, this, [this] {
            const QList<QTreeWidgetItem *> selected = m_view.selectedItems();
            QTC_ASSERT(selected.count() == 1, return);
            delete selected.first();
        });
        connect(editButton, &QPushButton::clicked, this, [this] {
            const QList<QTreeWidgetItem *> selected = m_view.selectedItems();
            QTC_ASSERT(selected.count() == 1, return);
            m_view.editItem(selected.first(), 0);
        });
        const auto updateButtonStates = [this, removeButton, editButton] {
            const bool hasSelection = !m_view.selectedItems().isEmpty();
            removeButton->setEnabled(hasSelection);
            editButton->setEnabled(hasSelection);
        };
        connect(m_view.selectionModel(), &QItemSelectionModel::selectionChanged,
                this, updateButtonStates);
        updateButtonStates();
    }

    QString paths() const
    {
        QStringList pathList;
        for (int i = 0; i < m_view.topLevelItemCount(); ++i)
            pathList << m_view.topLevelItem(i)->text(0);
        return pathList.join(Utils::HostOsInfo::pathListSeparator());
    }

private:
    void addPath(const QString &path)
    {
        const auto item = new QTreeWidgetItem(&m_view, {path});
        item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable
                       | Qt::ItemIsDragEnabled);
    }

    PathTreeWidget m_view;
};

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
                edit, m_model, m_view, index, Tr::tr("Variable already exists.")));
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
    EnvironmentWidget::Type m_type = EnvironmentWidget::TypeLocal;
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
    d->m_type = type;
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
    const auto stretcher = new HeaderViewStretcher(d->m_environmentView->header(), 1);
    connect(d->m_model, &QAbstractItemModel::dataChanged,
            stretcher, &HeaderViewStretcher::softStretch);
    connect(d->m_model, &EnvironmentModel::userChangesChanged,
            stretcher, &HeaderViewStretcher::softStretch);
    d->m_environmentView->setSelectionMode(QAbstractItemView::SingleSelection);
    d->m_environmentView->setSelectionBehavior(QAbstractItemView::SelectItems);
    d->m_environmentView->setFrameShape(QFrame::NoFrame);
    QFrame *findWrapper = Core::ItemViewFind::createSearchableWrapper(d->m_environmentView, Core::ItemViewFind::LightColored);
    findWrapper->setFrameStyle(QFrame::StyledPanel);
    horizontalLayout->addWidget(findWrapper);

    auto buttonLayout = new QVBoxLayout();

    d->m_editButton = new QPushButton(this);
    d->m_editButton->setText(Tr::tr("Ed&it"));
    buttonLayout->addWidget(d->m_editButton);

    d->m_addButton = new QPushButton(this);
    d->m_addButton->setText(Tr::tr("&Add"));
    buttonLayout->addWidget(d->m_addButton);

    d->m_resetButton = new QPushButton(this);
    d->m_resetButton->setEnabled(false);
    d->m_resetButton->setText(Tr::tr("&Reset"));
    buttonLayout->addWidget(d->m_resetButton);

    d->m_unsetButton = new QPushButton(this);
    d->m_unsetButton->setEnabled(false);
    d->m_unsetButton->setText(Tr::tr("&Unset"));
    buttonLayout->addWidget(d->m_unsetButton);

    d->m_toggleButton = new QPushButton(Tr::tr("Disable"), this);
    buttonLayout->addWidget(d->m_toggleButton);
    connect(d->m_toggleButton, &QPushButton::clicked, this, [this] {
        d->m_model->toggleVariable(d->m_environmentView->currentIndex());
        updateButtons();
    });

    if (type == TypeLocal) {
        d->m_appendPathButton = new QPushButton(this);
        d->m_appendPathButton->setEnabled(false);
        d->m_appendPathButton->setText(Tr::tr("Append Path..."));
        buttonLayout->addWidget(d->m_appendPathButton);
        d->m_prependPathButton = new QPushButton(this);
        d->m_prependPathButton->setEnabled(false);
        d->m_prependPathButton->setText(Tr::tr("Prepend Path..."));
        buttonLayout->addWidget(d->m_prependPathButton);
        connect(d->m_appendPathButton, &QAbstractButton::clicked,
                this, &EnvironmentWidget::appendPathButtonClicked);
        connect(d->m_prependPathButton, &QAbstractButton::clicked,
                this, &EnvironmentWidget::prependPathButtonClicked);
    }

    d->m_batchEditButton = new QPushButton(this);
    d->m_batchEditButton->setText(Tr::tr("&Batch Edit..."));
    buttonLayout->addWidget(d->m_batchEditButton);

    d->m_terminalButton = new QPushButton(this);
    d->m_terminalButton->setText(Tr::tr("Open &Terminal"));
    d->m_terminalButton->setToolTip(Tr::tr("Open a terminal with this environment set up."));
    d->m_terminalButton->setEnabled(true);
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
            Core::FileUtils::openTerminal(FilePath::fromString(QDir::currentPath()), env);
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
    d->m_terminalButton->setVisible(bool(func));
}

void EnvironmentWidget::expand()
{
    d->m_detailsContainer->setState(Utils::DetailsWidget::Expanded);
}

void EnvironmentWidget::updateSummaryText()
{
    Utils::EnvironmentItems list = d->m_model->userChanges();
    Utils::EnvironmentItem::sort(&list);

    QString text;
    for (const Utils::EnvironmentItem &item : std::as_const(list)) {
        if (item.name != ::Utils::Tr::tr("<VARIABLE>")) {
            if (!d->m_baseEnvironmentText.isEmpty() || !text.isEmpty())
                text.append(QLatin1String("<br>"));
            switch (item.operation) {
            case Utils::EnvironmentItem::Unset:
                text.append(Tr::tr("Unset <a href=\"%1\"><b>%1</b></a>").arg(item.name.toHtmlEscaped()));
                break;
            case Utils::EnvironmentItem::SetEnabled:
                text.append(Tr::tr("Set <a href=\"%1\"><b>%1</b></a> to <b>%2</b>").arg(item.name.toHtmlEscaped(), item.value.toHtmlEscaped()));
                break;
            case Utils::EnvironmentItem::Append:
                text.append(Tr::tr("Append <b>%2</b> to <a href=\"%1\"><b>%1</b></a>").arg(item.name.toHtmlEscaped(), item.value.toHtmlEscaped()));
                break;
            case Utils::EnvironmentItem::Prepend:
                text.append(Tr::tr("Prepend <b>%2</b> to <a href=\"%1\"><b>%1</b></a>").arg(item.name.toHtmlEscaped(), item.value.toHtmlEscaped()));
                break;
            case Utils::EnvironmentItem::SetDisabled:
                text.append(Tr::tr("Set <a href=\"%1\"><b>%1</b></a> to <b>%2</b> [disabled]").arg(item.name.toHtmlEscaped(), item.value.toHtmlEscaped()));
                break;
            }
        }
    }

    if (text.isEmpty()) {
        //: %1 is "System Environment" or some such.
        if (!d->m_baseEnvironmentText.isEmpty())
            text.prepend(Tr::tr("Use <b>%1</b>").arg(d->m_baseEnvironmentText));
        else
            text.prepend(Tr::tr("<b>No environment changes</b>"));
    } else {
        //: Yup, word puzzle. The Set/Unset phrases above are appended to this.
        //: %1 is "System Environment" or some such.
        if (!d->m_baseEnvironmentText.isEmpty())
            text.prepend(Tr::tr("Use <b>%1</b> and").arg(d->m_baseEnvironmentText));
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
    const QModelIndex current = d->m_environmentView->currentIndex();
    if (current.column() == 1
            && d->m_type == TypeLocal
            && d->m_model->currentEntryIsPathList(current)) {
        PathListDialog dlg(d->m_model->indexToVariable(current),
                           d->m_model->data(current).toString(), this);
        if (dlg.exec() == QDialog::Accepted)
            d->m_model->setData(current, dlg.paths());
    } else {
        d->m_environmentView->edit(current);
    }
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

void EnvironmentWidget::amendPathList(Utils::NameValueItem::Operation op)
{
    const QString varName = d->m_model->indexToVariable(d->m_environmentView->currentIndex());
    const FilePath dir = FileUtils::getExistingDirectory(this, Tr::tr("Choose Directory"));
    if (dir.isEmpty())
        return;
    Utils::NameValueItems changes = d->m_model->userChanges();
    changes.append({varName, dir.toUserOutput(), op});
    d->m_model->setUserChanges(changes);
}

void EnvironmentWidget::appendPathButtonClicked()
{
    amendPathList(Utils::NameValueItem::Append);
}

void EnvironmentWidget::prependPathButtonClicked()
{
    amendPathList(Utils::NameValueItem::Prepend);
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
        d->m_toggleButton->setText(d->m_model->isEnabled(name) ? Tr::tr("Disable") : Tr::tr("Enable"));
    } else {
        d->m_editButton->setEnabled(false);
        d->m_resetButton->setEnabled(false);
        d->m_unsetButton->setEnabled(false);
        d->m_toggleButton->setEnabled(false);
        d->m_toggleButton->setText(Tr::tr("Disable"));
    }
    if (d->m_appendPathButton) {
        const bool isPathList = d->m_model->currentEntryIsPathList(current);
        d->m_appendPathButton->setEnabled(isPathList);
        d->m_prependPathButton->setEnabled(isPathList);
    }
}

void EnvironmentWidget::invalidateCurrentIndex()
{
    environmentCurrentIndexChanged(QModelIndex());
}

} // namespace ProjectExplorer
