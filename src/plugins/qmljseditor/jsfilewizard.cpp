/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "jsfilewizard.h"

#include <qmljstools/qmljstoolsconstants.h>

#include <utils/filewizarddialog.h>
#include <utils/qtcassert.h>
#include <utils/filewizarddialog.h>

#include <QFileInfo>
#include <QTextStream>
#include <QWizard>
#include <QPushButton>
#include <QBoxLayout>
#include <QCheckBox>

namespace {
class JsFileOptionsPage : public QWizardPage
{
    Q_OBJECT
public:
    JsFileOptionsPage()
    {
        setTitle(tr("Options"));

        QVBoxLayout *layout = new QVBoxLayout;
        m_statelessLibrary = new QCheckBox(tr("Stateless library"));
        m_statelessLibrary->setToolTip(
                    tr("Usually each QML component instance has a unique copy of\n"
                       "imported JavaScript libraries. Indicating that a library is\n"
                       "stateless means that a single instance will be shared among\n"
                       "all components. Stateless libraries will not be able to access\n"
                       "QML component instance objects and properties directly."));
        layout->addWidget(m_statelessLibrary);
        setLayout(layout);
    }

    bool statelessLibrary() const
    {
        return m_statelessLibrary->isChecked();
    }

private:
    QCheckBox *m_statelessLibrary;
};

class JsFileWizardDialog : public Utils::FileWizardDialog
{
    Q_OBJECT
public:
    JsFileWizardDialog(QWidget *parent = 0)
        : Utils::FileWizardDialog(parent)
        , m_optionsPage(new JsFileOptionsPage)
    {
        addPage(m_optionsPage);
    }

    JsFileOptionsPage *m_optionsPage;
};
} // anonymous namespace

using namespace QmlJSEditor;

JsFileWizard::JsFileWizard(const BaseFileWizardParameters &parameters,
                           QObject *parent):
    Core::BaseFileWizard(parameters, parent)
{
}

Core::FeatureSet JsFileWizard::requiredFeatures() const
{
    return Core::FeatureSet();
}

Core::GeneratedFiles JsFileWizard::generateFiles(const QWizard *w,
                                                 QString * /*errorMessage*/) const
{
    const JsFileWizardDialog *wizardDialog = qobject_cast<const JsFileWizardDialog *>(w);
    const QString path = wizardDialog->path();
    const QString name = wizardDialog->fileName();

    const QString mimeType = QLatin1String(QmlJSTools::Constants::JS_MIMETYPE);
    const QString fileName = Core::BaseFileWizard::buildFileName(path, name, preferredSuffix(mimeType));

    Core::GeneratedFile file(fileName);
    file.setContents(fileContents(fileName, wizardDialog->m_optionsPage->statelessLibrary()));
    file.setAttributes(Core::GeneratedFile::OpenEditorAttribute);
    return Core::GeneratedFiles() << file;
}

QString JsFileWizard::fileContents(const QString &, bool statelessLibrary) const
{
    QString contents;
    QTextStream str(&contents);

    if (statelessLibrary)
        str << QLatin1String(".pragma library\n\n");
    str << QLatin1String("function func() {\n")
        << QLatin1String("\n")
        << QLatin1String("}\n");

    return contents;
}

QWizard *JsFileWizard::createWizardDialog(QWidget *parent,
                                          const Core::WizardDialogParameters &wizardDialogParameters) const
{
    JsFileWizardDialog *wizardDialog = new JsFileWizardDialog(parent);
    wizardDialog->setWindowTitle(tr("New %1").arg(displayName()));
    setupWizard(wizardDialog);
    wizardDialog->setPath(wizardDialogParameters.defaultPath());
    foreach (QWizardPage *p, wizardDialogParameters.extensionPages())
        BaseFileWizard::applyExtensionPageShortTitle(wizardDialog, wizardDialog->addPage(p));
    return wizardDialog;
}

#include "jsfilewizard.moc"
