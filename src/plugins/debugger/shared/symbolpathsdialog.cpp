// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "symbolpathsdialog.h"

#include "../debuggertr.h"

#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QVBoxLayout>

using namespace Utils;

namespace Debugger::Internal {

SymbolPathsDialog::SymbolPathsDialog(QWidget *parent) :
    QDialog(parent)
{
    setWindowTitle(Tr::tr("Set up Symbol Paths", nullptr));

    m_pixmapLabel = new QLabel(this);
    m_pixmapLabel->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    m_pixmapLabel->setAlignment(Qt::AlignHCenter|Qt::AlignTop);
    m_pixmapLabel->setMargin(5);
    m_pixmapLabel->setPixmap(QMessageBox::standardIcon(QMessageBox::Question));

    m_msgLabel = new QLabel(Tr::tr("<html><head/><body><p>The debugger is not configured to use the "
        "public Microsoft Symbol Server.<br/>This is recommended for retrieval of the symbols "
        "of the operating system libraries.</p>"
        "<p><span style=\" font-style:italic;\">Note:</span> It is recommended, that if you use "
        "the Microsoft Symbol Server, to also use a local symbol cache.<br/>"
        "A fast internet connection is required for this to work smoothly,<br/>"
        "and a delay might occur when connecting for the first time and caching the symbols.</p>"
        "<p>What would you like to set up?</p></body></html>"));
    m_msgLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_msgLabel->setTextFormat(Qt::RichText);
    m_msgLabel->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignTop);

    m_useLocalSymbolCache = new QCheckBox(Tr::tr("Use Local Symbol Cache"));

    m_useSymbolServer = new QCheckBox(Tr::tr("Use Microsoft Symbol Server"));

    m_pathChooser = new PathChooser;

    auto buttonBox = new QDialogButtonBox;
    buttonBox->setOrientation(Qt::Horizontal);
    buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);

    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto horizontalLayout = new QHBoxLayout();
    horizontalLayout->addWidget(m_pixmapLabel);
    horizontalLayout->addWidget(m_msgLabel);

    auto verticalLayout = new QVBoxLayout(this);
    verticalLayout->addLayout(horizontalLayout);
    verticalLayout->addWidget(m_useLocalSymbolCache);
    verticalLayout->addWidget(m_useSymbolServer);
    verticalLayout->addWidget(m_pathChooser);
    verticalLayout->addWidget(buttonBox);
}

SymbolPathsDialog::~SymbolPathsDialog() = default;

bool SymbolPathsDialog::useSymbolCache() const
{
    return m_useLocalSymbolCache->isChecked();
}

bool SymbolPathsDialog::useSymbolServer() const
{
    return m_useSymbolServer->isChecked();
}

FilePath SymbolPathsDialog::path() const
{
    return m_pathChooser->filePath();
}

void SymbolPathsDialog::setUseSymbolCache(bool useSymbolCache)
{
    m_useLocalSymbolCache->setChecked(useSymbolCache);
}

void SymbolPathsDialog::setUseSymbolServer(bool useSymbolServer)
{
    m_useSymbolServer->setChecked(useSymbolServer);
}

void SymbolPathsDialog::setPath(const FilePath &path)
{
    m_pathChooser->setFilePath(path);
}

bool SymbolPathsDialog::useCommonSymbolPaths(bool &useSymbolCache,
                                             bool &useSymbolServer,
                                             FilePath &path)
{
    SymbolPathsDialog dialog;
    dialog.setUseSymbolCache(useSymbolCache);
    dialog.setUseSymbolServer(useSymbolServer);
    dialog.setPath(path);
    int ret = dialog.exec();
    useSymbolCache = dialog.useSymbolCache();
    useSymbolServer = dialog.useSymbolServer();
    path = dialog.path();
    return ret == QDialog::Accepted;
}

} // Debugger::Internal
