#include <QtTest/QTest>

#include "qobjectregistry.h"

class A : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int integer READ integer WRITE setInteger NOTIFY integerChanged FINAL)
    Q_PROPERTY(QString string READ string WRITE setString NOTIFY stringChanged FINAL)
    Q_PROPERTY(QList<int> numbers READ numbers WRITE setNumbers NOTIFY numbersChanged FINAL)

public:
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

public slots:
    QString ping() { return "pong"; };

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

class QObjectRegistryTest : public QObject
{
    Q_OBJECT

private slots:
    void simpleRead()
    {
        QObjectRegistry registry{};

        A a{};
        a.setInteger(2112);
        a.setString("rocks!");
        registry.registerObject("a", &a);

        QCOMPARE(registry.get("a.integer").toInt(), 2112);
        QCOMPARE(registry.get("a.string").toString(), "rocks!");
    }

    void recursiveRead()
    {
        QObjectRegistry registry{};

        A a{};
        a.setInteger(2112);
        a.setString("rocks!");

        B b{};
        b.setA(&a);
        registry.registerObject("b", &b);

        QCOMPARE(registry.get("b.a.integer").toInt(), 2112);
        QCOMPARE(registry.get("b.a.string").toString(), "rocks!");

        b.setA(nullptr);
        QCOMPARE(registry.get("b.a"), QVariant());
        QCOMPARE(registry.get("b.a.string"), QVariant());
    }

    void simpleListRead()
    {
        QObjectRegistry registry{};

        A a{};
        registry.registerObject("a", &a);
        QCOMPARE(registry.get("a").toList(), {});

        a.setNumbers({2, 1, 1, 2});
        QVariantList expected{2, 1, 1, 2};
        QCOMPARE(registry.get("a.numbers").toList(), expected);
    }

    void objectListRead()
    {
        QObjectRegistry registry{};

        A a1{};
        a1.setString("i am 1");
        a1.setInteger(1);

        A a2{};
        a2.setString("i am 2");
        a2.setInteger(2);

        A a3{};
        a3.setString("i am 3");
        a3.setInteger(3);

        B b{};
        b.setAs({&a1, &a2, &a3});
        registry.registerObject("b", &b);

        QCOMPARE(registry.get("b.as.0.string"), "i am 1");
        QCOMPARE(registry.get("b.as.0.integer"), 1);
        QCOMPARE(registry.get("b.as.1.string"), "i am 2");
        QCOMPARE(registry.get("b.as.1.integer"), 2);
        QCOMPARE(registry.get("b.as.2.string"), "i am 3");
        QCOMPARE(registry.get("b.as.2.integer"), 3);

        b.setAs({&a3, &a1, &a2});

        QCOMPARE(registry.get("b.as.0.string"), "i am 3");
        QCOMPARE(registry.get("b.as.0.integer"), 3);
        QCOMPARE(registry.get("b.as.1.string"), "i am 1");
        QCOMPARE(registry.get("b.as.1.integer"), 1);
        QCOMPARE(registry.get("b.as.2.string"), "i am 2");
        QCOMPARE(registry.get("b.as.2.integer"), 2);
    }
};

#include "qobjectregistry-test.moc"

QTEST_MAIN(QObjectRegistryTest)
