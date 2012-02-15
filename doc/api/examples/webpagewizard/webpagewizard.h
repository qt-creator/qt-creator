//! [0]

#ifndef WEBPAGEFILEWIZARD_H
#define WEBPAGEFILEWIZARD_H

#include "basefilewizard.h"
#include "utils/filewizarddialog.h"

#include <QWizardPage>

QT_BEGIN_NAMESPACE
class QLineEdit;
class QPlainTextEdit;
QT_END_NAMESPACE

namespace MyPlugin {
namespace Internal {

//! [1]
class WebContentPageWizardPage : public QWizardPage
{
    Q_OBJECT
public:
    explicit WebContentPageWizardPage(QWidget *parent = 0);

    QString title() const;
    QString contents() const;

    virtual bool isComplete() const { return m_complete; }

private slots:
    void slotChanged();

private:
    QLineEdit *m_titleLineEdit;
    QPlainTextEdit *m_textEdit;
    bool m_complete;
};

//! [1] //! [2]

class WebContentWizardDialog : public Utils::FileWizardDialog
{
    Q_OBJECT
public:
    explicit WebContentWizardDialog(QWidget *parent = 0);

    QString title() const    { return m_contentPage->title(); }
    QString contents() const { return m_contentPage->contents(); }

private:
    WebContentPageWizardPage *m_contentPage;
};

//! [2] //! [3]

class WebPageWizard : public Core::BaseFileWizard
{
public:
    explicit WebPageWizard(const Core::BaseFileWizardParameters &parameters,
                           QObject *parent = 0);
protected:
    virtual QWizard *createWizardDialog(QWidget *parent,
                                        const QString &defaultPath,
                                        const WizardPageList &extensionPages) const;
    virtual Core::GeneratedFiles generateFiles(const QWizard *w,
                                         QString *errorMessage) const;
};

//! [3]
} // namespace Internal
} // namespace MyPlugin


#endif // WEBPAGEFILEWIZARD_H
//! [0]
