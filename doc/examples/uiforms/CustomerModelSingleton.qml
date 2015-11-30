/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

pragma Singleton

import QtQuick 2.0

ListModel {
    property QtObject selection
    ListElement {
        customerId: "15881123"
        firstName: "Julia"
        title: "Ms."
        lastName: "Jefferson"
        email: "Julia@example.com"
        address: "Spandia Avenue, Suite 610"
        city: "Toronto"
        zipCode: "92334"
        phoneNumber: "0803-033330"
        notes: "Very demanding customer."
        history: "21.4.2014|Order|coffee~23.4.2014|Order|poster~29.4.2014|Invoice|poster 40$~05.5.2014|Overdue Notice|poster 40$"
    }

    ListElement {
        customerId: "29993496"
        firstName: "Tim"
        lastName: "Northington"
        title: "Mr."
        email: "Northington@example.com"
        address: "North Fifth Street 55"
        city: "San Jose"
        zipCode: "95112"
        phoneNumber: "09000-3330"
        notes: "Very good customer."
        history: "18.4.2014|Order|orange juice~23.4.2014|Order|chair~24.4.2014|Complaint|Chair is broken."
    }

    ListElement {
        customerId: "37713567"
        firstName: "Daniel"
        lastName: "Krumm"
        title: "Mr."
        email: "Krumm@example.com"
        address: "Waterfront 14"
        city: "Berlin"
        zipCode: "12334"
        phoneNumber: "0708093330"
        notes: "This customer has a lot of Complaints."
        history: "15.4.2014|Order|table~25.4.2014|Return|table~28.4.2014|Complaint|Table had wrong color."
    }

    ListElement {
        customerId: "45817387"
        firstName: "Sandra"
        lastName: "Booth"
        title: "Ms."
        email: "Sandrab@example.com"
        address: "Folsom Street 23"
        city: "San Francisco"
        zipCode: "94103"
        phoneNumber: "0103436370"
        notes: "This customer is not paying."
        history: "22.4.2014|Order|coffee~23.4.2014|Order|smartphone~29.4.2014|Invoice|smartphone 200$~05.5.2014|Overdue Notice|smartphone 200$"
    }

    ListElement {
        customerId: "588902396"
        firstName: "Lora"
        lastName: "Beckner"
        title: "Ms."
        email: "LoraB@example.com"
        address: " W Wilson Apt 3"
        city: "Los Angeles"
        zipCode: "90086"
        phoneNumber: "0903436360"
        notes: "This customer usually pays late."
        history: "17.4.2014|Order|soft drink~23.4.2014|Order|computer~29.4.2014|Invoice|computer 1200$~07.5.2014|Overdue Notice|computer 1200$"
    }

    ListElement {
        customerId: "78885693"
        firstName: "Vanessa"
        lastName: "Newbury"
        title: "Ms."
        email: "VanessaN@example.com"
        address: "Madison Ave. 277"
        city: "New York"
        zipCode: "10016"
        phoneNumber: "0503053530"
        notes: "Deliveries sometime do not arrive on time."
        history: "19.4.2014|Order|coffee~23.4.2014|Order|bicycle~29.4.2014|Invoice|bicycle 500$~06.5.2014|Overdue Notice|bicycle 500$"
    }
}
