/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <json.h>

#include <QFileInfo>
#include <QTest>
#include <QCryptographicHash>

#include <limits>

#define INVALID_UNICODE "\xCE\xBA\xE1"
#define UNICODE_NON_CHARACTER "\xEF\xBF\xBF"
#define UNICODE_DJE "\320\202" // Character from the Serbian Cyrillic alphabet

using namespace Json;

Q_DECLARE_METATYPE(Json::JsonArray)
Q_DECLARE_METATYPE(Json::JsonObject)

bool contains(const JsonObject::Keys &keys, const std::string &key)
{
    return std::find(keys.begin(), keys.end(), key) != keys.end();
}

class tst_Json: public QObject
{
    Q_OBJECT

public:
    tst_Json(QObject *parent = 0);

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    void testValueSimple();
    void testNumbers();
    void testNumbers_2();
    void testNumbers_3();

    void testObjectSimple();
    void testObjectSmallKeys();
    void testArraySimple();
    void testValueObject();
    void testValueArray();
    void testObjectNested();
    void testArrayNested();
    void testArrayNestedEmpty();
    void testObjectNestedEmpty();

    void testValueRef();
    void testObjectIteration();
    void testArrayIteration();

    void testObjectFind();

    void testDocument();

    void nullValues();
    void nullArrays();
    void nullObject();
    void constNullObject();

    void keySorting();

    void undefinedValues();

    void toJson();
    void toJsonSillyNumericValues();
    void toJsonLargeNumericValues();
    void fromJson();
    void fromJsonErrors();
    void fromBinary();
    void toAndFromBinary_data();
    void toAndFromBinary();
    void parseNumbers();
    void parseStrings();
    void parseDuplicateKeys();
    void testParser();

    void compactArray();
    void compactObject();

    void validation();

    void assignToDocument();

    void testDuplicateKeys();
    void testCompaction();
    void testCompactionError();

    void parseUnicodeEscapes();

    void assignObjects();
    void assignArrays();

    void testTrailingComma();
    void testDetachBug();
    void testJsonValueRefDefault();

    void valueEquals();
    void objectEquals_data();
    void objectEquals();
    void arrayEquals_data();
    void arrayEquals();

    void bom();
    void nesting();

    void longStrings();

    void arrayInitializerList();
    void objectInitializerList();

    void unicodeKeys();
    void garbageAtEnd();

    void removeNonLatinKey();
private:
    QString testDataDir;
};

tst_Json::tst_Json(QObject *parent) : QObject(parent)
{
}

void tst_Json::initTestCase()
{
    testDataDir = QFileInfo(QFINDTESTDATA("test.json")).absolutePath();
    if (testDataDir.isEmpty())
        testDataDir = QCoreApplication::applicationDirPath();
}

void tst_Json::cleanupTestCase()
{
}

void tst_Json::init()
{
}

void tst_Json::cleanup()
{
}

void tst_Json::testValueSimple()
{
    JsonObject object;
    object.insert("number", 999.);
    JsonArray array;
    for (int i = 0; i < 10; ++i)
        array.append((double)i);

    JsonValue value(true);
    QCOMPARE(value.type(), JsonValue::Bool);
    QCOMPARE(value.toDouble(), 0.);
    QCOMPARE(value.toString(), std::string());
    QCOMPARE(value.toBool(), true);
    QCOMPARE(value.toObject(), JsonObject());
    QCOMPARE(value.toArray(), JsonArray());
    QCOMPARE(value.toDouble(99.), 99.);
    QCOMPARE(value.toString("test"), std::string("test"));
    QCOMPARE(value.toObject(object), object);
    QCOMPARE(value.toArray(array), array);

    value = 999.;
    QCOMPARE(value.type(), JsonValue::Double);
    QCOMPARE(value.toDouble(), 999.);
    QCOMPARE(value.toString(), std::string());
    QCOMPARE(value.toBool(), false);
    QCOMPARE(value.toBool(true), true);
    QCOMPARE(value.toObject(), JsonObject());
    QCOMPARE(value.toArray(), JsonArray());

    value = "test";
    QCOMPARE(value.toDouble(), 0.);
    QCOMPARE(value.toString(), std::string("test"));
    QCOMPARE(value.toBool(), false);
    QCOMPARE(value.toObject(), JsonObject());
    QCOMPARE(value.toArray(), JsonArray());
}

void tst_Json::testNumbers()
{
    {
        int numbers[] = {
            0,
            -1,
            1,
            (1<<26),
            (1<<27),
            (1<<28),
            -(1<<26),
            -(1<<27),
            -(1<<28),
            (1<<26) - 1,
            (1<<27) - 1,
            (1<<28) - 1,
            -((1<<26) - 1),
            -((1<<27) - 1),
            -((1<<28) - 1)
        };
        int n = sizeof(numbers)/sizeof(int);

        JsonArray array;
        for (int i = 0; i < n; ++i)
            array.append((double)numbers[i]);

        std::string serialized = JsonDocument(array).toJson();
        JsonDocument json = JsonDocument::fromJson(serialized);
        JsonArray array2 = json.array();

        QCOMPARE(array.size(), array2.size());
        for (int i = 0; i < array.size(); ++i) {
            QCOMPARE(array.at(i).type(), JsonValue::Double);
            QCOMPARE(array.at(i).toDouble(), (double)numbers[i]);
            QCOMPARE(array2.at(i).type(), JsonValue::Double);
            QCOMPARE(array2.at(i).toDouble(), (double)numbers[i]);
        }
    }

    {
        int64_t numbers[] = {
            0,
            -1,
            1,
            (1ll<<54),
            (1ll<<55),
            (1ll<<56),
            -(1ll<<54),
            -(1ll<<55),
            -(1ll<<56),
            (1ll<<54) - 1,
            (1ll<<55) - 1,
            (1ll<<56) - 1,
            -((1ll<<54) - 1),
            -((1ll<<55) - 1),
            -((1ll<<56) - 1)
        };
        int n = sizeof(numbers)/sizeof(int64_t);

        JsonArray array;
        for (int i = 0; i < n; ++i)
            array.append((double)numbers[i]);

        std::string serialized = JsonDocument(array).toJson();
        JsonDocument json = JsonDocument::fromJson(serialized);
        JsonArray array2 = json.array();

        QCOMPARE(array.size(), array2.size());
        for (int i = 0; i < array.size(); ++i) {
            QCOMPARE(array.at(i).type(), JsonValue::Double);
            QCOMPARE(array.at(i).toDouble(), (double)numbers[i]);
            QCOMPARE(array2.at(i).type(), JsonValue::Double);
            QCOMPARE(array2.at(i).toDouble(), (double)numbers[i]);
        }
    }

    {
        double numbers[] = {
            0,
            -1,
            1,
            double(1ll<<54),
            double(1ll<<55),
            double(1ll<<56),
            double(-(1ll<<54)),
            double(-(1ll<<55)),
            double(-(1ll<<56)),
            double((1ll<<54) - 1),
            double((1ll<<55) - 1),
            double((1ll<<56) - 1),
            double(-((1ll<<54) - 1)),
            double(-((1ll<<55) - 1)),
            double(-((1ll<<56) - 1)),
            1.1,
            0.1,
            -0.1,
            -1.1,
            1e200,
            -1e200
        };
        int n = sizeof(numbers)/sizeof(double);

        JsonArray array;
        for (int i = 0; i < n; ++i)
            array.append(numbers[i]);

        std::string serialized = JsonDocument(array).toJson();
        JsonDocument json = JsonDocument::fromJson(serialized);
        JsonArray array2 = json.array();

        QCOMPARE(array.size(), array2.size());
        for (int i = 0; i < array.size(); ++i) {
            QCOMPARE(array.at(i).type(), JsonValue::Double);
            QCOMPARE(array.at(i).toDouble(), numbers[i]);
            QCOMPARE(array2.at(i).type(), JsonValue::Double);
            QCOMPARE(array2.at(i).toDouble(), numbers[i]);
        }
    }

}

void tst_Json::testNumbers_2()
{
    // test cases from TC39 test suite for ECMAScript
    // http://hg.ecmascript.org/tests/test262/file/d067d2f0ca30/test/suite/ch08/8.5/8.5.1.js

    // Fill an array with 2 to the power of (0 ... -1075)
    double value = 1;
    double floatValues[1076], floatValues_1[1076];
    JsonObject jObject;
    for (int power = 0; power <= 1075; power++) {
        floatValues[power] = value;
        jObject.insert(std::to_string(power), JsonValue(floatValues[power]));
        // Use basic math operations for testing, which are required to support 'gradual underflow' rather
        // than Math.pow etc..., which are defined as 'implementation dependent'.
        value = value * 0.5;
    }

    JsonDocument jDocument1(jObject);
    std::string ba(jDocument1.toJson());

    JsonDocument jDocument2(JsonDocument::fromJson(ba));
    for (int power = 0; power <= 1075; power++) {
        floatValues_1[power] = jDocument2.object().value(std::to_string(power)).toDouble();
#ifdef Q_OS_QNX
        if (power >= 970)
            QEXPECT_FAIL("", "See QTBUG-37066", Abort);
#endif
        QVERIFY2(floatValues[power] == floatValues_1[power],
                 QString::fromLatin1("floatValues[%1] != floatValues_1[%1]").arg(power).toLatin1());
    }

    // The last value is below min denorm and should round to 0, everything else should contain a value
    QVERIFY2(floatValues_1[1075] == 0, "Value after min denorm should round to 0");

    // Validate the last actual value is min denorm
    QVERIFY2(floatValues_1[1074] == 4.9406564584124654417656879286822e-324,
            QString::fromLatin1("Min denorm value is incorrect: %1").arg(floatValues_1[1074]).toLatin1());

    // Validate that every value is half the value before it up to 1
    for (int index = 1074; index > 0; index--) {
        QVERIFY2(floatValues_1[index] != 0,
                 QString::fromLatin1("2**- %1 should not be 0").arg(index).toLatin1());

        QVERIFY2(floatValues_1[index - 1] == (floatValues_1[index] * 2),
                QString::fromLatin1("Value should be double adjacent value at index %1").arg(index).toLatin1());
    }
}

void tst_Json::testNumbers_3()
{
    // test case from QTBUG-31926
    double d1 = 1.123451234512345;
    double d2 = 1.123451234512346;

    JsonObject jObject;
    jObject.insert("d1", JsonValue(d1));
    jObject.insert("d2", JsonValue(d2));
    JsonDocument jDocument1(jObject);
    std::string ba(jDocument1.toJson());

    JsonDocument jDocument2(JsonDocument::fromJson(ba));

    double d1_1(jDocument2.object().value("d1").toDouble());
    double d2_1(jDocument2.object().value("d2").toDouble());
    QVERIFY(d1_1 != d2_1);
}

void tst_Json::testObjectSimple()
{
    JsonObject object;
    object.insert("number", 999.);
    QCOMPARE(object.value("number").type(), JsonValue::Double);
    QCOMPARE(object.value("number").toDouble(), 999.);
    object.insert("string", std::string("test"));
    QCOMPARE(object.value("string").type(), JsonValue::String);
    QCOMPARE(object.value("string").toString(), std::string("test"));
    object.insert("boolean", true);
    QCOMPARE(object.value("boolean").toBool(), true);

    JsonObject::Keys keys = object.keys();
    QVERIFY2(contains(keys, "number"), "key number not found");
    QVERIFY2(contains(keys, "string"), "key string not found");
    QVERIFY2(contains(keys, "boolean"), "key boolean not found");

    // if we put a JsonValue into the JsonObject and retrieve
    // it, it should be identical.
    JsonValue value("foo");
    object.insert("value", value);
    QCOMPARE(object.value("value"), value);

    int size = object.size();
    object.remove("boolean");
    QCOMPARE(object.size(), size - 1);
    QVERIFY2(!object.contains("boolean"), "key boolean should have been removed");

    JsonValue taken = object.take("value");
    QCOMPARE(taken, value);
    QVERIFY2(!object.contains("value"), "key value should have been removed");

    std::string before = object.value("string").toString();
    object.insert("string", std::string("foo"));
    QVERIFY2(object.value("string").toString() != before, "value should have been updated");

    size = object.size();
    JsonObject subobject;
    subobject.insert("number", 42);
    subobject.insert("string", "foobar");
    object.insert("subobject", subobject);
    QCOMPARE(object.size(), size+1);
    JsonValue subvalue = object.take("subobject");
    QCOMPARE(object.size(), size);
    QCOMPARE(subvalue.toObject(), subobject);
    // make object detach by modifying it many times
    for (int i = 0; i < 64; ++i)
        object.insert("string", "bar");
    QCOMPARE(object.size(), size);
    QCOMPARE(subvalue.toObject(), subobject);
}

void tst_Json::testObjectSmallKeys()
{
    JsonObject data1;
    data1.insert("1", 123.);
    QVERIFY(data1.contains("1"));
    QCOMPARE(data1.value("1").toDouble(), (double)123);
    data1.insert("12", 133.);
    QCOMPARE(data1.value("12").toDouble(), (double)133);
    QVERIFY(data1.contains("12"));
    data1.insert("123", 323.);
    QCOMPARE(data1.value("12").toDouble(), (double)133);
    QVERIFY(data1.contains("123"));
    QCOMPARE(data1.value("123").type(), JsonValue::Double);
    QCOMPARE(data1.value("123").toDouble(), (double)323);
}

void tst_Json::testArraySimple()
{
    JsonArray array;
    array.append(999.);
    array.append(std::string("test"));
    array.append(true);

    JsonValue val = array.at(0);
    QCOMPARE(array.at(0).toDouble(), 999.);
    QCOMPARE(array.at(1).toString(), std::string("test"));
    QCOMPARE(array.at(2).toBool(), true);
    QCOMPARE(array.size(), 3);

    // if we put a JsonValue into the JsonArray and retrieve
    // it, it should be identical.
    JsonValue value("foo");
    array.append(value);
    QCOMPARE(array.at(3), value);

    int size = array.size();
    array.removeAt(2);
    --size;
    QCOMPARE(array.size(), size);

    JsonValue taken = array.takeAt(0);
    --size;
    QCOMPARE(taken.toDouble(), 999.);
    QCOMPARE(array.size(), size);

    // check whether null values work
    array.append(JsonValue());
    ++size;
    QCOMPARE(array.size(), size);
    QCOMPARE(array.last().type(), JsonValue::Null);
    QCOMPARE(array.last(), JsonValue());

    QCOMPARE(array.first().type(), JsonValue::String);
    QCOMPARE(array.first(), JsonValue("test"));

    array.prepend(false);
    QCOMPARE(array.first().type(), JsonValue::Bool);
    QCOMPARE(array.first(), JsonValue(false));

    QCOMPARE(array.at(-1), JsonValue(JsonValue::Undefined));
    QCOMPARE(array.at(array.size()), JsonValue(JsonValue::Undefined));

    array.replace(0, -555.);
    QCOMPARE(array.first().type(), JsonValue::Double);
    QCOMPARE(array.first(), JsonValue(-555.));
    QCOMPARE(array.at(1).type(), JsonValue::String);
    QCOMPARE(array.at(1), JsonValue("test"));
}

void tst_Json::testValueObject()
{
    JsonObject object;
    object.insert("number", 999.);
    object.insert("string", "test");
    object.insert("boolean", true);

    JsonValue value(object);

    // if we don't modify the original JsonObject, toObject()
    // on the JsonValue should return the same object (non-detached).
    QCOMPARE(value.toObject(), object);

    // if we modify the original object, it should detach
    object.insert("test", JsonValue("test"));
    QVERIFY2(value.toObject() != object, "object should have detached");
}

void tst_Json::testValueArray()
{
    JsonArray array;
    array.append(999.);
    array.append("test");
    array.append(true);

    JsonValue value(array);

    // if we don't modify the original JsonArray, toArray()
    // on the JsonValue should return the same object (non-detached).
    QCOMPARE(value.toArray(), array);

    // if we modify the original array, it should detach
    array.append("test");
    QVERIFY2(value.toArray() != array, "array should have detached");
}

void tst_Json::testObjectNested()
{
    JsonObject inner, outer;
    inner.insert("number", 999.);
    outer.insert("nested", inner);

    // if we don't modify the original JsonObject, value()
    // should return the same object (non-detached).
    JsonObject value = outer.value("nested").toObject();
    QCOMPARE(value, inner);
    QCOMPARE(value.value("number").toDouble(), 999.);

    // if we modify the original object, it should detach and not
    // affect the nested object
    inner.insert("number", 555.);
    value = outer.value("nested").toObject();
    QVERIFY2(inner.value("number").toDouble() != value.value("number").toDouble(),
             "object should have detached");

    // array in object
    JsonArray array;
    array.append(123.);
    array.append(456.);
    outer.insert("array", array);
    QCOMPARE(outer.value("array").toArray(), array);
    QCOMPARE(outer.value("array").toArray().at(1).toDouble(), 456.);

    // two deep objects
    JsonObject twoDeep;
    twoDeep.insert("boolean", true);
    inner.insert("nested", twoDeep);
    outer.insert("nested", inner);
    QCOMPARE(outer.value("nested").toObject().value("nested").toObject(), twoDeep);
    QCOMPARE(outer.value("nested").toObject().value("nested").toObject().value("boolean").toBool(),
             true);
}

void tst_Json::testArrayNested()
{
    JsonArray inner, outer;
    inner.append(999.);
    outer.append(inner);

    // if we don't modify the original JsonArray, value()
    // should return the same array (non-detached).
    JsonArray value = outer.at(0).toArray();
    QCOMPARE(value, inner);
    QCOMPARE(value.at(0).toDouble(), 999.);

    // if we modify the original array, it should detach and not
    // affect the nested array
    inner.append(555.);
    value = outer.at(0).toArray();
    QVERIFY2(inner.size() != value.size(), "array should have detached");

    // objects in arrays
    JsonObject object;
    object.insert("boolean", true);
    outer.append(object);
    QCOMPARE(outer.last().toObject(), object);
    QCOMPARE(outer.last().toObject().value("boolean").toBool(), true);

    // two deep arrays
    JsonArray twoDeep;
    twoDeep.append(JsonValue("nested"));
    inner.append(twoDeep);
    outer.append(inner);
    QCOMPARE(outer.last().toArray().last().toArray(), twoDeep);
    QCOMPARE(outer.last().toArray().last().toArray().at(0).toString(), std::string("nested"));
}

void tst_Json::testArrayNestedEmpty()
{
    JsonObject object;
    JsonArray inner;
    object.insert("inner", inner);
    JsonValue val = object.value("inner");
    JsonArray value = object.value("inner").toArray();
    QCOMPARE(value.size(), 0);
    QCOMPARE(value, inner);
    QCOMPARE(value.size(), 0);
    object.insert("count", 0.);
    QCOMPARE(object.value("inner").toArray().size(), 0);
    QVERIFY(object.value("inner").toArray().isEmpty());
    JsonDocument(object).toBinaryData();
    QCOMPARE(object.value("inner").toArray().size(), 0);
}

void tst_Json::testObjectNestedEmpty()
{
    JsonObject object;
    JsonObject inner;
    JsonObject inner2;
    object.insert("inner", inner);
    object.insert("inner2", inner2);
    JsonObject value = object.value("inner").toObject();
    QCOMPARE(value.size(), 0);
    QCOMPARE(value, inner);
    QCOMPARE(value.size(), 0);
    object.insert("count", 0.);
    QCOMPARE(object.value("inner").toObject().size(), 0);
    QCOMPARE(object.value("inner").type(), JsonValue::Object);
    JsonDocument(object).toBinaryData();
    QVERIFY(object.value("inner").toObject().isEmpty());
    QVERIFY(object.value("inner2").toObject().isEmpty());
    JsonDocument doc = JsonDocument::fromBinaryData(JsonDocument(object).toBinaryData());
    QVERIFY(!doc.isNull());
    JsonObject reconstituted(doc.object());
    QCOMPARE(reconstituted.value("inner").toObject().size(), 0);
    QCOMPARE(reconstituted.value("inner").type(), JsonValue::Object);
    QCOMPARE(reconstituted.value("inner2").type(), JsonValue::Object);
}

void tst_Json::testValueRef()
{
    JsonArray array;
    array.append(1.);
    array.append(2.);
    array.append(3.);
    array.append(4);
    array.append(4.1);
    array[1] = false;

    QCOMPARE(array.size(), 5);
    QCOMPARE(array.at(0).toDouble(), 1.);
    QCOMPARE(array.at(2).toDouble(), 3.);
    QCOMPARE(array.at(3).toInt(), 4);
    QCOMPARE(array.at(4).toInt(), 0);
    QCOMPARE(array.at(1).type(), JsonValue::Bool);
    QCOMPARE(array.at(1).toBool(), false);

    JsonObject object;
    object["key"] = true;
    QCOMPARE(object.size(), 1);
    object.insert("null", JsonValue());
    QCOMPARE(object.value("null"), JsonValue());
    object["null"] = 100.;
    QCOMPARE(object.value("null").type(), JsonValue::Double);
    JsonValue val = object["null"];
    QCOMPARE(val.toDouble(), 100.);
    QCOMPARE(object.size(), 2);

    array[1] = array[2] = object["key"] = 42;
    QCOMPARE(array[1], array[2]);
    QCOMPARE(array[2], object["key"]);
    QCOMPARE(object.value("key"), JsonValue(42));
}

void tst_Json::testObjectIteration()
{
    JsonObject object;

    for (JsonObject::iterator it = object.begin(); it != object.end(); ++it)
        QVERIFY(false);

    const std::string property = "kkk";
    object.insert(property, 11);
    object.take(property);
    for (JsonObject::iterator it = object.begin(); it != object.end(); ++it)
        QVERIFY(false);

    for (int i = 0; i < 10; ++i)
        object[std::to_string(i)] = (double)i;

    QCOMPARE(object.size(), 10);

    QCOMPARE(object.begin()->toDouble(), object.constBegin()->toDouble());

    for (JsonObject::iterator it = object.begin(); it != object.end(); ++it) {
        JsonValue value = it.value();
        QCOMPARE((double)atoi(it.key().data()), value.toDouble());
    }

    {
        JsonObject object2 = object;
        QCOMPARE(object, object2);

        JsonValue val = *object2.begin();
        object2.erase(object2.begin());
        QCOMPARE(object.size(), 10);
        QCOMPARE(object2.size(), 9);

        for (JsonObject::const_iterator it = object2.constBegin(); it != object2.constEnd(); ++it) {
            JsonValue value = it.value();
            QVERIFY(it.value() != val);
            QCOMPARE((double)atoi(it.key().data()), value.toDouble());
        }
    }

    {
        JsonObject object2 = object;
        QCOMPARE(object, object2);

        JsonObject::iterator it = object2.find(std::to_string(5));
        object2.erase(it);
        QCOMPARE(object.size(), 10);
        QCOMPARE(object2.size(), 9);
    }

    {
        JsonObject::iterator it = object.begin();
        it += 5;
        QCOMPARE(JsonValue(it.value()).toDouble(), 5.);
        it -= 3;
        QCOMPARE(JsonValue(it.value()).toDouble(), 2.);
        JsonObject::iterator it2 = it + 5;
        QCOMPARE(JsonValue(it2.value()).toDouble(), 7.);
        it2 = it - 1;
        QCOMPARE(JsonValue(it2.value()).toDouble(), 1.);
    }

    {
        JsonObject::const_iterator it = object.constBegin();
        it += 5;
        QCOMPARE(JsonValue(it.value()).toDouble(), 5.);
        it -= 3;
        QCOMPARE(JsonValue(it.value()).toDouble(), 2.);
        JsonObject::const_iterator it2 = it + 5;
        QCOMPARE(JsonValue(it2.value()).toDouble(), 7.);
        it2 = it - 1;
        QCOMPARE(JsonValue(it2.value()).toDouble(), 1.);
    }

    JsonObject::iterator it = object.begin();
    while (!object.isEmpty())
        it = object.erase(it);
    QCOMPARE(object.size() , 0);
    QCOMPARE(it, object.end());
}

void tst_Json::testArrayIteration()
{
    JsonArray array;
    for (int i = 0; i < 10; ++i)
        array.append(i);

    QCOMPARE(array.size(), 10);

    int i = 0;
    for (JsonArray::iterator it = array.begin(); it != array.end(); ++it, ++i) {
        JsonValue value = (*it);
        QCOMPARE((double)i, value.toDouble());
    }

    QCOMPARE(array.begin()->toDouble(), array.constBegin()->toDouble());

    {
        JsonArray array2 = array;
        QCOMPARE(array, array2);

        JsonValue val = *array2.begin();
        array2.erase(array2.begin());
        QCOMPARE(array.size(), 10);
        QCOMPARE(array2.size(), 9);

        i = 1;
        for (JsonArray::const_iterator it = array2.constBegin(); it != array2.constEnd(); ++it, ++i) {
            JsonValue value = (*it);
            QCOMPARE((double)i, value.toDouble());
        }
    }

    {
        JsonArray::iterator it = array.begin();
        it += 5;
        QCOMPARE(JsonValue((*it)).toDouble(), 5.);
        it -= 3;
        QCOMPARE(JsonValue((*it)).toDouble(), 2.);
        JsonArray::iterator it2 = it + 5;
        QCOMPARE(JsonValue(*it2).toDouble(), 7.);
        it2 = it - 1;
        QCOMPARE(JsonValue(*it2).toDouble(), 1.);
    }

    {
        JsonArray::const_iterator it = array.constBegin();
        it += 5;
        QCOMPARE(JsonValue((*it)).toDouble(), 5.);
        it -= 3;
        QCOMPARE(JsonValue((*it)).toDouble(), 2.);
        JsonArray::const_iterator it2 = it + 5;
        QCOMPARE(JsonValue(*it2).toDouble(), 7.);
        it2 = it - 1;
        QCOMPARE(JsonValue(*it2).toDouble(), 1.);
    }

    JsonArray::iterator it = array.begin();
    while (!array.isEmpty())
        it = array.erase(it);
    QCOMPARE(array.size() , 0);
    QCOMPARE(it, array.end());
}

void tst_Json::testObjectFind()
{
    JsonObject object;
    for (int i = 0; i < 10; ++i)
        object[std::to_string(i)] = i;

    QCOMPARE(object.size(), 10);

    JsonObject::iterator it = object.find("1");
    QCOMPARE((*it).toDouble(), 1.);
    it = object.find("11");
    QCOMPARE((*it).type(), JsonValue::Undefined);
    QCOMPARE(it, object.end());

    JsonObject::const_iterator cit = object.constFind("1");
    QCOMPARE((*cit).toDouble(), 1.);
    cit = object.constFind("11");
    QCOMPARE((*it).type(), JsonValue::Undefined);
    QCOMPARE(it, object.end());
}

void tst_Json::testDocument()
{
    JsonDocument doc;
    QCOMPARE(doc.isEmpty(), true);
    QCOMPARE(doc.isArray(), false);
    QCOMPARE(doc.isObject(), false);

    JsonObject object;
    doc.setObject(object);
    QCOMPARE(doc.isEmpty(), false);
    QCOMPARE(doc.isArray(), false);
    QCOMPARE(doc.isObject(), true);

    object.insert("Key", "Value");
    doc.setObject(object);
    QCOMPARE(doc.isEmpty(), false);
    QCOMPARE(doc.isArray(), false);
    QCOMPARE(doc.isObject(), true);
    QCOMPARE(doc.object(), object);
    QCOMPARE(doc.array(), JsonArray());

    doc = JsonDocument();
    QCOMPARE(doc.isEmpty(), true);
    QCOMPARE(doc.isArray(), false);
    QCOMPARE(doc.isObject(), false);

    JsonArray array;
    doc.setArray(array);
    QCOMPARE(doc.isEmpty(), false);
    QCOMPARE(doc.isArray(), true);
    QCOMPARE(doc.isObject(), false);

    array.append("Value");
    doc.setArray(array);
    QCOMPARE(doc.isEmpty(), false);
    QCOMPARE(doc.isArray(), true);
    QCOMPARE(doc.isObject(), false);
    QCOMPARE(doc.array(), array);
    QCOMPARE(doc.object(), JsonObject());

    JsonObject outer;
    outer.insert("outerKey", 22);
    JsonObject inner;
    inner.insert("innerKey", 42);
    outer.insert("innter", inner);
    JsonArray innerArray;
    innerArray.append(23);
    outer.insert("innterArray", innerArray);

    JsonDocument doc2(outer.value("innter").toObject());
    QVERIFY(doc2.object().contains("innerKey"));
    QCOMPARE(doc2.object().value("innerKey"), JsonValue(42));

    JsonDocument doc3;
    doc3.setObject(outer.value("innter").toObject());
    QCOMPARE(doc3.isArray(), false);
    QCOMPARE(doc3.isObject(), true);
    QVERIFY(doc3.object().contains("innerKey"));
    QCOMPARE(doc3.object().value("innerKey"), JsonValue(42));

    JsonDocument doc4(outer.value("innterArray").toArray());
    QCOMPARE(doc4.isArray(), true);
    QCOMPARE(doc4.isObject(), false);
    QCOMPARE(doc4.array().size(), 1);
    QCOMPARE(doc4.array().at(0), JsonValue(23));

    JsonDocument doc5;
    doc5.setArray(outer.value("innterArray").toArray());
    QCOMPARE(doc5.isArray(), true);
    QCOMPARE(doc5.isObject(), false);
    QCOMPARE(doc5.array().size(), 1);
    QCOMPARE(doc5.array().at(0), JsonValue(23));
}

void tst_Json::nullValues()
{
    JsonArray array;
    array.append(JsonValue());

    QCOMPARE(array.size(), 1);
    QCOMPARE(array.at(0), JsonValue());

    JsonObject object;
    object.insert("key", JsonValue());
    QCOMPARE(object.contains("key"), true);
    QCOMPARE(object.size(), 1);
    QCOMPARE(object.value("key"), JsonValue());
}

void tst_Json::nullArrays()
{
    JsonArray nullArray;
    JsonArray nonNull;
    nonNull.append("bar");

    QCOMPARE(nullArray, JsonArray());
    QVERIFY(nullArray != nonNull);
    QVERIFY(nonNull != nullArray);

    QCOMPARE(nullArray.size(), 0);
    QCOMPARE(nullArray.takeAt(0), JsonValue(JsonValue::Undefined));
    QCOMPARE(nullArray.first(), JsonValue(JsonValue::Undefined));
    QCOMPARE(nullArray.last(), JsonValue(JsonValue::Undefined));
    nullArray.removeAt(0);
    nullArray.removeAt(-1);

    nullArray.append("bar");
    nullArray.removeAt(0);

    QCOMPARE(nullArray.size(), 0);
    QCOMPARE(nullArray.takeAt(0), JsonValue(JsonValue::Undefined));
    QCOMPARE(nullArray.first(), JsonValue(JsonValue::Undefined));
    QCOMPARE(nullArray.last(), JsonValue(JsonValue::Undefined));
    nullArray.removeAt(0);
    nullArray.removeAt(-1);
}

void tst_Json::nullObject()
{
    JsonObject nullObject;
    JsonObject nonNull;
    nonNull.insert("foo", "bar");

    QCOMPARE(nullObject, JsonObject());
    QVERIFY(nullObject != nonNull);
    QVERIFY(nonNull != nullObject);

    QCOMPARE(nullObject.size(), 0);
    QCOMPARE(nullObject.keys(), JsonObject::Keys());
    nullObject.remove("foo");
    QCOMPARE(nullObject, JsonObject());
    QCOMPARE(nullObject.take("foo"), JsonValue(JsonValue::Undefined));
    QCOMPARE(nullObject.contains("foo"), false);

    nullObject.insert("foo", "bar");
    nullObject.remove("foo");

    QCOMPARE(nullObject.size(), 0);
    QCOMPARE(nullObject.keys(), JsonObject::Keys());
    nullObject.remove("foo");
    QCOMPARE(nullObject, JsonObject());
    QCOMPARE(nullObject.take("foo"), JsonValue(JsonValue::Undefined));
    QCOMPARE(nullObject.contains("foo"), false);
}

void tst_Json::constNullObject()
{
    const JsonObject nullObject;
    JsonObject nonNull;
    nonNull.insert("foo", "bar");

    QCOMPARE(nullObject, JsonObject());
    QVERIFY(nullObject != nonNull);
    QVERIFY(nonNull != nullObject);

    QCOMPARE(nullObject.size(), 0);
    QCOMPARE(nullObject.keys(), JsonObject::Keys());
    QCOMPARE(nullObject, JsonObject());
    QCOMPARE(nullObject.contains("foo"), false);
    QCOMPARE(nullObject["foo"], JsonValue(JsonValue::Undefined));
}

void tst_Json::keySorting()
{
    const char *json = "{ \"B\": true, \"A\": false }";
    JsonDocument doc = JsonDocument::fromJson(json);

    QCOMPARE(doc.isObject(), true);

    JsonObject o = doc.object();
    QCOMPARE(o.size(), 2);
    JsonObject::const_iterator it = o.constBegin();
    QCOMPARE(it.key(), std::string("A"));
    ++it;
    QCOMPARE(it.key(), std::string("B"));

    JsonObject::Keys keys;
    keys.push_back("A");
    keys.push_back("B");
    QCOMPARE(o.keys(), keys);
}

void tst_Json::undefinedValues()
{
    JsonObject object;
    object.insert("Key", JsonValue(JsonValue::Undefined));
    QCOMPARE(object.size(), 0);

    object.insert("Key", "Value");
    QCOMPARE(object.size(), 1);
    QCOMPARE(object.value("Key").type(), JsonValue::String);
    QCOMPARE(object.value("foo").type(), JsonValue::Undefined);
    object.insert("Key", JsonValue(JsonValue::Undefined));
    QCOMPARE(object.size(), 0);
    QCOMPARE(object.value("Key").type(), JsonValue::Undefined);

    JsonArray array;
    array.append(JsonValue(JsonValue::Undefined));
    QCOMPARE(array.size(), 1);
    QCOMPARE(array.at(0).type(), JsonValue::Null);

    QCOMPARE(array.at(1).type(), JsonValue::Undefined);
    QCOMPARE(array.at(-1).type(), JsonValue::Undefined);
}

void tst_Json::toJson()
{
    // Test JsonDocument::Indented format
    {
        JsonObject object;
        object.insert("\\Key\n", "Value");
        object.insert("null", JsonValue());
        JsonArray array;
        array.append(true);
        array.append(999.);
        array.append("string");
        array.append(JsonValue());
        array.append("\\\a\n\r\b\tabcABC\"");
        object.insert("Array", array);

        std::string json = JsonDocument(object).toJson();

        std::string expected =
                "{\n"
                "    \"Array\": [\n"
                "        true,\n"
                "        999,\n"
                "        \"string\",\n"
                "        null,\n"
                "        \"\\\\\\u0007\\n\\r\\b\\tabcABC\\\"\"\n"
                "    ],\n"
                "    \"\\\\Key\\n\": \"Value\",\n"
                "    \"null\": null\n"
                "}\n";
        QCOMPARE(json, expected);

        JsonDocument doc;
        doc.setObject(object);
        json = doc.toJson();
        QCOMPARE(json, expected);

        doc.setArray(array);
        json = doc.toJson();
        expected =
                "[\n"
                "    true,\n"
                "    999,\n"
                "    \"string\",\n"
                "    null,\n"
                "    \"\\\\\\u0007\\n\\r\\b\\tabcABC\\\"\"\n"
                "]\n";
        QCOMPARE(json, expected);
    }

    // Test JsonDocument::Compact format
    {
        JsonObject object;
        object.insert("\\Key\n", "Value");
        object.insert("null", JsonValue());
        JsonArray array;
        array.append(true);
        array.append(999.);
        array.append("string");
        array.append(JsonValue());
        array.append("\\\a\n\r\b\tabcABC\"");
        object.insert("Array", array);

        std::string json = JsonDocument(object).toJson(JsonDocument::Compact);
        std::string expected =
                "{\"Array\":[true,999,\"string\",null,\"\\\\\\u0007\\n\\r\\b\\tabcABC\\\"\"],\"\\\\Key\\n\":\"Value\",\"null\":null}";
        QCOMPARE(json, expected);

        JsonDocument doc;
        doc.setObject(object);
        json = doc.toJson(JsonDocument::Compact);
        QCOMPARE(json, expected);

        doc.setArray(array);
        json = doc.toJson(JsonDocument::Compact);
        expected = "[true,999,\"string\",null,\"\\\\\\u0007\\n\\r\\b\\tabcABC\\\"\"]";
        QCOMPARE(json, expected);
    }
}

void tst_Json::toJsonSillyNumericValues()
{
    JsonObject object;
    JsonArray array;
    array.append(JsonValue(std::numeric_limits<double>::infinity()));  // encode to: null
    array.append(JsonValue(-std::numeric_limits<double>::infinity())); // encode to: null
    array.append(JsonValue(std::numeric_limits<double>::quiet_NaN())); // encode to: null
    object.insert("Array", array);

    std::string json = JsonDocument(object).toJson();

    std::string expected =
            "{\n"
            "    \"Array\": [\n"
            "        null,\n"
            "        null,\n"
            "        null\n"
            "    ]\n"
            "}\n";

    QCOMPARE(json, expected);

    JsonDocument doc;
    doc.setObject(object);
    json = doc.toJson();
    QCOMPARE(json, expected);
}

void tst_Json::toJsonLargeNumericValues()
{
    JsonObject object;
    JsonArray array;
    array.append(JsonValue(1.234567)); // actual precision bug in Qt 5.0.0
    array.append(JsonValue(1.7976931348623157e+308)); // JS Number.MAX_VALUE
    array.append(JsonValue(5e-324));                  // JS Number.MIN_VALUE
    array.append(JsonValue(std::numeric_limits<double>::min()));
    array.append(JsonValue(std::numeric_limits<double>::max()));
    array.append(JsonValue(std::numeric_limits<double>::epsilon()));
    array.append(JsonValue(std::numeric_limits<double>::denorm_min()));
    array.append(JsonValue(0.0));
    array.append(JsonValue(-std::numeric_limits<double>::min()));
    array.append(JsonValue(-std::numeric_limits<double>::max()));
    array.append(JsonValue(-std::numeric_limits<double>::epsilon()));
    array.append(JsonValue(-std::numeric_limits<double>::denorm_min()));
    array.append(JsonValue(-0.0));
    array.append(JsonValue(int64_t(9007199254740992LL)));  // JS Number max integer
    array.append(JsonValue(int64_t(-9007199254740992LL))); // JS Number min integer
    object.insert("Array", array);

    std::string json = JsonDocument(object).toJson();

    std::string expected =
            "{\n"
            "    \"Array\": [\n"
            "        1.234567,\n"
            "        1.7976931348623157e+308,\n"
            //     ((4.9406564584124654e-324 == 5e-324) == true)
            // I can only think JavaScript has a special formatter to
            //  emit this value for this IEEE754 bit pattern.
            "        4.9406564584124654e-324,\n"
            "        2.2250738585072014e-308,\n"
            "        1.7976931348623157e+308,\n"
            "        2.2204460492503131e-16,\n"
            "        4.9406564584124654e-324,\n"
            "        0,\n"
            "        -2.2250738585072014e-308,\n"
            "        -1.7976931348623157e+308,\n"
            "        -2.2204460492503131e-16,\n"
            "        -4.9406564584124654e-324,\n"
            "        0,\n"
            "        9007199254740992,\n"
            "        -9007199254740992\n"
            "    ]\n"
            "}\n";

    QCOMPARE(json, expected);

    JsonDocument doc;
    doc.setObject(object);
    json = doc.toJson();
    QCOMPARE(json, expected);
}

void tst_Json::fromJson()
{
    {
        std::string json = "[\n    true\n]\n";
        JsonDocument doc = JsonDocument::fromJson(json);
        QVERIFY(!doc.isEmpty());
        QCOMPARE(doc.isArray(), true);
        QCOMPARE(doc.isObject(), false);
        JsonArray array = doc.array();
        QCOMPARE(array.size(), 1);
        QCOMPARE(array.at(0).type(), JsonValue::Bool);
        QCOMPARE(array.at(0).toBool(), true);
        QCOMPARE(doc.toJson(), json);
    }
    {
        //regression test: test if unicode_control_characters are correctly decoded
        std::string json = "[\n    \"" UNICODE_NON_CHARACTER "\"\n]\n";
        JsonDocument doc = JsonDocument::fromJson(json);
        QVERIFY(!doc.isEmpty());
        QCOMPARE(doc.isArray(), true);
        QCOMPARE(doc.isObject(), false);
        JsonArray array = doc.array();
        QCOMPARE(array.size(), 1);
        QCOMPARE(array.at(0).type(), JsonValue::String);
        QCOMPARE(array.at(0).toString(), std::string(UNICODE_NON_CHARACTER));
        QCOMPARE(doc.toJson(), json);
    }
    {
        std::string json = "[]";
        JsonDocument doc = JsonDocument::fromJson(json);
        QVERIFY(!doc.isEmpty());
        QCOMPARE(doc.isArray(), true);
        QCOMPARE(doc.isObject(), false);
        JsonArray array = doc.array();
        QCOMPARE(array.size(), 0);
    }
    {
        std::string json = "{}";
        JsonDocument doc = JsonDocument::fromJson(json);
        QVERIFY(!doc.isEmpty());
        QCOMPARE(doc.isArray(), false);
        QCOMPARE(doc.isObject(), true);
        JsonObject object = doc.object();
        QCOMPARE(object.size(), 0);
    }
    {
        std::string json = "{\n    \"Key\": true\n}\n";
        JsonDocument doc = JsonDocument::fromJson(json);
        QVERIFY(!doc.isEmpty());
        QCOMPARE(doc.isArray(), false);
        QCOMPARE(doc.isObject(), true);
        JsonObject object = doc.object();
        QCOMPARE(object.size(), 1);
        QCOMPARE(object.value("Key"), JsonValue(true));
        QCOMPARE(doc.toJson(), json);
    }
    {
        std::string json = "[ null, true, false, \"Foo\", 1, [], {} ]";
        JsonDocument doc = JsonDocument::fromJson(json);
        QVERIFY(!doc.isEmpty());
        QCOMPARE(doc.isArray(), true);
        QCOMPARE(doc.isObject(), false);
        JsonArray array = doc.array();
        QCOMPARE(array.size(), 7);
        QCOMPARE(array.at(0).type(), JsonValue::Null);
        QCOMPARE(array.at(1).type(), JsonValue::Bool);
        QCOMPARE(array.at(1).toBool(), true);
        QCOMPARE(array.at(2).type(), JsonValue::Bool);
        QCOMPARE(array.at(2).toBool(), false);
        QCOMPARE(array.at(3).type(), JsonValue::String);
        QCOMPARE(array.at(3).toString(), std::string("Foo"));
        QCOMPARE(array.at(4).type(), JsonValue::Double);
        QCOMPARE(array.at(4).toDouble(), 1.);
        QCOMPARE(array.at(5).type(), JsonValue::Array);
        QCOMPARE(array.at(5).toArray().size(), 0);
        QCOMPARE(array.at(6).type(), JsonValue::Object);
        QCOMPARE(array.at(6).toObject().size(), 0);
    }
    {
        std::string json = "{ \"0\": null, \"1\": true, \"2\": false, \"3\": \"Foo\", \"4\": 1, \"5\": [], \"6\": {} }";
        JsonDocument doc = JsonDocument::fromJson(json);
        QVERIFY(!doc.isEmpty());
        QCOMPARE(doc.isArray(), false);
        QCOMPARE(doc.isObject(), true);
        JsonObject object = doc.object();
        QCOMPARE(object.size(), 7);
        QCOMPARE(object.value("0").type(), JsonValue::Null);
        QCOMPARE(object.value("1").type(), JsonValue::Bool);
        QCOMPARE(object.value("1").toBool(), true);
        QCOMPARE(object.value("2").type(), JsonValue::Bool);
        QCOMPARE(object.value("2").toBool(), false);
        QCOMPARE(object.value("3").type(), JsonValue::String);
        QCOMPARE(object.value("3").toString(), std::string("Foo"));
        QCOMPARE(object.value("4").type(), JsonValue::Double);
        QCOMPARE(object.value("4").toDouble(), 1.);
        QCOMPARE(object.value("5").type(), JsonValue::Array);
        QCOMPARE(object.value("5").toArray().size(), 0);
        QCOMPARE(object.value("6").type(), JsonValue::Object);
        QCOMPARE(object.value("6").toObject().size(), 0);
    }
    {
        std::string compactJson = "{\"Array\": [true,999,\"string\",null,\"\\\\\\u0007\\n\\r\\b\\tabcABC\\\"\"],\"\\\\Key\\n\": \"Value\",\"null\": null}";
        JsonDocument doc = JsonDocument::fromJson(compactJson);
        QVERIFY(!doc.isEmpty());
        QCOMPARE(doc.isArray(), false);
        QCOMPARE(doc.isObject(), true);
        JsonObject object = doc.object();
        QCOMPARE(object.size(), 3);
        QCOMPARE(object.value("\\Key\n").isString(), true);
        QCOMPARE(object.value("\\Key\n").toString(), std::string("Value"));
        QCOMPARE(object.value("null").isNull(), true);
        QCOMPARE(object.value("Array").isArray(), true);
        JsonArray array = object.value("Array").toArray();
        QCOMPARE(array.size(), 5);
        QCOMPARE(array.at(0).isBool(), true);
        QCOMPARE(array.at(0).toBool(), true);
        QCOMPARE(array.at(1).isDouble(), true);
        QCOMPARE(array.at(1).toDouble(), 999.);
        QCOMPARE(array.at(2).isString(), true);
        QCOMPARE(array.at(2).toString(), std::string("string"));
        QCOMPARE(array.at(3).isNull(), true);
        QCOMPARE(array.at(4).isString(), true);
        QCOMPARE(array.at(4).toString(), std::string("\\\a\n\r\b\tabcABC\""));
    }
}

void tst_Json::fromJsonErrors()
{
    {
        JsonParseError error;
        std::string json = "{\n    \n\n";
        JsonDocument doc = JsonDocument::fromJson(json, &error);
        QVERIFY(doc.isEmpty());
        QCOMPARE(error.error, JsonParseError::UnterminatedObject);
        QCOMPARE(error.offset, 8);
    }
    {
        JsonParseError error;
        std::string json = "{\n    \"key\" 10\n";
        JsonDocument doc = JsonDocument::fromJson(json, &error);
        QVERIFY(doc.isEmpty());
        QCOMPARE(error.error, JsonParseError::MissingNameSeparator);
        QCOMPARE(error.offset, 13);
    }
    {
        JsonParseError error;
        std::string json = "[\n    \n\n";
        JsonDocument doc = JsonDocument::fromJson(json, &error);
        QVERIFY(doc.isEmpty());
        QCOMPARE(error.error, JsonParseError::UnterminatedArray);
        QCOMPARE(error.offset, 8);
    }
    {
        JsonParseError error;
        std::string json = "[\n   1, true\n\n";
        JsonDocument doc = JsonDocument::fromJson(json, &error);
        QVERIFY(doc.isEmpty());
        QCOMPARE(error.error, JsonParseError::UnterminatedArray);
        QCOMPARE(error.offset, 14);
    }
    {
        JsonParseError error;
        std::string json = "[\n  1 true\n\n";
        JsonDocument doc = JsonDocument::fromJson(json, &error);
        QVERIFY(doc.isEmpty());
        QCOMPARE(error.error, JsonParseError::MissingValueSeparator);
        QCOMPARE(error.offset, 7);
    }
    {
        JsonParseError error;
        std::string json = "[\n    nul";
        JsonDocument doc = JsonDocument::fromJson(json, &error);
        QVERIFY(doc.isEmpty());
        QCOMPARE(error.error, JsonParseError::IllegalValue);
        QCOMPARE(error.offset, 7);
    }
    {
        JsonParseError error;
        std::string json = "[\n    nulzz";
        JsonDocument doc = JsonDocument::fromJson(json, &error);
        QVERIFY(doc.isEmpty());
        QCOMPARE(error.error, JsonParseError::IllegalValue);
        QCOMPARE(error.offset, 10);
    }
    {
        JsonParseError error;
        std::string json = "[\n    tru";
        JsonDocument doc = JsonDocument::fromJson(json, &error);
        QVERIFY(doc.isEmpty());
        QCOMPARE(error.error, JsonParseError::IllegalValue);
        QCOMPARE(error.offset, 7);
    }
    {
        JsonParseError error;
        std::string json = "[\n    trud]";
        JsonDocument doc = JsonDocument::fromJson(json, &error);
        QVERIFY(doc.isEmpty());
        QCOMPARE(error.error, JsonParseError::IllegalValue);
        QCOMPARE(error.offset, 10);
    }
    {
        JsonParseError error;
        std::string json = "[\n    fal";
        JsonDocument doc = JsonDocument::fromJson(json, &error);
        QVERIFY(doc.isEmpty());
        QCOMPARE(error.error, JsonParseError::IllegalValue);
        QCOMPARE(error.offset, 7);
    }
    {
        JsonParseError error;
        std::string json = "[\n    falsd]";
        JsonDocument doc = JsonDocument::fromJson(json, &error);
        QVERIFY(doc.isEmpty());
        QCOMPARE(error.error, JsonParseError::IllegalValue);
        QCOMPARE(error.offset, 11);
    }
    {
        JsonParseError error;
        std::string json = "[\n    11111";
        JsonDocument doc = JsonDocument::fromJson(json, &error);
        QVERIFY(doc.isEmpty());
        QCOMPARE(error.error, JsonParseError::TerminationByNumber);
        QCOMPARE(error.offset, 11);
    }
    {
        JsonParseError error;
        std::string json = "[\n    -1E10000]";
        JsonDocument doc = JsonDocument::fromJson(json, &error);
        QVERIFY(doc.isEmpty());
        QCOMPARE(error.error, JsonParseError::IllegalNumber);
        QCOMPARE(error.offset, 14);
    }
    {
        /*
        JsonParseError error;
        std::string json = "[\n    -1e-10000]";
        JsonDocument doc = JsonDocument::fromJson(json, &error);
        QVERIFY(doc.isEmpty());
        QCOMPARE(error.error, JsonParseError::IllegalNumber);
        QCOMPARE(error.offset, 15);
        */
    }
    {
        JsonParseError error;
        std::string json = "[\n    \"\\u12\"]";
        JsonDocument doc = JsonDocument::fromJson(json, &error);
        QVERIFY(doc.isEmpty());
        QCOMPARE(error.error, JsonParseError::IllegalEscapeSequence);
        QCOMPARE(error.offset, 11);
    }
    {
        // This is not caught by the new parser as we don't parse
        // UTF-8 anymore, but pass it as opaque blob.
//        JsonParseError error;
//        std::string json = "[\n    \"foo" INVALID_UNICODE "bar\"]";
//        JsonDocument doc = JsonDocument::fromJson(json, &error);
//        QVERIFY(doc.isEmpty());
//        QCOMPARE(error.error, JsonParseError::IllegalUTF8String);
//        QCOMPARE(error.offset, 12);
    }
    {
        JsonParseError error;
        std::string json = "[\n    \"";
        JsonDocument doc = JsonDocument::fromJson(json, &error);
        QVERIFY(doc.isEmpty());
        QCOMPARE(error.error, JsonParseError::UnterminatedString);
        QCOMPARE(error.offset, 8);
    }
    {
        JsonParseError error;
        std::string json = "[\n    \"c" UNICODE_DJE "a\\u12\"]";
        JsonDocument doc = JsonDocument::fromJson(json, &error);
        QVERIFY(doc.isEmpty());
        QCOMPARE(error.error, JsonParseError::IllegalEscapeSequence);
        QCOMPARE(error.offset, 15);
    }
    {
        // This is not caught by the new parser as we don't parse
        // UTF-8 anymore, but pass it as opaque blob.
//        JsonParseError error;
//        std::string json = "[\n    \"c" UNICODE_DJE "a" INVALID_UNICODE "bar\"]";
//        JsonDocument doc = JsonDocument::fromJson(json, &error);
//        QVERIFY(doc.isEmpty());
//        QCOMPARE(error.error, JsonParseError::IllegalUTF8String);
//        QCOMPARE(error.offset, 13);
    }
    {
        JsonParseError error;
        std::string json = "[\n    \"c" UNICODE_DJE "a ]";
        JsonDocument doc = JsonDocument::fromJson(json, &error);
        QVERIFY(doc.isEmpty());
        QCOMPARE(error.error, JsonParseError::UnterminatedString);
        QCOMPARE(error.offset, 14);
    }
}

void tst_Json::fromBinary()
{
    QFile file(testDataDir + QLatin1String("/test.json"));
    file.open(QFile::ReadOnly);
    std::string testJson = file.readAll().data();

    JsonDocument doc = JsonDocument::fromJson(testJson);
    JsonDocument outdoc = JsonDocument::fromBinaryData(doc.toBinaryData());
    QVERIFY(!outdoc.isNull());
    QCOMPARE(doc, outdoc);

//    // Can be used to re-create test.bjson:
//    QFile b1file(testDataDir + QLatin1String("/test.bjson.x"));
//    b1file.open(QFile::WriteOnly);
//    std::string d = doc.toBinaryData();
//    b1file.write(d.data(), d.size());
//    b1file.close();

    QFile bfile(testDataDir + QLatin1String("/test.bjson"));
    bfile.open(QFile::ReadOnly);
    std::string binary = bfile.readAll().toStdString();

    JsonDocument bdoc = JsonDocument::fromBinaryData(binary);
    QVERIFY(!bdoc.isNull());
    QCOMPARE(doc, bdoc);
}

void tst_Json::toAndFromBinary_data()
{
    QTest::addColumn<QString>("filename");
    QTest::newRow("test.json") << (testDataDir + QLatin1String("/test.json"));
    QTest::newRow("test2.json") << (testDataDir + QLatin1String("/test2.json"));
}

void tst_Json::toAndFromBinary()
{
    QFETCH(QString, filename);
    QFile file(filename);
    QVERIFY(file.open(QFile::ReadOnly));
    std::string data = file.readAll().data();

    JsonDocument doc = JsonDocument::fromJson(data);
    QVERIFY(!doc.isNull());
    JsonDocument outdoc = JsonDocument::fromBinaryData(doc.toBinaryData());
    QVERIFY(!outdoc.isNull());
    QCOMPARE(doc, outdoc);
}

void tst_Json::parseNumbers()
{
    {
        // test number parsing
        struct Numbers {
            const char *str;
            int n;
        };
        Numbers numbers [] = {
            {"0", 0},
            {"1", 1},
            {"10", 10},
            {"-1", -1},
            {"100000", 100000},
            {"-999", -999}
        };
        int size = sizeof(numbers)/sizeof(Numbers);
        for (int i = 0; i < size; ++i) {
            std::string json = "[ ";
            json += numbers[i].str;
            json += " ]";
            JsonDocument doc = JsonDocument::fromJson(json);
            QVERIFY(!doc.isEmpty());
            QCOMPARE(doc.isArray(), true);
            QCOMPARE(doc.isObject(), false);
            JsonArray array = doc.array();
            QCOMPARE(array.size(), 1);
            JsonValue val = array.at(0);
            QCOMPARE(val.type(), JsonValue::Double);
            QCOMPARE(val.toDouble(), (double)numbers[i].n);
        }
    }
    {
        // test number parsing
        struct Numbers {
            const char *str;
            double n;
        };
        Numbers numbers [] = {
            {"0", 0},
            {"1", 1},
            {"10", 10},
            {"-1", -1},
            {"100000", 100000},
            {"-999", -999},
            {"1.1", 1.1},
            {"1e10", 1e10},
            {"-1.1", -1.1},
            {"-1e10", -1e10},
            {"-1E10", -1e10},
            {"1.1e10", 1.1e10},
            {"1.1e308", 1.1e308},
            {"-1.1e308", -1.1e308},
            {"1.1e-308", 1.1e-308},
            {"-1.1e-308", -1.1e-308},
            {"1.1e+308", 1.1e+308},
            {"-1.1e+308", -1.1e+308},
            {"1.e+308", 1.e+308},
            {"-1.e+308", -1.e+308}
        };
        int size = sizeof(numbers)/sizeof(Numbers);
        for (int i = 0; i < size; ++i) {
            std::string json = "[ ";
            json += numbers[i].str;
            json += " ]";
            JsonDocument doc = JsonDocument::fromJson(json);
#ifdef Q_OS_QNX
            if (0 == QString::compare(numbers[i].str, "1.1e-308"))
                QEXPECT_FAIL("", "See QTBUG-37066", Abort);
#endif
            QVERIFY(!doc.isEmpty());
            QCOMPARE(doc.isArray(), true);
            QCOMPARE(doc.isObject(), false);
            JsonArray array = doc.array();
            QCOMPARE(array.size(), 1);
            JsonValue val = array.at(0);
            QCOMPARE(val.type(), JsonValue::Double);
            QCOMPARE(val.toDouble(), numbers[i].n);
        }
    }
}

void tst_Json::parseStrings()
{
    const char *strings [] =
    {
        "Foo",
        "abc\\\"abc",
        "abc\\\\abc",
        "abc\\babc",
        "abc\\fabc",
        "abc\\nabc",
        "abc\\rabc",
        "abc\\tabc",
        "abc\\u0019abc",
        "abc" UNICODE_DJE "abc",
        UNICODE_NON_CHARACTER
    };
    int size = sizeof(strings)/sizeof(const char *);

    for (int i = 0; i < size; ++i) {
        std::string json = "[\n    \"";
        json += strings[i];
        json += "\"\n]\n";
        JsonDocument doc = JsonDocument::fromJson(json);
        QVERIFY(!doc.isEmpty());
        QCOMPARE(doc.isArray(), true);
        QCOMPARE(doc.isObject(), false);
        JsonArray array = doc.array();
        QCOMPARE(array.size(), 1);
        JsonValue val = array.at(0);
        QCOMPARE(val.type(), JsonValue::String);

        QCOMPARE(doc.toJson(), json);
    }

    struct Pairs {
        const char *in;
        const char *out;
    };
    Pairs pairs [] = {
        {"abc\\/abc", "abc/abc"},
        {"abc\\u0402abc", "abc" UNICODE_DJE "abc"},
        {"abc\\u0065abc", "abceabc"},
        {"abc\\uFFFFabc", "abc" UNICODE_NON_CHARACTER "abc"}
    };
    size = sizeof(pairs)/sizeof(Pairs);

    for (int i = 0; i < size; ++i) {
        std::string json = "[\n    \"";
        json += pairs[i].in;
        json += "\"\n]\n";
        std::string out = "[\n    \"";
        out += pairs[i].out;
        out += "\"\n]\n";
        JsonDocument doc = JsonDocument::fromJson(json);
        QVERIFY(!doc.isEmpty());
        QCOMPARE(doc.isArray(), true);
        QCOMPARE(doc.isObject(), false);
        JsonArray array = doc.array();
        QCOMPARE(array.size(), 1);
        JsonValue val = array.at(0);
        QCOMPARE(val.type(), JsonValue::String);

        QCOMPARE(doc.toJson(), out);
    }

}

void tst_Json::parseDuplicateKeys()
{
    const char *json = "{ \"B\": true, \"A\": null, \"B\": false }";

    JsonDocument doc = JsonDocument::fromJson(json);
    QCOMPARE(doc.isObject(), true);

    JsonObject o = doc.object();
    QCOMPARE(o.size(), 2);
    JsonObject::const_iterator it = o.constBegin();
    QCOMPARE(it.key(), std::string("A"));
    QCOMPARE(it.value(), JsonValue());
    ++it;
    QCOMPARE(it.key(), std::string("B"));
    QCOMPARE(it.value(), JsonValue(false));
}

void tst_Json::testParser()
{
    QFile file(testDataDir + QLatin1String("/test.json"));
    file.open(QFile::ReadOnly);
    std::string testJson = file.readAll().data();

    JsonDocument doc = JsonDocument::fromJson(testJson);
    QVERIFY(!doc.isEmpty());
}

void tst_Json::compactArray()
{
    JsonArray array;
    array.append("First Entry");
    array.append("Second Entry");
    array.append("Third Entry");
    JsonDocument doc(array);
    auto s = doc.toBinaryData().size();
    array.removeAt(1);
    doc.setArray(array);
    QVERIFY(s > doc.toBinaryData().size());
    s = doc.toBinaryData().size();
    QCOMPARE(doc.toJson(),
             std::string("[\n"
                        "    \"First Entry\",\n"
                        "    \"Third Entry\"\n"
                        "]\n"));

    array.removeAt(0);
    doc.setArray(array);
    QVERIFY(s > doc.toBinaryData().size());
    s = doc.toBinaryData().size();
    QCOMPARE(doc.toJson(),
             std::string("[\n"
                        "    \"Third Entry\"\n"
                        "]\n"));

    array.removeAt(0);
    doc.setArray(array);
    QVERIFY(s > doc.toBinaryData().size());
    s = doc.toBinaryData().size();
    QCOMPARE(doc.toJson(),
             std::string("[\n"
                        "]\n"));

}

void tst_Json::compactObject()
{
    JsonObject object;
    object.insert("Key1", "First Entry");
    object.insert("Key2", "Second Entry");
    object.insert("Key3", "Third Entry");
    JsonDocument doc(object);
    auto s = doc.toBinaryData().size();
    object.remove("Key2");
    doc.setObject(object);
    QVERIFY(s > doc.toBinaryData().size());
    s = doc.toBinaryData().size();
    QCOMPARE(doc.toJson(),
             std::string("{\n"
                        "    \"Key1\": \"First Entry\",\n"
                        "    \"Key3\": \"Third Entry\"\n"
                        "}\n"));

    object.remove("Key1");
    doc.setObject(object);
    QVERIFY(s > doc.toBinaryData().size());
    s = doc.toBinaryData().size();
    QCOMPARE(doc.toJson(),
             std::string("{\n"
                        "    \"Key3\": \"Third Entry\"\n"
                        "}\n"));

    object.remove("Key3");
    doc.setObject(object);
    QVERIFY(s > doc.toBinaryData().size());
    s = doc.toBinaryData().size();
    QCOMPARE(doc.toJson(),
             std::string("{\n"
                        "}\n"));

}

void tst_Json::validation()
{
    // this basically tests that we don't crash on corrupt data
    QFile file(testDataDir + QLatin1String("/test.json"));
    QVERIFY(file.open(QFile::ReadOnly));
    std::string testJson = file.readAll().data();
    QVERIFY(!testJson.empty());

    JsonDocument doc = JsonDocument::fromJson(testJson);
    QVERIFY(!doc.isNull());

    std::string binary = doc.toBinaryData();

    // only test the first 1000 bytes. Testing the full file takes too long
    for (int i = 0; i < 1000; ++i) {
        std::string corrupted = binary;
        corrupted[i] = char(0xff);
        JsonDocument doc = JsonDocument::fromBinaryData(corrupted);
        if (doc.isNull())
            continue;
        std::string json = doc.toJson();
    }


    QFile file2(testDataDir + QLatin1String("/test3.json"));
    file2.open(QFile::ReadOnly);
    testJson = file2.readAll().data();
    QVERIFY(!testJson.empty());

    doc = JsonDocument::fromJson(testJson);
    QVERIFY(!doc.isNull());

    binary = doc.toBinaryData();

    for (size_t i = 0; i < binary.size(); ++i) {
        std::string corrupted = binary;
        corrupted[i] = char(0xff);
        JsonDocument doc = JsonDocument::fromBinaryData(corrupted);
        if (doc.isNull())
            continue;
        std::string json = doc.toJson();

        corrupted = binary;
        corrupted[i] = 0x00;
        doc = JsonDocument::fromBinaryData(corrupted);
        if (doc.isNull())
            continue;
        json = doc.toJson();
    }
}

void tst_Json::assignToDocument()
{
    {
        const char *json = "{ \"inner\": { \"key\": true } }";
        JsonDocument doc = JsonDocument::fromJson(json);

        JsonObject o = doc.object();
        JsonValue inner = o.value("inner");

        JsonDocument innerDoc(inner.toObject());

        QVERIFY(innerDoc != doc);
        QCOMPARE(innerDoc.object(), inner.toObject());
    }
    {
        const char *json = "[ [ true ] ]";
        JsonDocument doc = JsonDocument::fromJson(json);

        JsonArray a = doc.array();
        JsonValue inner = a.at(0);

        JsonDocument innerDoc(inner.toArray());

        QVERIFY(innerDoc != doc);
        QCOMPARE(innerDoc.array(), inner.toArray());
    }
}


void tst_Json::testDuplicateKeys()
{
    JsonObject obj;
    obj.insert("foo", "bar");
    obj.insert("foo", "zap");
    QCOMPARE(obj.size(), 1);
    QCOMPARE(obj.value("foo").toString(), std::string("zap"));
}

void tst_Json::testCompaction()
{
    // modify object enough times to trigger compactionCounter
    // and make sure the data is still valid
    JsonObject obj;
    for (int i = 0; i < 33; ++i) {
        obj.remove("foo");
        obj.insert("foo", "bar");
    }
    QCOMPARE(obj.size(), 1);
    QCOMPARE(obj.value("foo").toString(), std::string("bar"));

    JsonDocument doc = JsonDocument::fromBinaryData(JsonDocument(obj).toBinaryData());
    QVERIFY(!doc.isNull());
    QVERIFY(!doc.isEmpty());
    QCOMPARE(doc.isArray(), false);
    QCOMPARE(doc.isObject(), true);
    QCOMPARE(doc.object(), obj);
}

void tst_Json::testCompactionError()
{
    JsonObject schemaObject;
    schemaObject.insert("_Type", "_SchemaType");
    schemaObject.insert("name", "Address");
    schemaObject.insert("schema", JsonObject());
    {
        JsonObject content(schemaObject);
        JsonDocument doc(content);
        QVERIFY(!doc.isNull());
        QByteArray hash = QCryptographicHash::hash(doc.toBinaryData().data(), QCryptographicHash::Md5).toHex();
        schemaObject.insert("_Version", hash.data());
    }

    JsonObject schema;
    schema.insert("streetNumber", schema.value("number").toObject());
    schemaObject.insert("schema", schema);
    {
        JsonObject content(schemaObject);
        content.remove("_Uuid");
        content.remove("_Version");
        JsonDocument doc(content);
        QVERIFY(!doc.isNull());
        QByteArray hash = QCryptographicHash::hash(doc.toBinaryData().data(), QCryptographicHash::Md5).toHex();
        schemaObject.insert("_Version", hash.data());
    }
}

void tst_Json::parseUnicodeEscapes()
{
    const std::string json = "[ \"A\\u00e4\\u00C4\" ]";

    JsonDocument doc = JsonDocument::fromJson(json);
    JsonArray array = doc.array();

    QString result = QLatin1String("A");
    result += QChar(0xe4);
    result += QChar(0xc4);

    std::string expected = result.toUtf8().data();

    QCOMPARE(array.first().toString(), expected);
}

void tst_Json::assignObjects()
{
    const char *json =
            "[ { \"Key\": 1 }, { \"Key\": 2 } ]";

    JsonDocument doc = JsonDocument::fromJson(json);
    JsonArray array = doc.array();

    JsonObject object = array.at(0).toObject();
    QCOMPARE(object.value("Key").toDouble(), 1.);

    object = array.at(1).toObject();
    QCOMPARE(object.value("Key").toDouble(), 2.);
}

void tst_Json::assignArrays()
{
    const char *json =
            "[ [ 1 ], [ 2 ] ]";

    JsonDocument doc = JsonDocument::fromJson(json);
    JsonArray array = doc.array();

    JsonArray inner = array.at(0).toArray()  ;
    QCOMPARE(inner.at(0).toDouble(), 1.);

    inner= array.at(1).toArray();
    QCOMPARE(inner.at(0).toDouble(), 2.);
}

void tst_Json::testTrailingComma()
{
    const char *jsons[] = {"{ \"Key\": 1, }", "[ { \"Key\": 1 }, ]"};

    for (unsigned i = 0; i < sizeof(jsons)/sizeof(jsons[0]); ++i) {
        JsonParseError error;
        JsonDocument doc = JsonDocument::fromJson(jsons[i], &error);
        QCOMPARE(error.error, JsonParseError::MissingObject);
    }
}

void tst_Json::testDetachBug()
{
    JsonObject dynamic;
    JsonObject embedded;

    JsonObject local;

    embedded.insert("Key1", "Value1");
    embedded.insert("Key2", "Value2");
    dynamic.insert("Bogus", "bogusValue");
    dynamic.insert("embedded", embedded);
    local = dynamic.value("embedded").toObject();

    dynamic.remove("embedded");

    QCOMPARE(local.keys().size(), size_t(2));
    local.remove("Key1");
    local.remove("Key2");
    QCOMPARE(local.keys().size(), size_t(0));

    local.insert("Key1", "anotherValue");
    QCOMPARE(local.keys().size(), size_t(1));
}

void tst_Json::valueEquals()
{
    QCOMPARE(JsonValue(), JsonValue());
    QVERIFY(JsonValue() != JsonValue(JsonValue::Undefined));
    QVERIFY(JsonValue() != JsonValue(true));
    QVERIFY(JsonValue() != JsonValue(1.));
    QVERIFY(JsonValue() != JsonValue(JsonArray()));
    QVERIFY(JsonValue() != JsonValue(JsonObject()));

    QCOMPARE(JsonValue(true), JsonValue(true));
    QVERIFY(JsonValue(true) != JsonValue(false));
    QVERIFY(JsonValue(true) != JsonValue(JsonValue::Undefined));
    QVERIFY(JsonValue(true) != JsonValue());
    QVERIFY(JsonValue(true) != JsonValue(1.));
    QVERIFY(JsonValue(true) != JsonValue(JsonArray()));
    QVERIFY(JsonValue(true) != JsonValue(JsonObject()));

    QCOMPARE(JsonValue(1), JsonValue(1));
    QVERIFY(JsonValue(1) != JsonValue(2));
    QCOMPARE(JsonValue(1), JsonValue(1.));
    QVERIFY(JsonValue(1) != JsonValue(1.1));
    QVERIFY(JsonValue(1) != JsonValue(JsonValue::Undefined));
    QVERIFY(JsonValue(1) != JsonValue());
    QVERIFY(JsonValue(1) != JsonValue(true));
    QVERIFY(JsonValue(1) != JsonValue(JsonArray()));
    QVERIFY(JsonValue(1) != JsonValue(JsonObject()));

    QCOMPARE(JsonValue(1.), JsonValue(1.));
    QVERIFY(JsonValue(1.) != JsonValue(2.));
    QVERIFY(JsonValue(1.) != JsonValue(JsonValue::Undefined));
    QVERIFY(JsonValue(1.) != JsonValue());
    QVERIFY(JsonValue(1.) != JsonValue(true));
    QVERIFY(JsonValue(1.) != JsonValue(JsonArray()));
    QVERIFY(JsonValue(1.) != JsonValue(JsonObject()));

    QCOMPARE(JsonValue(JsonArray()), JsonValue(JsonArray()));
    JsonArray nonEmptyArray;
    nonEmptyArray.append(true);
    QVERIFY(JsonValue(JsonArray()) != nonEmptyArray);
    QVERIFY(JsonValue(JsonArray()) != JsonValue(JsonValue::Undefined));
    QVERIFY(JsonValue(JsonArray()) != JsonValue());
    QVERIFY(JsonValue(JsonArray()) != JsonValue(true));
    QVERIFY(JsonValue(JsonArray()) != JsonValue(1.));
    QVERIFY(JsonValue(JsonArray()) != JsonValue(JsonObject()));

    QCOMPARE(JsonValue(JsonObject()), JsonValue(JsonObject()));
    JsonObject nonEmptyObject;
    nonEmptyObject.insert("Key", true);
    QVERIFY(JsonValue(JsonObject()) != nonEmptyObject);
    QVERIFY(JsonValue(JsonObject()) != JsonValue(JsonValue::Undefined));
    QVERIFY(JsonValue(JsonObject()) != JsonValue());
    QVERIFY(JsonValue(JsonObject()) != JsonValue(true));
    QVERIFY(JsonValue(JsonObject()) != JsonValue(1.));
    QVERIFY(JsonValue(JsonObject()) != JsonValue(JsonArray()));

    QCOMPARE(JsonValue("foo"), JsonValue("foo"));
    QCOMPARE(JsonValue("\x66\x6f\x6f"), JsonValue("foo"));
    QCOMPARE(JsonValue("\x62\x61\x72"), JsonValue("bar"));
    QCOMPARE(JsonValue(UNICODE_NON_CHARACTER), JsonValue(UNICODE_NON_CHARACTER));
    QCOMPARE(JsonValue(UNICODE_DJE), JsonValue(UNICODE_DJE));
    QCOMPARE(JsonValue("\xc3\xa9"), JsonValue("\xc3\xa9"));
}

void tst_Json::objectEquals_data()
{
    QTest::addColumn<JsonObject>("left");
    QTest::addColumn<JsonObject>("right");
    QTest::addColumn<bool>("result");

    QTest::newRow("two defaults") << JsonObject() << JsonObject() << true;

    JsonObject object1;
    object1.insert("property", 1);
    JsonObject object2;
    object2["property"] = 1;
    JsonObject object3;
    object3.insert("property1", 1);
    object3.insert("property2", 2);

    QTest::newRow("the same object (1 vs 2)") << object1 << object2 << true;
    QTest::newRow("the same object (3 vs 3)") << object3 << object3 << true;
    QTest::newRow("different objects (2 vs 3)") << object2 << object3 << false;
    QTest::newRow("object vs default") << object1 << JsonObject() << false;

    JsonObject empty;
    empty.insert("property", 1);
    empty.take("property");
    QTest::newRow("default vs empty") << JsonObject() << empty << true;
    QTest::newRow("empty vs empty") << empty << empty << true;
    QTest::newRow("object vs empty") << object1 << empty << false;

    JsonObject referencedEmpty;
    referencedEmpty["undefined"];
    QTest::newRow("referenced empty vs referenced empty") << referencedEmpty << referencedEmpty << true;
    QTest::newRow("referenced empty vs object") << referencedEmpty << object1 << false;

    JsonObject referencedObject1;
    referencedObject1.insert("property", 1);
    referencedObject1["undefined"];
    JsonObject referencedObject2;
    referencedObject2.insert("property", 1);
    referencedObject2["aaaaaaaaa"]; // earlier then "property"
    referencedObject2["zzzzzzzzz"]; // after "property"
    QTest::newRow("referenced object vs default") << referencedObject1 << JsonObject() << false;
    QTest::newRow("referenced object vs referenced object") << referencedObject1 << referencedObject1 << true;
    QTest::newRow("referenced object vs object (different)") << referencedObject1 << object3 << false;
}

void tst_Json::objectEquals()
{
    QFETCH(JsonObject, left);
    QFETCH(JsonObject, right);
    QFETCH(bool, result);

    QCOMPARE(left == right, result);
    QCOMPARE(right == left, result);

    // invariants checks
    QCOMPARE(left, left);
    QCOMPARE(right, right);
    QCOMPARE(left != right, !result);
    QCOMPARE(right != left, !result);

    // The same but from JsonValue perspective
    QCOMPARE(JsonValue(left) == JsonValue(right), result);
    QCOMPARE(JsonValue(left) != JsonValue(right), !result);
    QCOMPARE(JsonValue(right) == JsonValue(left), result);
    QCOMPARE(JsonValue(right) != JsonValue(left), !result);
}

void tst_Json::arrayEquals_data()
{
    QTest::addColumn<JsonArray>("left");
    QTest::addColumn<JsonArray>("right");
    QTest::addColumn<bool>("result");

    QTest::newRow("two defaults") << JsonArray() << JsonArray() << true;

    JsonArray array1;
    array1.append(1);
    JsonArray array2;
    array2.append(2111);
    array2[0] = 1;
    JsonArray array3;
    array3.insert(0, 1);
    array3.insert(1, 2);

    QTest::newRow("the same array (1 vs 2)") << array1 << array2 << true;
    QTest::newRow("the same array (3 vs 3)") << array3 << array3 << true;
    QTest::newRow("different arrays (2 vs 3)") << array2 << array3 << false;
    QTest::newRow("array vs default") << array1 << JsonArray() << false;

    JsonArray empty;
    empty.append(1);
    empty.takeAt(0);
    QTest::newRow("default vs empty") << JsonArray() << empty << true;
    QTest::newRow("empty vs default") << empty << JsonArray() << true;
    QTest::newRow("empty vs empty") << empty << empty << true;
    QTest::newRow("array vs empty") << array1 << empty << false;
}

void tst_Json::arrayEquals()
{
    QFETCH(JsonArray, left);
    QFETCH(JsonArray, right);
    QFETCH(bool, result);

    QCOMPARE(left == right, result);
    QCOMPARE(right == left, result);

    // invariants checks
    QCOMPARE(left, left);
    QCOMPARE(right, right);
    QCOMPARE(left != right, !result);
    QCOMPARE(right != left, !result);

    // The same but from JsonValue perspective
    QCOMPARE(JsonValue(left) == JsonValue(right), result);
    QCOMPARE(JsonValue(left) != JsonValue(right), !result);
    QCOMPARE(JsonValue(right) == JsonValue(left), result);
    QCOMPARE(JsonValue(right) != JsonValue(left), !result);
}

void tst_Json::bom()
{
    QFile file(testDataDir + QLatin1String("/bom.json"));
    file.open(QFile::ReadOnly);
    std::string json = file.readAll().data();

    // Import json document into a JsonDocument
    JsonParseError error;
    JsonDocument doc = JsonDocument::fromJson(json, &error);

    QVERIFY(!doc.isNull());
    QCOMPARE(error.error, JsonParseError::NoError);
}

void tst_Json::nesting()
{
    // check that we abort parsing too deeply nested json documents.
    // this is to make sure we don't crash because the parser exhausts the
    // stack.

    const char *array_data =
            "[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[["
            "[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[["
            "[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[["
            "[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[["
            "[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[["
            "[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[["
            "[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[["
            "[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[["
            "]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]"
            "]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]"
            "]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]"
            "]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]"
            "]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]"
            "]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]"
            "]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]"
            "]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]";

    std::string json(array_data);
    JsonParseError error;
    JsonDocument doc = JsonDocument::fromJson(json, &error);

    QVERIFY(!doc.isNull());
    QCOMPARE(error.error, JsonParseError::NoError);

    json = '[' + json + ']';
    doc = JsonDocument::fromJson(json, &error);

    QVERIFY(doc.isNull());
    QCOMPARE(error.error, JsonParseError::DeepNesting);

    json = std::string("true ");

    for (int i = 0; i < 1024; ++i)
        json = "{ \"Key\": " + json + " }";

    doc = JsonDocument::fromJson(json, &error);

    QVERIFY(!doc.isNull());
    QCOMPARE(error.error, JsonParseError::NoError);

    json = '[' + json + ']';
    doc = JsonDocument::fromJson(json, &error);

    QVERIFY(doc.isNull());
    QCOMPARE(error.error, JsonParseError::DeepNesting);

}

void tst_Json::longStrings()
{
#if 0
    // test around 15 and 16 bit boundaries, as these are limits
    // in the data structures (for Latin1String in qjson_p.h)
    QString s(0x7ff0, 'a');
    for (int i = 0x7ff0; i < 0x8010; i++) {
        s.append(QLatin1Char('c'));

        QMap <QString, QVariant> map;
        map["key"] = s;

        /* Create a JsonDocument from the QMap ... */
        JsonDocument d1 = JsonDocument::fromVariant(QVariant(map));
        /* ... and a std::string from the JsonDocument */
        std::string a1 = d1.toJson();

        /* Create a JsonDocument from the std::string ... */
        JsonDocument d2 = JsonDocument::fromJson(a1);
        /* ... and a std::string from the JsonDocument */
        std::string a2 = d2.toJson();
        QCOMPARE(a1, a2);
    }

    s = QString(0xfff0, 'a');
    for (int i = 0xfff0; i < 0x10010; i++) {
        s.append(QLatin1Char('c'));

        QMap <QString, QVariant> map;
        map["key"] = s;

        /* Create a JsonDocument from the QMap ... */
        JsonDocument d1 = JsonDocument::fromVariant(QVariant(map));
        /* ... and a std::string from the JsonDocument */
        std::string a1 = d1.toJson();

        /* Create a JsonDocument from the std::string ... */
        JsonDocument d2 = JsonDocument::fromJson(a1);
        /* ... and a std::string from the JsonDocument */
        std::string a2 = d2.toJson();
        QCOMPARE(a1, a2);
    }
#endif
}

void tst_Json::testJsonValueRefDefault()
{
    JsonObject empty;

    QCOMPARE(empty["n/a"].toString(), std::string());
    QCOMPARE(empty["n/a"].toString("default"), std::string("default"));

    QCOMPARE(empty["n/a"].toBool(), false);
    QCOMPARE(empty["n/a"].toBool(true), true);

    QCOMPARE(empty["n/a"].toInt(), 0);
    QCOMPARE(empty["n/a"].toInt(42), 42);

    QCOMPARE(empty["n/a"].toDouble(), 0.0);
    QCOMPARE(empty["n/a"].toDouble(42.0), 42.0);
}

void tst_Json::arrayInitializerList()
{
#ifndef Q_COMPILER_INITIALIZER_LISTS
    QSKIP("initializer_list is enabled only with c++11 support");
#else
    QVERIFY(JsonArray{}.isEmpty());
    QCOMPARE(JsonArray{"one"}.count(), 1);
    QCOMPARE(JsonArray{1}.count(), 1);

    {
        JsonArray a{1.3, "hello", 0};
        QCOMPARE(JsonValue(a[0]), JsonValue(1.3));
        QCOMPARE(JsonValue(a[1]), JsonValue("hello"));
        QCOMPARE(JsonValue(a[2]), JsonValue(0));
        QCOMPARE(a.count(), 3);
    }
    {
        JsonObject o;
        o["property"] = 1;
        JsonArray a1{o};
        QCOMPARE(a1.count(), 1);
        QCOMPARE(a1[0].toObject(), o);

        JsonArray a2{o, 23};
        QCOMPARE(a2.count(), 2);
        QCOMPARE(a2[0].toObject(), o);
        QCOMPARE(JsonValue(a2[1]), JsonValue(23));

        JsonArray a3{a1, o, a2};
        QCOMPARE(JsonValue(a3[0]), JsonValue(a1));
        QCOMPARE(JsonValue(a3[1]), JsonValue(o));
        QCOMPARE(JsonValue(a3[2]), JsonValue(a2));

        JsonArray a4{1, JsonArray{1,2,3}, JsonArray{"hello", 2}, JsonObject{{"one", 1}}};
        QCOMPARE(a4.count(), 4);
        QCOMPARE(JsonValue(a4[0]), JsonValue(1));

        {
            JsonArray a41 = a4[1].toArray();
            JsonArray a42 = a4[2].toArray();
            JsonObject a43 = a4[3].toObject();
            QCOMPARE(a41.count(), 3);
            QCOMPARE(a42.count(), 2);
            QCOMPARE(a43.count(), 1);

            QCOMPARE(JsonValue(a41[2]), JsonValue(3));
            QCOMPARE(JsonValue(a42[1]), JsonValue(2));
            QCOMPARE(JsonValue(a43["one"]), JsonValue(1));
        }
    }
#endif
}

void tst_Json::objectInitializerList()
{
#ifndef Q_COMPILER_INITIALIZER_LISTS
    QSKIP("initializer_list is enabled only with c++11 support");
#else
    QVERIFY(JsonObject{}.isEmpty());

    {   // one property
        JsonObject one {{"one", 1}};
        QCOMPARE(one.count(), 1);
        QVERIFY(one.contains("one"));
        QCOMPARE(JsonValue(one["one"]), JsonValue(1));
    }
    {   // two properties
        JsonObject two {
            {"one", 1},
            {"two", 2}
        };
        QCOMPARE(two.count(), 2);
        QVERIFY(two.contains("one"));
        QVERIFY(two.contains("two"));
        QCOMPARE(JsonValue(two["one"]), JsonValue(1));
        QCOMPARE(JsonValue(two["two"]), JsonValue(2));
    }
    {   // nested object
        JsonObject object{{"nested", JsonObject{{"innerProperty", 2}}}};
        QCOMPARE(object.count(), 1);
        QVERIFY(object.contains("nested"));
        QVERIFY(object["nested"].isObject());

        JsonObject nested = object["nested"].toObject();
        QCOMPARE(JsonValue(nested["innerProperty"]), JsonValue(2));
    }
    {   // nested array
        JsonObject object{{"nested", JsonArray{"innerValue", 2.1, "bum cyk cyk"}}};
        QCOMPARE(object.count(), 1);
        QVERIFY(object.contains("nested"));
        QVERIFY(object["nested"].isArray());

        JsonArray nested = object["nested"].toArray();
        QCOMPARE(nested.count(), 3);
        QCOMPARE(JsonValue(nested[0]), JsonValue("innerValue"));
        QCOMPARE(JsonValue(nested[1]), JsonValue(2.1));
    }
#endif
}

void tst_Json::unicodeKeys()
{
    std::string json = "{"
                      "\"x\\u2090_1\": \"hello_1\","
                      "\"y\\u2090_2\": \"hello_2\","
                      "\"T\\u2090_3\": \"hello_3\","
                      "\"xyz_4\": \"hello_4\","
                      "\"abc_5\": \"hello_5\""
                      "}";

    JsonParseError error;
    JsonDocument doc = JsonDocument::fromJson(json, &error);
    QCOMPARE(error.error, JsonParseError::NoError);
    JsonObject o = doc.object();

    QCOMPARE(o.keys().size(), size_t(5));
    Q_FOREACH (const std::string &k, o.keys()) {
        QByteArray key(k.data());
        std::string suffix = key.mid(key.indexOf('_')).data();
        QCOMPARE(o[key.data()].toString(), "hello" + suffix);
    }
}

void tst_Json::garbageAtEnd()
{
    JsonParseError error;
    JsonDocument doc = JsonDocument::fromJson("{},", &error);
    QCOMPARE(error.error, JsonParseError::GarbageAtEnd);
    QCOMPARE(error.offset, 2);
    QVERIFY(doc.isEmpty());

    doc = JsonDocument::fromJson("{}    ", &error);
    QCOMPARE(error.error, JsonParseError::NoError);
    QVERIFY(!doc.isEmpty());
}

void tst_Json::removeNonLatinKey()
{
    const std::string nonLatinKeyName = "Атрибут100500";

    JsonObject sourceObject;

    sourceObject.insert("code", 1);
    sourceObject.remove("code");

    sourceObject.insert(nonLatinKeyName, 1);

    const std::string json = JsonDocument(sourceObject).toJson();
    const JsonObject restoredObject = JsonDocument::fromJson(json).object();

    QCOMPARE(sourceObject.keys(), restoredObject.keys());
    QVERIFY(sourceObject.contains(nonLatinKeyName));
    QVERIFY(restoredObject.contains(nonLatinKeyName));
}

QTEST_MAIN(tst_Json)

#include "tst_json.moc"
