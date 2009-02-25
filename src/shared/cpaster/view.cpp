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

#include "view.h"

#include <QFontMetrics>
#include <QPainter>
#include <QScrollBar>
#include <QPushButton>
#include <QSettings>

class ColumnIndicatorTextEdit : public QTextEdit
{
public:
    ColumnIndicatorTextEdit(QWidget *parent) : QTextEdit(parent), m_columnIndicator(0)
    {
        QFont font;
        font.setFamily(QString::fromUtf8("Courier New"));
        //font.setPointSizeF(8.0);
        setFont(font);
        setReadOnly(true);
        QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        sizePolicy.setVerticalStretch(3);
        setSizePolicy(sizePolicy);
        int cmx = 0, cmy = 0, cmw = 0, cmh = 0;
        getContentsMargins(&cmx, &cmy, &cmw, &cmh);
        m_columnIndicator = QFontMetrics(font).width('W') * 100 + cmx + 1;
        m_columnIndicatorFont.setFamily(QString::fromUtf8("Times"));
        m_columnIndicatorFont.setPointSizeF(7.0);
    }

    int m_columnIndicator;
    QFont m_columnIndicatorFont;

protected:
    virtual void paintEvent(QPaintEvent *event);
};

void ColumnIndicatorTextEdit::paintEvent(QPaintEvent *event)
{
    QTextEdit::paintEvent(event);

    QPainter p(viewport());
    p.setFont(m_columnIndicatorFont);
    p.setPen(QPen(QColor(0xa0, 0xa0, 0xa0, 0xa0)));
    p.drawLine(m_columnIndicator, 0, m_columnIndicator, viewport()->height());
    int yOffset = verticalScrollBar()->value();
    p.drawText(m_columnIndicator + 1, m_columnIndicatorFont.pointSize() - yOffset, "100");
}

// -------------------------------------------------------------------------------------------------


View::View(QWidget *parent)
    : QDialog(parent)
{
    m_ui.setupUi(this);

    // Swap out the Patch View widget with a ColumnIndicatorTextEdit, which will indicate column 100
    delete m_ui.uiPatchView;
    m_ui.uiPatchView = new ColumnIndicatorTextEdit(m_ui.groupBox);
    m_ui.vboxLayout1->addWidget(m_ui.uiPatchView);
    m_ui.buttonBox->button(QDialogButtonBox::Ok)->setText("Paste");
    connect(m_ui.uiPatchList, SIGNAL(itemChanged(QListWidgetItem*)), this, SLOT(contentChanged()));
}

View::~View()
{
}

QString View::getUser()
{
    const QString username = m_ui.uiUsername->text();
    if (username.isEmpty() || username == "<Username>")
        return "Anonymous";
    return username;
}

QString View::getDescription()
{
    const QString description = m_ui.uiDescription->text();
    if (description == "<Description>")
        return QString();
    return description;
}

QString View::getComment()
{
    const QString comment = m_ui.uiComment->toPlainText();
    if (comment == "<Comment>")
        return QString();
    return comment;
}

QByteArray View::getContent()
{
    QByteArray newContent;
    for (int i = 0; i < m_ui.uiPatchList->count(); ++i) {
        QListWidgetItem *item = m_ui.uiPatchList->item(i);
        if (item->checkState() != Qt::Unchecked)
            newContent += m_parts.at(i).content;
    }
    return newContent;
}

void View::contentChanged()
{
    m_ui.uiPatchView->setPlainText(getContent());
}

int View::show(const QString &user, const QString &description, const QString &comment,
               const FileDataList &parts)
{
    if (user.isEmpty())
        m_ui.uiUsername->setText("<Username>");
    else
        m_ui.uiUsername->setText(user);

    if (description.isEmpty())
        m_ui.uiDescription->setText("<Description>");
    else
        m_ui.uiDescription->setText(description);

    if (comment.isEmpty())
        m_ui.uiComment->setPlainText("<Comment>");
    else
        m_ui.uiComment->setPlainText(comment);

    QByteArray content;
    m_parts = parts;
    m_ui.uiPatchList->clear();
    foreach (const FileData part, parts) {
        QListWidgetItem *itm = new QListWidgetItem(part.filename, m_ui.uiPatchList);
        itm->setCheckState(Qt::Checked);
        itm->setFlags(Qt::ItemIsSelectable | Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
        content += part.content;
    }
    m_ui.uiPatchView->setPlainText(content);

    m_ui.uiDescription->setFocus();
    m_ui.uiDescription->selectAll();

    // (Re)store dialog size
    QSettings settings("Trolltech", "cpaster");
    int h = settings.value("/gui/height", height()).toInt();
    int w = settings.value("/gui/width",
                           ((ColumnIndicatorTextEdit*)m_ui.uiPatchView)->m_columnIndicator + 50)
                                .toInt();
    resize(w, h);
    int ret = QDialog::exec();
    settings.setValue("/gui/height", height());
    settings.setValue("/gui/width", width());

    return ret;
}
