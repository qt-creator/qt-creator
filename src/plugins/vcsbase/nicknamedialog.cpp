/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "nicknamedialog.h"
#include "ui_nicknamedialog.h"

#include <QtCore/QDebug>
#include <QtCore/QFile>
#include <QtGui/QPushButton>
#include <QtGui/QStandardItemModel>
#include <QtGui/QSortFilterProxyModel>

namespace VCSBase {
namespace Internal {

struct NickEntry {
    void clear();
    bool parse(const QString &);
    QString nickName() const;

    QString name;
    QString email;
    QString aliasName;
    QString aliasEmail;
};

void NickEntry::clear()
{
    name.clear();
    email.clear();
    aliasName.clear();
    aliasEmail.clear();
}

// Parse "Hans Mustermann <HM@acme.de> [Alias [<alias@acme.de>]]"

bool NickEntry::parse(const QString &l)
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

QString NickEntry::nickName() const
{
    return aliasName.isEmpty() ? formatNick(name, email) : formatNick(aliasName, aliasEmail);
}

// Sort by name
bool operator<(const NickEntry &n1,  const NickEntry &n2)
{
    return n1.name < n2.name;
}

QDebug operator<<(QDebug d, const NickEntry &e)
{
    d.nospace() << "Name='" << e.name  << "' Mail='" << e.email
            << " Alias='" << e.aliasName << " AliasEmail='" << e.aliasEmail << "'\n";
    return  d;
}

// Globally cached list
static QList<NickEntry> &nickList()
{
    static QList<NickEntry> rc;
    return rc;
}

// Create a model populated with the names
static QStandardItemModel *createModel(QObject *parent)
{
    QStandardItemModel *rc = new QStandardItemModel(parent);
    QStringList headers;
    headers << NickNameDialog::tr("Name")
            << NickNameDialog::tr("E-mail")
            << NickNameDialog::tr("Alias")
            << NickNameDialog::tr("Alias e-mail");
    rc->setHorizontalHeaderLabels(headers);
    foreach(const NickEntry &ne, nickList()) {
        QList<QStandardItem *> row;
        row.push_back(new QStandardItem(ne.name));
        row.push_back(new QStandardItem(ne.email));
        row.push_back(new QStandardItem(ne.aliasName));
        row.push_back(new QStandardItem(ne.aliasEmail));
        rc->appendRow(row);
    }
    return rc;
}

NickNameDialog::NickNameDialog(QWidget *parent) :
        QDialog(parent),
        m_ui(new Ui::NickNameDialog),
        m_filterModel(new QSortFilterProxyModel(this))
{
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    m_ui->setupUi(this);
    okButton()->setEnabled(false);

    // Populate model and grow tree to accommodate it
    m_filterModel->setSourceModel(createModel(this));
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
        return nickList().at(sourceIndex.row()).nickName();
    }
    return QString();
}

void NickNameDialog::clearNickNames()
{
    nickList().clear();
}

bool NickNameDialog::readNickNamesFromMailCapFile(const QString &fileName, QString *errorMessage)
{
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly|QIODevice::Text)) {
         *errorMessage = tr("Cannot open '%1': %2").arg(fileName, file.errorString());
         return false;
    }
    // Split into lines and read
    QList<NickEntry> &nl = nickList();
    nl.clear();
    NickEntry entry;
    const QStringList lines = QString::fromUtf8(file.readAll()).trimmed().split(QLatin1Char('\n'));
    const int count = lines.size();
    for (int i = 0; i < count; i++) {
        if (entry.parse(lines.at(i))) {
            nl.push_back(entry);
        } else {
            qWarning("%s: Invalid mail cap entry at line %d: '%s'\n", qPrintable(fileName), i + 1, qPrintable(lines.at(i)));
        }
    }
    qStableSort(nl);
    return true;
}

QStringList NickNameDialog::nickNameList()
{
    QStringList  rc;
    foreach(const NickEntry &ne, nickList())
        rc.push_back(ne.nickName());
    return rc;
}

}
}
