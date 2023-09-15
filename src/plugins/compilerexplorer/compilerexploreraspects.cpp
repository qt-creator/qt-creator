// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "compilerexploreraspects.h"
#include "compilerexplorertr.h"

#include "api/library.h"

#include <utils/layoutbuilder.h>

#include <QComboBox>
#include <QCompleter>
#include <QFutureWatcher>
#include <QPushButton>
#include <QStackedLayout>

using namespace Utils;

namespace CompilerExplorer {

LibrarySelectionAspect::LibrarySelectionAspect(AspectContainer *container)
    : TypedAspect<QMap<QString, QString>>(container)
{}

void LibrarySelectionAspect::bufferToGui()
{
    if (!m_model)
        return;

    for (int i = 0; i < m_model->rowCount(); i++) {
        QModelIndex idx = m_model->index(i, 0);
        if (m_buffer.contains(qvariant_cast<Api::Library>(idx.data(LibraryData)).id))
            m_model->setData(idx,
                             m_buffer[qvariant_cast<Api::Library>(idx.data(LibraryData)).id],
                             SelectedVersion);
        else
            m_model->setData(idx, QVariant(), SelectedVersion);
    }

    handleGuiChanged();
}

bool LibrarySelectionAspect::guiToBuffer()
{
    if (!m_model)
        return false;

    auto oldBuffer = m_buffer;

    m_buffer.clear();

    for (int i = 0; i < m_model->rowCount(); i++) {
        if (m_model->item(i)->data(SelectedVersion).isValid()) {
            m_buffer.insert(qvariant_cast<Api::Library>(m_model->item(i)->data(LibraryData)).id,
                            m_model->item(i)->data(SelectedVersion).toString());
        }
    }
    return oldBuffer != m_buffer;
}

void LibrarySelectionAspect::addToLayout(Layouting::LayoutItem &parent)
{
    using namespace Layouting;

    QTC_ASSERT(m_fillCallback, return);

    auto cb = [this](const QList<QStandardItem *> &items) {
        for (QStandardItem *item : items)
            m_model->appendRow(item);

        bufferToGui();
    };

    if (!m_model) {
        m_model = new QStandardItemModel(this);

        connect(this, &LibrarySelectionAspect::refillRequested, this, [this, cb] {
            m_model->clear();
            m_fillCallback(cb);
        });

        m_fillCallback(cb);
    }

    QComboBox *nameCombo = new QComboBox();
    nameCombo->setInsertPolicy(QComboBox::InsertPolicy::NoInsert);
    nameCombo->setEditable(true);
    nameCombo->completer()->setCompletionMode(QCompleter::PopupCompletion);
    nameCombo->completer()->setFilterMode(Qt::MatchContains);

    nameCombo->setModel(m_model);

    QComboBox *versionCombo = new QComboBox();
    versionCombo->addItem("--");

    auto refreshVersionCombo = [nameCombo, versionCombo] {
        versionCombo->clear();
        versionCombo->addItem("--");
        QString selected = nameCombo->currentData(SelectedVersion).toString();
        Api::Library lib = qvariant_cast<Api::Library>(nameCombo->currentData(LibraryData));
        for (const auto &version : lib.versions) {
            versionCombo->addItem(version.version, version.id);
            if (version.id == selected)
                versionCombo->setCurrentIndex(versionCombo->count() - 1);
        }
    };

    refreshVersionCombo();

    connect(nameCombo, &QComboBox::currentIndexChanged, this, refreshVersionCombo);

    connect(versionCombo, &QComboBox::activated, this, [this, nameCombo, versionCombo] {
        m_model->setData(m_model->index(nameCombo->currentIndex(), 0),
                         versionCombo->currentData(),
                         SelectedVersion);
        handleGuiChanged();
    });

    QPushButton *clearBtn = new QPushButton("Clear All");
    connect(clearBtn, &QPushButton::clicked, clearBtn, [this, refreshVersionCombo] {
        for (int i = 0; i < m_model->rowCount(); i++)
            m_model->setData(m_model->index(i, 0), QVariant(), SelectedVersion);
        handleGuiChanged();
        refreshVersionCombo();
    });

    ElidingLabel *displayLabel = new ElidingLabel();

    auto updateLabel = [displayLabel, this] {
        QStringList libs;
        for (int i = 0; i < m_model->rowCount(); i++) {
            QModelIndex idx = m_model->index(i, 0);
            if (idx.data(SelectedVersion).isValid())
                libs.append(QString("%1 %2")
                                .arg(idx.data().toString())
                                .arg(idx.data(SelectedVersion).toString()));
        }
        if (libs.empty())
            displayLabel->setText(Tr::tr("No libraries selected"));
        else
            displayLabel->setText(libs.join(", "));
    };

    connect(m_model, &QStandardItemModel::itemChanged, displayLabel, updateLabel);

    updateLabel();

    QPushButton *editBtn = new QPushButton(Tr::tr("Edit"));

    QStackedLayout *stack{nullptr};

    // clang-format off
    auto s = Stack {
        bindTo(&stack),
        noMargin,
        Row { noMargin, displayLabel, editBtn }.emerge(),
        Row { noMargin, nameCombo, versionCombo, clearBtn }.emerge()
    }.emerge();
    // clang-format on
    connect(editBtn, &QPushButton::clicked, this, [stack] { stack->setCurrentIndex(1); });

    addLabeledItem(parent, s);
}

} // namespace CompilerExplorer
