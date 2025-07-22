#include <QtTest/QTest>

#include "json.h"

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
};

#include "json-test.moc"

QTEST_MAIN(JSONTest)
