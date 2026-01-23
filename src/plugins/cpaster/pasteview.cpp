// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "pasteview.h"

#include "columnindicatortextedit.h"
#include "cpastertr.h"
#include "protocol.h"
#include "settings.h"
#include "splitter.h"

#include <coreplugin/icore.h>

#include <utils/layoutbuilder.h>
#include <utils/mimeconstants.h>
#include <utils/qtcassert.h>
#include <utils/qtcsettings.h>

#include <QApplication>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSettings>
#include <QSpinBox>
#include <QStackedWidget>
#include <QTextEdit>

using namespace Utils;

namespace CodePaster {

const char groupC[] = "CPaster";
const char heightKeyC[] = "PasteViewHeight";
const char widthKeyC[] = "PasteViewWidth";

static ContentType contentType(const QString &mt)
{
    using namespace Utils::Constants;
    if (mt == QLatin1StringView(C_SOURCE_MIMETYPE)
        || mt == QLatin1StringView(C_HEADER_MIMETYPE)
        || mt == QLatin1StringView(GLSL_MIMETYPE)
        || mt == QLatin1StringView(GLSL_VERT_MIMETYPE)
        || mt == QLatin1StringView(GLSL_FRAG_MIMETYPE)
        || mt == QLatin1StringView(GLSL_ES_VERT_MIMETYPE)
        || mt == QLatin1StringView(GLSL_ES_FRAG_MIMETYPE))
        return C;
    if (mt == QLatin1StringView(CPP_SOURCE_MIMETYPE)
        || mt == QLatin1StringView(CPP_HEADER_MIMETYPE)
        || mt == QLatin1StringView(OBJECTIVE_C_SOURCE_MIMETYPE)
        || mt == QLatin1StringView(OBJECTIVE_CPP_SOURCE_MIMETYPE))
        return Cpp;
    if (mt == QLatin1StringView(QML_MIMETYPE)
        || mt == QLatin1StringView(QMLUI_MIMETYPE)
        || mt == QLatin1StringView(QMLPROJECT_MIMETYPE)
        || mt == QLatin1StringView(QBS_MIMETYPE)
        || mt == QLatin1StringView(JS_MIMETYPE)
        || mt == QLatin1StringView(JSON_MIMETYPE))
        return JavaScript;
    if (mt == QLatin1StringView("text/x-patch"))
        return Diff;
    if (mt == QLatin1StringView("text/xml")
        || mt == QLatin1StringView("application/xml")
        || mt == QLatin1StringView(RESOURCE_MIMETYPE)
        || mt == QLatin1StringView(FORM_MIMETYPE))
        return Xml;
    return Text;
}

class PasteView : public QDialog
{
public:
    enum Mode
    {
        // Present a list of read-only diff chunks which the user can check for inclusion
        DiffChunkMode,
        // Present plain, editable text.
        PlainTextMode
    };

    explicit PasteView(const QList<Protocol *> &protocols);
    ~PasteView() override;

    // Show up with checkable list of diff chunks.
    int show(const QString &user, int expiryDays, const FileDataList &parts);
    // Show up with editable plain text.
    int show(const QString &user, int expiryDays, const QString &content);

    void setProtocol(const QString &protocol);

    QString user() const;
    QString description() const;
    QString content() const;
    int protocol() const;
    void setExpiryDays(int d);
    int expiryDays() const;

    void accept() override;

private:
    void contentChanged();
    void protocolChanged(int);

    int showDialog();

    const QList<Protocol *> m_protocols;

    QComboBox *m_protocolBox;
    QSpinBox *m_expirySpinBox;
    QLineEdit *m_uiUsername;
    QLineEdit *m_uiDescription;
    QStackedWidget *m_stackedWidget;
    QListWidget *m_uiPatchList;
    ColumnIndicatorTextEdit *m_uiPatchView;
    QPlainTextEdit *m_plainTextEdit;
    FileDataList m_parts;
    Mode m_mode = DiffChunkMode;
};

PasteView::PasteView(const QList<Protocol *> &protocols)
    : QDialog(Core::ICore::dialogParent())
    , m_protocols(protocols)
{
    setObjectName("CodePaster.ViewDialog");
    resize(670, 678);
    setWindowTitle(Tr::tr("Send to Codepaster"));

    m_protocolBox = new QComboBox;
    m_protocolBox->setObjectName("protocolBox");
    for (const Protocol *p : protocols)
        m_protocolBox->addItem(p->name());

    m_expirySpinBox = new QSpinBox;
    m_expirySpinBox->setSuffix(Tr::tr(" Days"));
    m_expirySpinBox->setRange(1, 365);

    m_uiUsername = new QLineEdit(this);
    m_uiUsername->setPlaceholderText(Tr::tr("<Username>"));

    m_uiDescription = new QLineEdit(this);
    m_uiDescription->setObjectName("uiDescription");
    m_uiDescription->setPlaceholderText(Tr::tr("<Description>"));

    QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    m_uiPatchList = new QListWidget;
    sizePolicy.setVerticalStretch(1);
    m_uiPatchList->setSizePolicy(sizePolicy);
    m_uiPatchList->setUniformItemSizes(true);

    m_uiPatchView = new CodePaster::ColumnIndicatorTextEdit;
    sizePolicy.setVerticalStretch(3);
    m_uiPatchView->setSizePolicy(sizePolicy);

    QFont font;
    font.setFamilies({"Courier New"});
    m_uiPatchView->setFont(font);
    m_uiPatchView->setReadOnly(true);

    auto groupBox = new QGroupBox(Tr::tr("Parts to Send to Server"));
    groupBox->setFlat(true);

    m_plainTextEdit = new QPlainTextEdit;
    m_plainTextEdit->setObjectName("plainTextEdit");

    m_stackedWidget = new QStackedWidget(this);
    m_stackedWidget->addWidget(groupBox);
    m_stackedWidget->addWidget(m_plainTextEdit);
    m_stackedWidget->setCurrentIndex(0);

    auto buttonBox = new QDialogButtonBox(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);
    buttonBox->button(QDialogButtonBox::Ok)->setText(Tr::tr("Paste"));

    const bool __sortingEnabled = m_uiPatchList->isSortingEnabled();
    m_uiPatchList->setSortingEnabled(false);
    m_uiPatchList->setSortingEnabled(__sortingEnabled);

    using namespace Layouting;

    Column {
        m_uiPatchList,
        m_uiPatchView
    }.attachTo(groupBox);

    Column {
        spacing(2),
        Form {
            Tr::tr("Protocol:"), m_protocolBox, br,
            Tr::tr("&Expires after:"), m_expirySpinBox, br,
            Tr::tr("&Username:"), m_uiUsername, br,
            Tr::tr("&Description:"), m_uiDescription,
        },
        m_stackedWidget,
        buttonBox
    }.attachTo(this);

    connect(m_uiPatchList, &QListWidget::itemChanged, this, &PasteView::contentChanged);

    connect(m_protocolBox, &QComboBox::currentIndexChanged, this, &PasteView::protocolChanged);

    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

PasteView::~PasteView() = default;

QString PasteView::user() const
{
    const QString username = m_uiUsername->text();
    if (username.isEmpty())
        return QLatin1String("Anonymous");
    return username;
}

QString PasteView::description() const
{
    return m_uiDescription->text();
}

QString PasteView::content() const
{
    if (m_mode == PlainTextMode)
        return m_plainTextEdit->toPlainText();

    QString newContent;
    for (int i = 0; i < m_uiPatchList->count(); ++i) {
        QListWidgetItem *item = m_uiPatchList->item(i);
        if (item->checkState() != Qt::Unchecked)
            newContent += m_parts.at(i).content;
    }
    return newContent;
}

int PasteView::protocol() const
{
    return m_protocolBox->currentIndex();
}

void PasteView::contentChanged()
{
    m_uiPatchView->setPlainText(content());
}

void PasteView::protocolChanged(int p)
{
    QTC_ASSERT(p >= 0 && p < m_protocols.size(), return);
    const Capabilities caps = m_protocols.at(p)->capabilities();
    m_uiDescription->setEnabled(caps & Capability::PostDescription);
    m_uiUsername->setEnabled(caps & Capability::PostUserName);
}

int PasteView::showDialog()
{
    m_uiDescription->setFocus();
    m_uiDescription->selectAll();

    // (Re)store dialog size
    const QtcSettings *settings = Core::ICore::settings();
    const Key rootKey = Key(groupC) + '/';
    const int h = settings->value(rootKey + heightKeyC, height()).toInt();
    const int defaultWidth = m_uiPatchView->columnIndicator() + 50;
    const int w = settings->value(rootKey + widthKeyC, defaultWidth).toInt();

    resize(w, h);

    return QDialog::exec();
}

// Show up with checkable list of diff chunks.
int PasteView::show(const QString &user, int expiryDays, const FileDataList &parts)
{
    m_uiUsername->setText(user);
    m_uiPatchList->clear();
    m_parts = parts;
    m_mode = DiffChunkMode;
    QString content;
    for (const FileData &part : parts) {
        auto itm = new QListWidgetItem(part.filename, m_uiPatchList);
        itm->setCheckState(Qt::Checked);
        itm->setFlags(Qt::ItemIsSelectable | Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
        content += part.content;
    }
    m_stackedWidget->setCurrentIndex(0);
    m_uiPatchView->setPlainText(content);
    setExpiryDays(expiryDays);
    return showDialog();
}

// Show up with editable plain text.
int PasteView::show(const QString &user, int expiryDays, const QString &content)
{
    m_uiUsername->setText(user);
    m_mode = PlainTextMode;
    m_stackedWidget->setCurrentIndex(1);
    m_plainTextEdit->setPlainText(content);
    setExpiryDays(expiryDays);
    return showDialog();
}

void PasteView::setExpiryDays(int d)
{
    m_expirySpinBox->setValue(d);
}

int PasteView::expiryDays() const
{
    return m_expirySpinBox->value();
}

void PasteView::accept()
{
    const int index = m_protocolBox->currentIndex();
    if (index == -1)
        return;

    Protocol *protocol = m_protocols.at(index);

    if (!Protocol::ensureConfiguration(protocol))
        return;

    const QString data = content();
    if (data.isEmpty())
        return;

    // Store settings and close
    QtcSettings *settings = Core::ICore::settings();
    settings->beginGroup(groupC);
    settings->setValue(heightKeyC, height());
    settings->setValue(widthKeyC, width());
    settings->endGroup();
    QDialog::accept();
}

void PasteView::setProtocol(const QString &protocol)
{
     const int index = m_protocolBox->findText(protocol);
     if (index < 0)
         return;
     m_protocolBox->setCurrentIndex(index);
     if (index == m_protocolBox->currentIndex())
         protocolChanged(index); // Force enabling
     else
         m_protocolBox->setCurrentIndex(index);
}

static inline void fixSpecialCharacters(QString &data)
{
    QChar *uc = data.data();
    QChar *e = uc + data.size();

    for (; uc != e; ++uc) {
        switch (uc->unicode()) {
        case 0xfdd0: // QTextBeginningOfFrame
        case 0xfdd1: // QTextEndOfFrame
        case QChar::ParagraphSeparator:
        case QChar::LineSeparator:
            *uc = QLatin1Char('\n');
            break;
        case QChar::Nbsp:
            *uc = QLatin1Char(' ');
            break;
        default:
            break;
        }
    }
}

std::optional<PasteInputData> executePasteDialog(const QList<Protocol *> &protocols,
                                                 const QString &data, const QString &mimeType)
{
    QString copiedData = data;
    fixSpecialCharacters(copiedData);

    const QString username = settings().username();

    PasteView view(protocols);
    view.setProtocol(settings().protocols.stringValue());

    const FileDataList diffChunks = splitDiffToFiles(copiedData);
    const int dialogResult = diffChunks.isEmpty()
                                 ? view.show(username, settings().expiryDays(), copiedData)
                                 : view.show(username, settings().expiryDays(), diffChunks);
    if (dialogResult != QDialog::Accepted)
        return {};

    // Save new protocol in case user changed it.
    if (settings().protocols() != view.protocol()) {
        settings().protocols.setValue(view.protocol());
        settings().writeSettings();
    }

    return PasteInputData{view.content(), contentType(mimeType), view.expiryDays(),
                          view.user(), view.description()};
}

} // CodePaster
