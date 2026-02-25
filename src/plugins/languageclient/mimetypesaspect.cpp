// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "mimetypesaspect.h"

#include "languageclienttr.h"

#include <coreplugin/icore.h>
#include <utils/algorithm.h>
#include <utils/fancylineedit.h>
#include <utils/layoutbuilder.h>
#include <utils/mimeutils.h>

#include <QDialog>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QListView>
#include <QPointer>
#include <QPushButton>
#include <QSortFilterProxyModel>
#include <QStringListModel>
#include <QVBoxLayout>

namespace LanguageClient {

static constexpr char filterSeparator = ';';

class MimeTypeModel final : public QStringListModel
{
public:
    using QStringListModel::QStringListModel;

    QVariant data(const QModelIndex &index, int role) const final
    {
        if (index.isValid() && role == Qt::CheckStateRole)
            return m_selectedMimeTypes.contains(index.data().toString()) ? Qt::Checked
                                                                         : Qt::Unchecked;
        return QStringListModel::data(index, role);
    }

    bool setData(const QModelIndex &index, const QVariant &value, int role) final
    {
        if (index.isValid() && role == Qt::CheckStateRole) {
            const QString mimeType = index.data().toString();
            if (value.toInt() == Qt::Checked) {
                if (!m_selectedMimeTypes.contains(mimeType))
                    m_selectedMimeTypes.append(mimeType);
            } else {
                m_selectedMimeTypes.removeAll(mimeType);
            }
            return true;
        }
        return QStringListModel::setData(index, value, role);
    }

    Qt::ItemFlags flags(const QModelIndex &index) const final
    {
        if (!index.isValid())
            return Qt::NoItemFlags;
        return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsUserCheckable;
    }

    QStringList m_selectedMimeTypes;
};

class MimeTypeDialog : public QDialog
{
public:
    explicit MimeTypeDialog(const QStringList &selectedMimeTypes, QWidget *parent = nullptr)
        : QDialog(parent)
    {
        setWindowTitle(Tr::tr("Select MIME Types"));

        auto mainLayout = new QVBoxLayout;
        auto filter = new Utils::FancyLineEdit(this);
        filter->setFiltering(true);
        filter->setPlaceholderText(Tr::tr("Filter"));
        mainLayout->addWidget(filter);

        auto listView = new QListView(this);
        mainLayout->addWidget(listView);

        auto buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
        mainLayout->addWidget(buttons);
        setLayout(mainLayout);

        connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
        connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

        auto proxy = new QSortFilterProxyModel(this);
        m_mimeTypeModel = new MimeTypeModel(
            Utils::transform(Utils::allMimeTypes(), &Utils::MimeType::name), this);
        m_mimeTypeModel->m_selectedMimeTypes = selectedMimeTypes;
        proxy->setSourceModel(m_mimeTypeModel);
        proxy->sort(0);
        connect(filter, &QLineEdit::textChanged, proxy, &QSortFilterProxyModel::setFilterWildcard);
        listView->setModel(proxy);

        setModal(true);
    }

    MimeTypeDialog(const MimeTypeDialog &) = delete;
    MimeTypeDialog(MimeTypeDialog &&) = delete;
    MimeTypeDialog &operator=(const MimeTypeDialog &) = delete;
    MimeTypeDialog &operator=(MimeTypeDialog &&) = delete;

    QStringList mimeTypes() const { return m_mimeTypeModel->m_selectedMimeTypes; }

private:
    MimeTypeModel *m_mimeTypeModel = nullptr;
};

namespace Internal {

class MimeTypesAspectPrivate
{
public:
    QPointer<QLabel> m_displayLabel;
};

} // namespace Internal

MimeTypesAspect::MimeTypesAspect(Utils::AspectContainer *container)
    : TypedAspect(container)
    , d(new Internal::MimeTypesAspectPrivate)
{
    setDefaultValue({});
    setLabelText(Tr::tr("Language:"));

    // TODO: remove once the lsp settings are fully aspectified
    connect(this, &Utils::BaseAspect::volatileValueChanged, this, &Utils::markSettingsDirty);
}

MimeTypesAspect::~MimeTypesAspect() = default;

void MimeTypesAspect::addToLayoutImpl(Layouting::Layout &parent)
{
    auto displayLabel = createSubWidget<QLabel>();
    auto button = createSubWidget<QPushButton>(Tr::tr("Set MIME Types..."));

    d->m_displayLabel = displayLabel;

    connect(button, &QPushButton::pressed, this, &MimeTypesAspect::showMimeTypeDialog);

    volatileValueToGui();

    auto container = new QWidget;
    auto hLayout = new QHBoxLayout(container);
    hLayout->setContentsMargins(0, 0, 0, 0);
    hLayout->addWidget(displayLabel);
    hLayout->addStretch();
    hLayout->addWidget(button);

    addLabeledItem(parent, container);
}

bool MimeTypesAspect::guiToVolatileValue()
{
    if (!d->m_displayLabel)
        return false;
    const QStringList newValue
        = d->m_displayLabel->text().split(filterSeparator, Qt::SkipEmptyParts);
    return updateStorage(m_volatileValue, newValue);
}

void MimeTypesAspect::volatileValueToGui()
{
    if (d->m_displayLabel)
        d->m_displayLabel->setText(m_volatileValue.join(filterSeparator));
}

void MimeTypesAspect::showMimeTypeDialog()
{
    MimeTypeDialog dialog(m_volatileValue, Core::ICore::dialogParent());
    if (dialog.exec() == QDialog::Rejected)
        return;
    if (d->m_displayLabel)
        d->m_displayLabel->setText(dialog.mimeTypes().join(filterSeparator));
    handleGuiChanged();
}

} // namespace LanguageClient
