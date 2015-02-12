/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/


#ifndef FILEWIDGET_H
#define FILEWIDGET_H

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

    FileWidget(QWidget *parent = 0);
    ~FileWidget();

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

public slots:
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

#endif

