// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "remotedialog.h"

#include "gitclient.h"
#include "gitplugin.h"
#include "gittr.h"
#include "remotemodel.h"

#include <utils/fancylineedit.h>
#include <utils/headerviewstretcher.h>
#include <utils/layoutbuilder.h>

#include <vcsbase/vcsoutputwindow.h>

#include <QApplication>
#include <QApplication>
#include <QDialogButtonBox>
#include <QGroupBox>
#include <QHeaderView>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QRegularExpression>
#include <QTreeView>

using namespace Utils;

namespace Git::Internal {

// --------------------------------------------------------------------------
// RemoteAdditionDialog:
// --------------------------------------------------------------------------

class RemoteAdditionDialog : public QDialog
{
public:
    RemoteAdditionDialog(const QStringList &remoteNames) :
        m_invalidRemoteNameChars(GitPlugin::invalidBranchAndRemoteNamePattern()),
        m_remoteNames(remoteNames)
    {
        resize(381, 93);

        m_nameEdit = new FancyLineEdit(this);
        m_nameEdit->setHistoryCompleter("Git.RemoteNames");
        m_nameEdit->setValidationFunction([this](FancyLineEdit *edit, QString *errorMessage) {
            if (!edit)
                return false;

            QString input = edit->text();
            edit->setText(input.replace(m_invalidRemoteNameChars, "_"));

            // "Intermediate" patterns, may change to Acceptable when user edits further:

            if (input.endsWith(".lock")) //..may not end with ".lock"
                return false;

            if (input.endsWith('.')) // no dot at the end (but allowed in the middle)
                return false;

            if (input.endsWith('/')) // no slash at the end (but allowed in the middle)
                return false;

            if (m_remoteNames.contains(input)) {
                if (errorMessage)
                    *errorMessage = Tr::tr("A remote with the name \"%1\" already exists.").arg(input);
                return false;
            }

            // is a valid remote name
            return !input.isEmpty();
        });

        m_urlEdit = new FancyLineEdit(this);
        m_urlEdit->setHistoryCompleter("Git.RemoteUrls");
        m_urlEdit->setValidationFunction([](FancyLineEdit *edit, QString *errorMessage) {
            if (!edit || edit->text().isEmpty())
                return false;

            const GitRemote r(edit->text());
            if (!r.isValid && errorMessage)
                *errorMessage = Tr::tr("The URL may not be valid.");

            return r.isValid;
        });

        auto buttonBox = new QDialogButtonBox(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);
        buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);

        using namespace Layouting;
        Grid {
            Tr::tr("Name:"), m_nameEdit, br,
            Tr::tr("URL:"), m_urlEdit, br,
            Span(2, buttonBox)
        }.attachTo(this);

        connect(m_nameEdit, &QLineEdit::textChanged, this, [this, buttonBox] {
            buttonBox->button(QDialogButtonBox::Ok)->setEnabled(m_nameEdit->isValid());
        });

        connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
        connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    }

    QString remoteName() const
    {
        return m_nameEdit->text();
    }

    QString remoteUrl() const
    {
        return m_urlEdit->text();
    }

private:
    FancyLineEdit *m_nameEdit;
    FancyLineEdit *m_urlEdit;

    const QRegularExpression m_invalidRemoteNameChars;
    QStringList m_remoteNames;
};


// --------------------------------------------------------------------------
// RemoteDialog:
// --------------------------------------------------------------------------


RemoteDialog::RemoteDialog(QWidget *parent) :
    QDialog(parent),
    m_remoteModel(new RemoteModel(this))
{
    setModal(false);
    setAttribute(Qt::WA_DeleteOnClose, true); // Do not update unnecessarily
        setWindowTitle(Tr::tr("Remotes"));

    m_repositoryLabel = new QLabel;

    auto refreshButton = new QPushButton(Tr::tr("Re&fresh"));
    refreshButton->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);

    m_remoteView = new QTreeView;
    m_remoteView->setMinimumSize(QSize(0, 100));
    m_remoteView->setEditTriggers(QAbstractItemView::AnyKeyPressed|QAbstractItemView::DoubleClicked|QAbstractItemView::EditKeyPressed);
    m_remoteView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_remoteView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_remoteView->setRootIsDecorated(false);
    m_remoteView->setUniformRowHeights(true);
    m_remoteView->setModel(m_remoteModel);
    new HeaderViewStretcher(m_remoteView->header(), 1);

    m_addButton = new QPushButton(Tr::tr("&Add..."));
    m_addButton->setAutoDefault(false);

    m_fetchButton = new QPushButton(Tr::tr("F&etch"));

    m_pushButton = new QPushButton(Tr::tr("&Push"));

    m_removeButton = new QPushButton(Tr::tr("&Remove"));
    m_removeButton->setAutoDefault(false);

    auto buttonBox = new QDialogButtonBox(QDialogButtonBox::Close);

    using namespace Layouting;
    Column {
        Group {
            Row { m_repositoryLabel, refreshButton }
        },
        Group {
            title(Tr::tr("Remotes")),
            Column {
                m_remoteView,
                Row { st, m_addButton, m_fetchButton, m_pushButton, m_removeButton }
            }
        },
        buttonBox,
    }.attachTo(this);

    connect(m_addButton, &QPushButton::clicked, this, &RemoteDialog::addRemote);
    connect(m_fetchButton, &QPushButton::clicked, this, &RemoteDialog::fetchFromRemote);
    connect(m_pushButton, &QPushButton::clicked, this, &RemoteDialog::pushToRemote);
    connect(m_removeButton, &QPushButton::clicked, this, &RemoteDialog::removeRemote);
    connect(refreshButton, &QPushButton::clicked, this, &RemoteDialog::refreshRemotes);

    connect(m_remoteView->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &RemoteDialog::updateButtonState);
    connect(m_remoteModel, &RemoteModel::refreshed,
            this, &RemoteDialog::updateButtonState);

    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    updateButtonState();
}

RemoteDialog::~RemoteDialog() = default;

void RemoteDialog::refresh(const FilePath &repository, bool force)
{
    if (m_remoteModel->workingDirectory() == repository && !force)
        return;
    // Refresh
    m_repositoryLabel->setText(GitPlugin::msgRepositoryLabel(repository));
    if (repository.isEmpty()) {
        m_remoteModel->clear();
    } else {
        QString errorMessage;
        if (!m_remoteModel->refresh(repository, &errorMessage))
            VcsBase::VcsOutputWindow::appendError(errorMessage);
    }
}

void RemoteDialog::refreshRemotes()
{
    refresh(m_remoteModel->workingDirectory(), true);
}

void RemoteDialog::addRemote()
{
    RemoteAdditionDialog addDialog(m_remoteModel->allRemoteNames());
    if (addDialog.exec() != QDialog::Accepted)
        return;

    m_remoteModel->addRemote(addDialog.remoteName(), addDialog.remoteUrl());
}

void RemoteDialog::removeRemote()
{
    const QModelIndexList indexList = m_remoteView->selectionModel()->selectedIndexes();
    if (indexList.isEmpty())
        return;

    int row = indexList.at(0).row();
    const QString remoteName = m_remoteModel->remoteName(row);
    if (QMessageBox::question(this, Tr::tr("Delete Remote"),
                              Tr::tr("Would you like to delete the remote \"%1\"?").arg(remoteName),
                              QMessageBox::Yes | QMessageBox::No,
                              QMessageBox::Yes) == QMessageBox::Yes) {
        m_remoteModel->removeRemote(row);
    }
}

void RemoteDialog::pushToRemote()
{
    const QModelIndexList indexList = m_remoteView->selectionModel()->selectedIndexes();
    if (indexList.isEmpty())
        return;

    const int row = indexList.at(0).row();
    const QString remoteName = m_remoteModel->remoteName(row);
    gitClient().push(m_remoteModel->workingDirectory(), {remoteName});
}

void RemoteDialog::fetchFromRemote()
{
    const QModelIndexList indexList = m_remoteView->selectionModel()->selectedIndexes();
    if (indexList.isEmpty())
        return;

    int row = indexList.at(0).row();
    const QString remoteName = m_remoteModel->remoteName(row);
    gitClient().fetch(m_remoteModel->workingDirectory(), remoteName);
}

void RemoteDialog::updateButtonState()
{
    const QModelIndexList indexList = m_remoteView->selectionModel()->selectedIndexes();

    const bool haveSelection = !indexList.isEmpty();
    m_addButton->setEnabled(true);
    m_fetchButton->setEnabled(haveSelection);
    m_pushButton->setEnabled(haveSelection);
    m_removeButton->setEnabled(haveSelection);
}

} // Git::Internal
