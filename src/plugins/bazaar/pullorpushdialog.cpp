// Copyright (C) 2016 Hugues Delorme
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "pullorpushdialog.h"

#include "bazaartr.h"

#include <utils/qtcassert.h>
#include <utils/layoutbuilder.h>

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QLabel>
#include <QLineEdit>
#include <QRadioButton>

namespace Bazaar::Internal {

PullOrPushDialog::PullOrPushDialog(Mode mode, QWidget *parent)
    : QDialog(parent), m_mode(mode)
{
    resize(477, 388);

    setWindowTitle(Tr::tr("Dialog"));

    m_defaultButton = new QRadioButton(Tr::tr("Default location"));
    m_defaultButton->setChecked(true);

    m_localButton = new QRadioButton(Tr::tr("Local filesystem:"));

    m_localPathChooser = new Utils::PathChooser;
    m_localPathChooser->setEnabled(false);

    auto urlButton = new QRadioButton(Tr::tr("Specify URL:"));
    urlButton->setToolTip(Tr::tr("For example: \"https://[user[:pass]@]host[:port]/[path]\"."));

    m_urlLineEdit = new QLineEdit;
    m_urlLineEdit->setEnabled(false);
    m_urlLineEdit->setToolTip(Tr::tr("For example: \"https://[user[:pass]@]host[:port]/[path]\"."));

    m_rememberCheckBox = new QCheckBox(Tr::tr("Remember specified location as default"));
    m_rememberCheckBox->setEnabled(false);

    m_overwriteCheckBox = new QCheckBox(Tr::tr("Overwrite"));
    m_overwriteCheckBox->setToolTip(Tr::tr("Ignores differences between branches and overwrites\n"
        "unconditionally."));

    m_useExistingDirCheckBox = new QCheckBox(Tr::tr("Use existing directory"));
    m_useExistingDirCheckBox->setToolTip(Tr::tr("By default, push will fail if the target directory "
        "exists, but does not already have a control directory.\n"
        "This flag will allow push to proceed."));

    m_createPrefixCheckBox = new QCheckBox(Tr::tr("Create prefix"));
    m_createPrefixCheckBox->setToolTip(Tr::tr("Creates the path leading up to the branch "
                                          "if it does not already exist."));

    m_revisionLineEdit = new QLineEdit;

    m_localCheckBox = new QCheckBox(Tr::tr("Local"));
    m_localCheckBox->setToolTip(Tr::tr("Performs a local pull in a bound branch.\n"
        "Local pulls are not applied to the master branch."));

    auto buttonBox = new QDialogButtonBox(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);

    m_localPathChooser->setExpectedKind(Utils::PathChooser::Directory);
    if (m_mode == PullMode) {
        setWindowTitle(Tr::tr("Pull Source"));
        m_useExistingDirCheckBox->setVisible(false);
        m_createPrefixCheckBox->setVisible(false);
    } else {
        setWindowTitle(Tr::tr("Push Destination"));
        m_localCheckBox->setVisible(false);
    }

    using namespace Layouting;
    Column {
        Group {
            title(Tr::tr("Branch Location")),
            Form {
                m_defaultButton, br,
                m_localButton, m_localPathChooser, br,
                urlButton,  m_urlLineEdit, br,
            }
        },
        Group {
            title(Tr::tr("Options")),
            Column {
                m_rememberCheckBox,
                m_overwriteCheckBox,
                m_localCheckBox,
                m_useExistingDirCheckBox,
                m_createPrefixCheckBox,
                Row { Tr::tr("Revision:"), m_revisionLineEdit },
            }
        },
        buttonBox,
    }.attachTo(this);

    setFixedHeight(sizeHint().height());
    setSizeGripEnabled(true);

    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(urlButton, &QRadioButton::toggled, m_urlLineEdit, &QWidget::setEnabled);
    connect(m_localButton, &QAbstractButton::toggled, m_localPathChooser, &QWidget::setEnabled);
    connect(urlButton, &QRadioButton::toggled, m_rememberCheckBox, &QWidget::setEnabled);
    connect(m_localButton, &QRadioButton::toggled, m_rememberCheckBox, &QWidget::setEnabled);
}

PullOrPushDialog::~PullOrPushDialog() = default;

QString PullOrPushDialog::branchLocation() const
{
    if (m_defaultButton->isChecked())
        return {};
    if (m_localButton->isChecked())
        return m_localPathChooser->filePath().toString();
    return m_urlLineEdit->text();
}

bool PullOrPushDialog::isRememberOptionEnabled() const
{
    if (m_defaultButton->isChecked())
        return false;
    return m_rememberCheckBox->isChecked();
}

bool PullOrPushDialog::isOverwriteOptionEnabled() const
{
    return m_overwriteCheckBox->isChecked();
}

QString PullOrPushDialog::revision() const
{
    return m_revisionLineEdit->text().simplified();
}

bool PullOrPushDialog::isLocalOptionEnabled() const
{
    QTC_ASSERT(m_mode == PullMode, return false);
    return m_localCheckBox->isChecked();
}

bool PullOrPushDialog::isUseExistingDirectoryOptionEnabled() const
{
    QTC_ASSERT(m_mode == PushMode, return false);
    return m_useExistingDirCheckBox->isChecked();
}

bool PullOrPushDialog::isCreatePrefixOptionEnabled() const
{
    QTC_ASSERT(m_mode == PushMode, return false);
    return m_createPrefixCheckBox->isChecked();
}

} // Bazaar::Internal
