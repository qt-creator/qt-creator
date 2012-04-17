//! [0]
#include "webpagewizard.h"

#include <utils/filewizarddialog.h>
#include <utils/qtcassert.h>

#include <QXmlStreamWriter>

#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>

namespace MyPlugin {
namespace Internal {

//! [1]
WebContentPageWizardPage::WebContentPageWizardPage(QWidget *parent) :
    QWizardPage(parent),
    m_titleLineEdit(new QLineEdit),
    m_textEdit(new QPlainTextEdit),
    m_complete(false)
{
    QGridLayout *layout = new QGridLayout(this);
    QLabel *titleLabel = new QLabel(tr("&Title"));
    layout->addWidget(titleLabel, 0, 0);
    layout->addWidget(m_titleLineEdit, 0, 1);
    QLabel *contentLabel = new QLabel(tr("&Content"));
    layout->addWidget(contentLabel, 1, 0, 1, 1, Qt::AlignTop);
    layout->addWidget(m_textEdit, 1, 1);
    titleLabel->setBuddy(m_titleLineEdit);
    contentLabel->setBuddy(m_textEdit);
    setTitle(tr("Web Page Contents"));

    connect(m_titleLineEdit, SIGNAL(textChanged(QString)), this, SLOT(slotChanged()));
    connect(m_textEdit, SIGNAL(textChanged()), this, SLOT(slotChanged()));
}

QString WebContentPageWizardPage::title() const
{
    return m_titleLineEdit->text();
}

QString WebContentPageWizardPage::contents() const
{
    return m_textEdit->toPlainText();
}

void WebContentPageWizardPage::slotChanged()
{
    const bool completeNow = !m_titleLineEdit->text().isEmpty()
            && m_textEdit->blockCount() > 0;
    if (completeNow != m_complete) {
        m_complete = completeNow;
        emit completeChanged();
    }
}

//! [1] //! [2]

WebContentWizardDialog::WebContentWizardDialog(QWidget *parent) :
    Utils::FileWizardDialog(parent), m_contentPage(new WebContentPageWizardPage)
{
    addPage(m_contentPage);
}

//! [2] //! [3]

WebPageWizard::WebPageWizard(const Core::BaseFileWizardParameters &parameters,
                             QObject *parent) :
    Core::BaseFileWizard(parameters, parent)
{
}

QWizard *WebPageWizard::createWizardDialog(QWidget *parent,
                                    const QString &defaultPath,
                                    const WizardPageList &extensionPages) const
{
    WebContentWizardDialog *dialog = new WebContentWizardDialog(parent);
    dialog->setPath(defaultPath);
    foreach (QWizardPage *p, extensionPages)
        dialog->addPage(p);
    return dialog;
}

Core::GeneratedFiles
    WebPageWizard::generateFiles(const QWizard *w, QString *) const
{
    Core::GeneratedFiles files;
    const WebContentWizardDialog *dialog = qobject_cast<const WebContentWizardDialog*>(w);
    QTC_ASSERT(dialog, return files);

    const QString fileName = Core::BaseFileWizard::buildFileName(dialog->path(), dialog->fileName(), QLatin1String("html"));

    Core::GeneratedFile file(fileName);

    QString contents;
    QXmlStreamWriter writer(&contents);
    writer.setAutoFormatting(true);
    writer.writeStartDocument();
    writer.writeStartElement(QLatin1String("html"));
    writer.writeStartElement(QLatin1String("head"));
    writer.writeTextElement(QLatin1String("title"), dialog->title());
    writer.writeEndElement(); // HEAD
    writer.writeStartElement(QLatin1String("body"));
    writer.writeTextElement(QLatin1String("h1"), dialog->title());
    writer.writeTextElement(QLatin1String("p"), dialog->contents());
    writer.writeEndElement(); // BODY
    writer.writeEndElement(); // HTML
    file.setAttributes(Core::GeneratedFile::OpenEditorAttribute);
    file.setContents(contents);
    files.push_back(file);
    return files;
}

//! [3]

} // namespace Internal
} // namespace MyPlugin

//! [0]
