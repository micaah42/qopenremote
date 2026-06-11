#include <QJsonArray>
#include <QJsonObject>
#include <QtTest/QTest>

#include "json.h"

class A : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int integer READ integer WRITE setInteger NOTIFY integerChanged FINAL)
    Q_PROPERTY(QString string READ string WRITE setString NOTIFY stringChanged FINAL)
    Q_PROPERTY(QList<int> numbers READ numbers WRITE setNumbers NOTIFY numbersChanged FINAL)

public:
    Q_INVOKABLE explicit A(QObject *parent = nullptr)
        : QObject{parent}
        , m_integer{0}
        , m_string{}
        , m_numbers{}
    {}

    int integer() const { return m_integer; }
    void setInteger(int newInteger)
    {
        if (m_integer == newInteger)
            return;
        m_integer = newInteger;
        emit integerChanged();
    }

    QString string() const { return m_string; }
    void setString(const QString &newString)
    {
        if (m_string == newString)
            return;
        m_string = newString;
        emit stringChanged();
    }

    QList<int> numbers() const { return m_numbers; }
    void setNumbers(const QList<int> &newNumbers)
    {
        if (m_numbers == newNumbers)
            return;
        m_numbers = newNumbers;
        emit numbersChanged();
    }

signals:
    void integerChanged();
    void stringChanged();
    void numbersChanged();

private:
    int m_integer;
    QString m_string;
    QList<int> m_numbers;
};

class B : public QObject
{
    Q_OBJECT
    Q_PROPERTY(A *a READ a WRITE setA NOTIFY aChanged FINAL)
    Q_PROPERTY(QList<A *> as READ as WRITE setAs NOTIFY asChanged FINAL)

public:
    Q_INVOKABLE explicit B(QObject *parent = nullptr)
        : QObject{parent}
        , m_a{nullptr}
    {}

    A *a() const { return m_a; }
    void setA(A *newA)
    {
        if (m_a == newA)
            return;
        m_a = newA;
        emit aChanged();
    }

    const QList<A *> &as() const { return m_as; }
    void setAs(const QList<A *> &newA)
    {
        if (m_as == newA)
            return;
        m_as = newA;
        emit asChanged();
    }

signals:
    void asChanged();
    void aChanged();

private:
    A *m_a = nullptr;
    QList<A *> m_as;
};

class JSONTest : public QObject
{
    Q_OBJECT

private slots:
    void testSpecialTypes()
    {
        // Valid QDateTime

        {
            QDateTime dt{{1, 1, 1999}, {21, 12}};
            auto serialized = JSON::serialize(dt);
            auto recovered = JSON::deserialize<QDateTime>(serialized);
            QCOMPARE(recovered, dt);
        }

        // Invalid QDateTime

        {
            QDateTime dt;
            auto serialized = JSON::serialize(dt);
            auto recovered = JSON::deserialize<QDateTime>(serialized);
            QCOMPARE(recovered, dt);
        }

        // Valid QTime

        {
            QTime time{21, 12};
            auto serialized = JSON::serialize(time);
            auto recovered = JSON::deserialize<QTime>(serialized);
            QCOMPARE(recovered, time);
        }

        // Invalid QTime

        {
            QTime time;
            auto serialized = JSON::serialize(time);
            auto recovered = JSON::deserialize<QTime>(serialized);
            QCOMPARE(recovered, time);
        }
    }

    void testSimpleQObject()
    {
        A obj;
        obj.setInteger(42);
        obj.setString("Hello World");
        obj.setNumbers({1, 2, 3, 4, 5});

        QJsonObject serialized = JSON::serialize(&obj).toObject();
        
        // Verify individual fields
        QCOMPARE(serialized["integer"].toInt(), 42);
        QCOMPARE(serialized["string"].toString(), QString("Hello World"));
        QCOMPARE(serialized["numbers"].toArray(), QJsonArray({1, 2, 3, 4, 5}));

        // Test Deserialization
        auto *recovered = JSON::deserialize<A*>(serialized);
        QVERIFY(recovered != nullptr);
        QCOMPARE(recovered->integer(), obj.integer());
        QCOMPARE(recovered->string(), obj.string());
        QCOMPARE(recovered->numbers(), obj.numbers());
        
        recovered->deleteLater();
    }

    void testNestedQObject()
    {
        B root;
        A *child = new A();
        child->setInteger(100);
        child->setString("Nested Object");
        
        root.setA(child);

        QJsonObject serialized = JSON::serialize(&root).toObject();
        
        
        QVERIFY(serialized.contains("a"));
        QJsonObject childJson = serialized["a"].toObject();
        QCOMPARE(childJson["integer"].toInt(), 100);

        
        auto *recoveredB = JSON::deserialize<B*>(serialized);
        QVERIFY(recoveredB->a() != nullptr);
        QCOMPARE(recoveredB->a()->integer(), 100);
        QCOMPARE(recoveredB->a()->string(), QString("Nested Object"));

        // Cleanup
        delete recoveredB->a();
        delete recoveredB;
        delete child;
    }

    void testQObjectList()
    {
        B root;
        A *a1 = new A();
        a1->setInteger(1);

        A *a2 = new A();
        a2->setInteger(2);

        root.setAs({a1, a2});

        QJsonObject serialized = JSON::serialize(&root).toObject();
        qInfo() << "serialized:" << serialized;
        QJsonArray array = serialized["as"].toArray();
        
        QCOMPARE(array.size(), 2);
        QCOMPARE(array.at(0).toObject()["integer"].toInt(), 1);
        QCOMPARE(array.at(1).toObject()["integer"].toInt(), 2);

        // Deserialization check
        auto *recoveredB = JSON::deserialize<B*>(serialized);
        QCOMPARE(recoveredB->as().size(), 2);
        QCOMPARE(recoveredB->as().at(0)->integer(), 1);
        
        // Cleanup logic (assuming the list owns the pointers or you manual clean)
        qDeleteAll(recoveredB->as());
        delete recoveredB;
        delete a1;
        delete a2;
    }

    void testNullPointers()
    {
        B root;
        root.setA(nullptr); // Ensure m_a is null

        QJsonObject serialized = JSON::serialize(&root).toObject();
        
        // Depending on your library implementation, 
        // null pointers should likely be QJsonValue::Null
        QVERIFY(serialized["a"].isNull());

        auto *recovered = JSON::deserialize<B*>(serialized);
        QVERIFY(recovered->a() == nullptr);
        
        delete recovered;
    }
};

#include "json-test.moc"

QTEST_MAIN(JSONTest)
