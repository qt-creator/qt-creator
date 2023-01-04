// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmleditorwidgets_global.h"

#include <QWidget>

#include <QUrl>

QT_BEGIN_NAMESPACE
class QLabel;
class QToolButton;
class QLineEdit;
class QComboBox;
QT_END_NAMESPACE

namespace QmlEditorWidgets {

class QMLEDITORWIDGETS_EXPORT FileWidget : public QWidget
{
    Q_OBJECT

    Q_PROPERTY(QString text READ text WRITE setText)
    Q_PROPERTY(QString fileName READ fileName WRITE setFileNameStr NOTIFY fileNameChanged)
    Q_PROPERTY(QString filter READ filter WRITE setFilter)
    Q_PROPERTY(bool showComboBox READ showComboBox WRITE setShowComboBox)
    Q_PROPERTY(QUrl path READ path WRITE setPath)

public:

    FileWidget(QWidget *parent = nullptr);
    ~FileWidget() override;

    QString fileName() const
    { return m_fileName.toString(); }

    void setText(const QString &/*text*/)
    {

    }

    void setPath(const QUrl &url) { m_path = url; setupComboBox(); }

    QUrl path() const { return m_path; }

    QString text() const
    {
        return QString();
    }

    void setFilter(const QString &filter)
    {
        m_filter = filter;
    }

    QString filter() const
    {
        return m_filter;
    }

    void setShowComboBox(bool show);

    bool showComboBox() const
    { return m_showComboBox; }

    void setFileName(const QUrl &fileName);
    void setFileNameStr(const QString &fileName);
    void onButtonReleased();
    void lineEditChanged();
    void comboBoxChanged();

signals:
    void fileNameChanged(const QUrl &fileName);
    void itemNodeChanged();

protected:

private:

    void setupComboBox();

    QToolButton *m_pushButton;
    QLineEdit *m_lineEdit;
    QComboBox *m_comboBox;
    QUrl m_fileName;
    QUrl m_path;
    QString m_filter;
    bool m_showComboBox;
    bool m_lock;

};

} //QmlEditorWidgets
