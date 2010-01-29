/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "nicknamedialog.h"
#include "ui_nicknamedialog.h"

#include <QtCore/QDebug>
#include <QtCore/QFile>
#include <QtGui/QPushButton>
#include <QtGui/QStandardItemModel>
#include <QtGui/QSortFilterProxyModel>

enum { NickNameRole = Qt::UserRole + 1 };

namespace VCSBase {
namespace Internal {

// For code clarity, a struct representing the entries of a mail map file
// with parse and model functions.
struct NickNameEntry {
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
    QStandardItem *i1 = new QStandardItem(name);
    i1->setFlags(flags);
    i1->setData(nickNameData, NickNameRole);
    QStandardItem *i2 = new QStandardItem(email);
    i1->setFlags(flags);
    i2->setData(nickNameData, NickNameRole);
    QStandardItem *i3 = new QStandardItem(aliasName);
    i3->setFlags(flags);
    i3->setData(nickNameData, NickNameRole);
    QStandardItem *i4 = new QStandardItem(aliasEmail);
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
        m_ui(new Ui::NickNameDialog),
        m_model(model),
        m_filterModel(new QSortFilterProxyModel(this))
{
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    m_ui->setupUi(this);
    okButton()->setEnabled(false);

    // Populate model and grow tree to accommodate it
    m_filterModel->setSourceModel(model);
    m_filterModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    m_ui->filterTreeView->setModel(m_filterModel);
    const int columnCount = m_filterModel->columnCount();
    int treeWidth = 0;
    for (int c = 0; c < columnCount; c++) {
        m_ui->filterTreeView->resizeColumnToContents(c);
        treeWidth += m_ui->filterTreeView->columnWidth(c);
    }
    m_ui->filterTreeView->setMinimumWidth(treeWidth + 20);
    connect(m_ui->filterTreeView, SIGNAL(doubleClicked(QModelIndex)), this,
            SLOT(slotDoubleClicked(QModelIndex)));
    connect(m_ui->filterTreeView->selectionModel(), SIGNAL(currentRowChanged(QModelIndex,QModelIndex)),
            this, SLOT(slotCurrentItemChanged(QModelIndex)));
    connect(m_ui->filterLineEdit, SIGNAL(textChanged(QString)),
            m_filterModel, SLOT(setFilterFixedString(QString)));
}

NickNameDialog::~NickNameDialog()
{
    delete m_ui;
}

QPushButton *NickNameDialog::okButton() const
{
    return m_ui->buttonBox->button(QDialogButtonBox::Ok);
}

void NickNameDialog::slotCurrentItemChanged(const QModelIndex &index)
{
    okButton()->setEnabled(index.isValid());
}

void NickNameDialog::slotDoubleClicked(const QModelIndex &)
{
    if (okButton()->isEnabled())
        okButton()->animateClick();
}

QString NickNameDialog::nickName() const
{
    const QModelIndex index = m_ui->filterTreeView->selectionModel()->currentIndex();
    if (index.isValid()) {
        const QModelIndex sourceIndex = m_filterModel->mapToSource(index);
        if (const QStandardItem *item = m_model->itemFromIndex(sourceIndex))
            return NickNameEntry::nickNameOf(item);
    }
    return QString();
}

QStandardItemModel *NickNameDialog::createModel(QObject *parent)
{
    QStandardItemModel *model = new QStandardItemModel(parent);
    QStringList headers;
    headers << tr("Name") << tr("E-mail")
            << tr("Alias") << tr("Alias e-mail");
    model->setHorizontalHeaderLabels(headers);
    return model;
}

bool NickNameDialog::populateModelFromMailCapFile(const QString &fileName,
                                                  QStandardItemModel *model,
                                                  QString *errorMessage)
{
    if (const int rowCount = model->rowCount())
        model->removeRows(0, rowCount);
    if (fileName.isEmpty())
        return true;
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly|QIODevice::Text)) {
         *errorMessage = tr("Cannot open '%1': %2").arg(fileName, file.errorString());
         return false;
    }
    // Split into lines and read
    NickNameEntry entry;
    const QStringList lines = QString::fromUtf8(file.readAll()).trimmed().split(QLatin1Char('\n'));
    const int count = lines.size();
    for (int i = 0; i < count; i++) {
        if (entry.parse(lines.at(i))) {
            model->appendRow(entry.toModelRow());
        } else {
            qWarning("%s: Invalid mail cap entry at line %d: '%s'\n", qPrintable(fileName), i + 1, qPrintable(lines.at(i)));
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

}
}
