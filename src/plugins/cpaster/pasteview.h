// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <splitter.h>

#include <QDialog>

QT_BEGIN_NAMESPACE
class QComboBox;
class QLineEdit;
class QListWidget;
class QPlainTextEdit;
class QSpinBox;
class QStackedWidget;
class QTextEdit;
QT_END_NAMESPACE

namespace CodePaster {

class ColumnIndicatorTextEdit;
class Protocol;

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

    explicit PasteView(const QList<Protocol *> &protocols,
                       const QString &mimeType,
                       QWidget *parent);
    ~PasteView() override;

    // Show up with checkable list of diff chunks.
    int show(const QString &user, const QString &description, const QString &comment,
             int expiryDays, const FileDataList &parts);
    // Show up with editable plain text.
    int show(const QString &user, const QString &description, const QString &comment,
             int expiryDays, const QString &content);

    void setProtocol(const QString &protocol);

    QString user() const;
    QString description() const;
    QString comment() const;
    QString content() const;
    int protocol() const;
    void setExpiryDays(int d);
    int expiryDays() const;

    void accept() override;

private:
    void contentChanged();
    void protocolChanged(int);

    void setupDialog(const QString &user, const QString &description, const QString &comment);
    int showDialog();

    const QList<Protocol *> m_protocols;
    const QString m_commentPlaceHolder;
    const QString m_mimeType;

    QComboBox *m_protocolBox;
    QSpinBox *m_expirySpinBox;
    QLineEdit *m_uiUsername;
    QLineEdit *m_uiDescription;
    QTextEdit *m_uiComment;
    QStackedWidget *m_stackedWidget;
    QListWidget *m_uiPatchList;
    ColumnIndicatorTextEdit *m_uiPatchView;
    QPlainTextEdit *m_plainTextEdit;
    FileDataList m_parts;
    Mode m_mode = DiffChunkMode;
};

} // CodePaster
