// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "nicknamedialog.h"

#include "vcsbasetr.h"

#include <utils/fancylineedit.h>
#include <utils/fileutils.h>
#include <utils/itemviews.h>
#include <utils/layoutbuilder.h>

#include <QDebug>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QStandardItemModel>
#include <QSortFilterProxyModel>

using namespace Utils;

enum { NickNameRole = Qt::UserRole + 1 };

/*!
    \class VcsBase::Internal::NickNameDialog

    \brief The NickNameDialog class shows users from a mail cap file.

    Manages a list of users read from an extended
    mail cap file, consisting of 4 columns:  "Name Mail [AliasName [AliasMail]]".

    The names can be used for insertion into "RevBy:" fields; aliases will
    be preferred.
*/

namespace VcsBase::Internal {

// For code clarity, a struct representing the entries of a mail map file
// with parse and model functions.
class NickNameEntry
{
public:
    void clear();
    bool parse(const QString &);
    QString nickName() const;
    QList<QStandardItem *> toModelRow() const;
    static QString nickNameOf(const QStandardItem *item);

    QString name;
    QString email;
    QString aliasName;
    QString aliasEmail;
};

void NickNameEntry::clear()
{
    name.clear();
    email.clear();
    aliasName.clear();
    aliasEmail.clear();
}

// Parse "Hans Mustermann <HM@acme.de> [Alias [<alias@acme.de>]]"

bool NickNameEntry::parse(const QString &l)
{
    clear();
    const QChar lessThan = QLatin1Char('<');
    const QChar greaterThan = QLatin1Char('>');
    // Get first name/mail pair
    int mailPos = l.indexOf(lessThan);
    if (mailPos == -1)
        return false;
    name = l.mid(0, mailPos).trimmed();
    mailPos++;
    const int mailEndPos = l.indexOf(greaterThan, mailPos);
    if (mailEndPos == -1)
        return false;
    email = l.mid(mailPos, mailEndPos - mailPos);
    // get optional 2nd name/mail pair
    const int aliasNameStart = mailEndPos + 1;
    if (aliasNameStart >= l.size())
        return true;
    int aliasMailPos = l.indexOf(lessThan, aliasNameStart);
    if (aliasMailPos == -1) {
        aliasName =l.mid(aliasNameStart, l.size() -  aliasNameStart).trimmed();
        return true;
    }
    aliasName = l.mid(aliasNameStart, aliasMailPos - aliasNameStart).trimmed();
    aliasMailPos++;
    const int aliasMailEndPos = l.indexOf(greaterThan, aliasMailPos);
    if (aliasMailEndPos == -1)
        return true;
    aliasEmail = l.mid(aliasMailPos, aliasMailEndPos - aliasMailPos);
    return true;
}

// Format "Hans Mustermann <HM@acme.de>"
static inline QString formatNick(const QString &name, const QString &email)
{
    QString rc = name;
    if (!email.isEmpty()) {
        rc += QLatin1String(" <");
        rc += email;
        rc += QLatin1Char('>');
    }
    return rc;
}

QString NickNameEntry::nickName() const
{
    return aliasName.isEmpty() ? formatNick(name, email) : formatNick(aliasName, aliasEmail);
}

QList<QStandardItem *> NickNameEntry::toModelRow() const
{
    const QVariant nickNameData = nickName();
    const Qt::ItemFlags flags = Qt::ItemIsSelectable|Qt::ItemIsEnabled;
    auto i1 = new QStandardItem(name);
    i1->setFlags(flags);
    i1->setData(nickNameData, NickNameRole);
    auto i2 = new QStandardItem(email);
    i1->setFlags(flags);
    i2->setData(nickNameData, NickNameRole);
    auto i3 = new QStandardItem(aliasName);
    i3->setFlags(flags);
    i3->setData(nickNameData, NickNameRole);
    auto i4 = new QStandardItem(aliasEmail);
    i4->setFlags(flags);
    i4->setData(nickNameData, NickNameRole);
    QList<QStandardItem *> row;
    row << i1 << i2 << i3 << i4;
    return row;
}

QString NickNameEntry::nickNameOf(const QStandardItem *item)
{
    return item->data(NickNameRole).toString();
}

QDebug operator<<(QDebug d, const NickNameEntry &e)
{
    d.nospace() << "Name='" << e.name  << "' Mail='" << e.email
            << " Alias='" << e.aliasName << " AliasEmail='" << e.aliasEmail << "'\n";
    return  d;
}

NickNameDialog::NickNameDialog(QStandardItemModel *model, QWidget *parent) :
        QDialog(parent),
        m_model(model),
        m_filterModel(new QSortFilterProxyModel(this))
{
    auto filterLineEdit = new FancyLineEdit;

    m_filterTreeView = new TreeView;

    m_buttonBox = new QDialogButtonBox(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);

    okButton()->setEnabled(false);

    // Populate model and grow tree to accommodate it
    m_filterModel->setSourceModel(model);
    m_filterModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    m_filterTreeView->setModel(m_filterModel);
    m_filterTreeView->setActivationMode(DoubleClickActivation);
    const int columnCount = m_filterModel->columnCount();
    int treeWidth = 0;
    for (int c = 0; c < columnCount; c++) {
        m_filterTreeView->resizeColumnToContents(c);
        treeWidth += m_filterTreeView->columnWidth(c);
    }
    m_filterTreeView->setMinimumWidth(treeWidth + 20);
    filterLineEdit->setFiltering(true);

    using namespace Layouting;
    Column {
        filterLineEdit,
        m_filterTreeView,
        m_buttonBox
    }.attachTo(this);

    connect(m_buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    connect(m_filterTreeView, &QAbstractItemView::activated, this,
            &NickNameDialog::slotActivated);
    connect(m_filterTreeView->selectionModel(), &QItemSelectionModel::currentRowChanged,
            this, &NickNameDialog::slotCurrentItemChanged);
    connect(filterLineEdit, &FancyLineEdit::filterChanged,
            m_filterModel, &QSortFilterProxyModel::setFilterFixedString);
}

NickNameDialog::~NickNameDialog() = default;

QPushButton *NickNameDialog::okButton() const
{
    return m_buttonBox->button(QDialogButtonBox::Ok);
}

void NickNameDialog::slotCurrentItemChanged(const QModelIndex &index)
{
    okButton()->setEnabled(index.isValid());
}

void NickNameDialog::slotActivated(const QModelIndex &)
{
    if (okButton()->isEnabled())
        okButton()->click();
}

QString NickNameDialog::nickName() const
{
    const QModelIndex index = m_filterTreeView->selectionModel()->currentIndex();
    if (index.isValid()) {
        const QModelIndex sourceIndex = m_filterModel->mapToSource(index);
        if (const QStandardItem *item = m_model->itemFromIndex(sourceIndex))
            return NickNameEntry::nickNameOf(item);
    }
    return {};
}

QStandardItemModel *NickNameDialog::createModel(QObject *parent)
{
    auto model = new QStandardItemModel(parent);
    QStringList headers = {Tr::tr("Name"), Tr::tr("Email"), Tr::tr("Alias"), Tr::tr("Alias email")};
    model->setHorizontalHeaderLabels(headers);
    return model;
}

bool NickNameDialog::populateModelFromMailCapFile(const FilePath &fileName,
                                                  QStandardItemModel *model,
                                                  QString *errorMessage)
{
    if (const int rowCount = model->rowCount())
        model->removeRows(0, rowCount);
    if (fileName.isEmpty())
        return true;
    FileReader reader;
    if (!reader.fetch(fileName, QIODevice::Text, errorMessage))
         return false;
    // Split into lines and read
    NickNameEntry entry;
    const QStringList lines = QString::fromUtf8(reader.data()).trimmed().split(QLatin1Char('\n'));
    const int count = lines.size();
    for (int i = 0; i < count; i++) {
        if (entry.parse(lines.at(i))) {
            model->appendRow(entry.toModelRow());
        } else {
            qWarning("%s: Invalid mail cap entry at line %d: '%s'\n",
                     qPrintable(fileName.toUserOutput()),
                     i + 1, qPrintable(lines.at(i)));
        }
    }
    model->sort(0);
    return true;
}

QStringList NickNameDialog::nickNameList(const QStandardItemModel *model)
{
    QStringList  rc;
    const int rowCount = model->rowCount();
    for (int r = 0; r < rowCount; r++)
        rc.push_back(NickNameEntry::nickNameOf(model->item(r, 0)));
    return rc;
}

} // VcsBase::Internal
