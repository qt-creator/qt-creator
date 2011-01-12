/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/


#ifndef FILEWIDGET_H
#define FILEWIDGET_H

#include <QtGui/QWidget>
#include <QLabel>
#include <QToolButton>
#include <QLineEdit>
#include <QComboBox>
#include <QUrl>
#include <qmlitemnode.h>

QT_BEGIN_NAMESPACE

class FileWidget : public QWidget
{
    Q_OBJECT

    Q_PROPERTY(QString text READ text WRITE setText)
    Q_PROPERTY(QString fileName READ fileName WRITE setFileNameStr NOTIFY fileNameChanged)
    Q_PROPERTY(QString filter READ filter WRITE setFilter)
    Q_PROPERTY(bool showComboBox READ showComboBox WRITE setShowComboBox)
    Q_PROPERTY(QVariant itemNode READ itemNode WRITE setItemNode NOTIFY itemNodeChanged)
    Q_PROPERTY(QUrl path READ path WRITE setPath)

public:

    FileWidget(QWidget *parent = 0);
    ~FileWidget();

    QVariant itemNode() const { return QVariant::fromValue(m_itemNode.modelNode()); }
    void setItemNode(const QVariant &itemNode);

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
    void buttonPressed();
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
    QmlDesigner::QmlItemNode m_itemNode;
    QString m_filter;
    bool m_showComboBox;
    bool m_lock;
    QString m_currentPath;

};

QT_END_NAMESPACE

#endif

