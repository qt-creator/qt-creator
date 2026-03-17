// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "envvarseparatoraspect.h"

#include "coreplugintr.h"

#include <utils/environment.h>
#include <utils/itemviews.h>
#include <utils/layoutbuilder.h>
#include <utils/namevaluedictionary.h>
#include <utils/treemodel.h>

#include <QDialog>
#include <QDialogButtonBox>
#include <QPushButton>

using namespace Utils;

namespace Core::Internal {

class EnvVarSeparatorItem : public TreeItem
{
public:
    EnvVarSeparatorItem(const QString &var, const QString &sep)
        : m_var(var)
        , m_sep(sep)
    {}

    QString var() const { return m_var; }
    QString sep() const { return m_sep; }

private:
    QVariant data(int column, int role) const override
    {
        if (role != Qt::DisplayRole && role != Qt::EditRole)
            return {};
        return column == 0 ? m_var : m_sep;
    }

    bool setData(int column, const QVariant &data, int) override
    {
        if (column == 0) {
            m_var = data.toString();
            return true;
        }
        if (column == 1) {
            m_sep = data.toString();
            return true;
        }
        return false;
    }

    Qt::ItemFlags flags(int column) const override
    {
        return TreeItem::flags(column) | Qt::ItemIsEditable;
    }

    QString m_var;
    QString m_sep;
};

class EnvVarSeparatorsDialog : public QDialog
{
public:
    EnvVarSeparatorsDialog(const NameValueDictionary &separators, QWidget *parent)
        : QDialog(parent)
    {
        const QString explanation = Tr::tr(
            "For environment variables with list semantics that do not use the standard path list\n"
            "separator, you need to configure the respective separators here if you plan to\n"
            "aggregate them from several places (for instance from the kit and from the project).");

        m_model.setHeader({Tr::tr("Variable"), Tr::tr("Separator")});
        for (auto it = separators.begin(); it != separators.end(); ++it)
            m_model.rootItem()->appendChild(new EnvVarSeparatorItem(it.key(), it.value()));

        const auto view = new TreeView(this);
        view->setSelectionMode(QAbstractItemView::SingleSelection);
        view->setSelectionBehavior(QAbstractItemView::SelectRows);
        view->setModel(&m_model);

        const auto addButton = new QPushButton("&Add");
        connect(addButton, &QPushButton::clicked, this, [this] {
            m_model.rootItem()->appendChild(new EnvVarSeparatorItem("CUSTOM_VAR", {}));
        });
        const auto removeButton = new QPushButton("&Remove");
        connect(removeButton, &QPushButton::clicked, this, [this, view] {
            const QModelIndexList selected = view->selectionModel()->selectedRows();
            if (selected.size() == 1)
                m_model.rootItem()->removeChildAt(selected.first().row());
        });
        const auto updateRemoveButtonState = [view, removeButton] {
            removeButton->setEnabled(view->selectionModel()->hasSelection());
        };
        connect(
            view->selectionModel(),
            &QItemSelectionModel::selectionChanged,
            this,
            updateRemoveButtonState);
        updateRemoveButtonState();

        const auto buttonBox
            = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
        connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
        connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

        using namespace Layouting;
        // clang-format off
        Column {
            explanation,
            Row {
                Column { view },
                Column {
                    addButton,
                    removeButton,
                    st
                }
            },
            buttonBox
        }.attachTo(this);
        // clang-format on
    }

    NameValueDictionary separators() const
    {
        NameValueDictionary seps;
        const TreeItem *const root = m_model.rootItem();
        for (int i = 0; i < root->childCount(); ++i) {
            const auto item = static_cast<EnvVarSeparatorItem *>(root->childAt(i));
            seps.set(item->var(), item->sep());
        }
        return seps;
    }

private:
    TreeModel<TreeItem, EnvVarSeparatorItem> m_model;
};

EnvVarSeparatorAspect::EnvVarSeparatorAspect(Utils::AspectContainer *container)
    : StringListAspect(container)
{
    Environment::setListSeparatorProvider([this](const QString &varName) -> std::optional<QString> {
        const NameValueDictionary seps = NameValueDictionary(value());
        if (const auto it = seps.find(varName); it != seps.end())
            return it.value();
        return {};
    });
}

void EnvVarSeparatorAspect::addToLayoutImpl(Layouting::Layout &parent)
{
    auto label = createLabel();
    if (label)
        parent.addItem(label);

    auto separatorsLabel = new ElidingLabel();
    auto updateSeparatorsLabel = [this, separatorsLabel]() {
        const NameValueDictionary seps = NameValueDictionary(volatileValue());
        QStringList parts;
        for (auto it = seps.begin(); it != seps.end(); ++it)
            parts.append(QString("%1: \"%2\"").arg(it.key()).arg(it.value()));
        separatorsLabel->setText(parts.join(", "));
    };
    updateSeparatorsLabel();
    connect(this, &EnvVarSeparatorAspect::volatileValueChanged, this, updateSeparatorsLabel);

    QPushButton *changeButton = new QPushButton(Tr::tr("Change..."));
    changeButton->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);

    connect(changeButton, &QPushButton::clicked, this, [this, changeButton]() {
        EnvVarSeparatorsDialog dlg(NameValueDictionary(volatileValue()), changeButton);
        if (dlg.exec() == QDialog::Accepted) {
            const QStringList newValues = dlg.separators().toStringList();
            if (volatileValue() == newValues)
                return;
            setVolatileValue(newValues);
        }
    });

    parent.addItem(separatorsLabel);
    parent.addItem(changeButton);
}

} // namespace Core
