#include <QFuture>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPromise>
#include <QSignalSpy>
#include <QtTest/QTest>

#include <jsonadapter.h>

#include <stdexcept>
/*
            watcher->deleteLater();

            QJsonValue result;

            try {
                if (watcher->future().resultCount())
                    result = JSON::serialize(watcher->result());
            } catch (const std::exception &error) {
result = QString::fromUtf8(error.what());
            }

            QString resultKey = watcher->future().isCanceled() ? "error" : "value";
            QString typeValue = watcher->future().isCanceled() ? "error" : "return";

            QJsonObject object{{"type", typeValue}, {"key", key}, {resultKey, result}};
            emit sendMessage(QJsonDocument{object}.toJson()); 
*/
class AsyncObject : public QObject
{
    Q_OBJECT

public:
    Q_INVOKABLE QFuture<QVariant> succeed()
    {
        qDebug() << "running" << __PRETTY_FUNCTION__;
        QPromise<QVariant> promise;
        auto future = promise.future();
        promise.addResult("done");
        promise.finish();
        return future;
    }

    Q_INVOKABLE QFuture<QVariant> cancel()
    {
        qDebug() << "running" << __PRETTY_FUNCTION__;
        QPromise<QVariant> promise;
        auto future = promise.future();
        future.cancel();
        promise.finish();
        return future;
    }

    Q_INVOKABLE QFuture<QVariant> fail()
    {
        qDebug() << "running" << __PRETTY_FUNCTION__;
        QPromise<QVariant> promise;
        auto future = promise.future();
        promise.setException(std::make_exception_ptr(std::runtime_error{"2112"}));
        promise.finish();
        return future;
    }
};

class JSONAdapterTest : public QObject
{
    Q_OBJECT

private slots:
    void testAsyncCalls()
    {
        ObjectRegistry2 registry;
        JSONAdapter adapter{registry};

        AsyncObject object;
        registry.registerValue("async", &object);
        QSignalSpy messages{&adapter, &JSONAdapter::sendMessage};

        QJsonObject responseObject;

        // test successful call

        adapter.handleMessage(QJsonDocument{{{"type", "call"}, {"key", "async.succeed"}, {"args", QJsonArray{}}}}.toJson());

        QTRY_COMPARE(messages.count(), 1);
        responseObject = QJsonDocument::fromJson(messages.takeFirst().at(0).toByteArray()).object();
        QCOMPARE(responseObject["type"].toString(), "return");
        QCOMPARE(responseObject["key"].toString(), "async.succeed");
        QCOMPARE(responseObject["value"], QJsonValue{"done"});

        // test cancelled call

        adapter.handleMessage(QJsonDocument{{{"type", "call"}, {"key", "async.cancel"}, {"args", QJsonArray{}}}}.toJson());

        QTRY_COMPARE(messages.count(), 1);
        responseObject = QJsonDocument::fromJson(messages.takeFirst().at(0).toByteArray()).object();
        QCOMPARE(responseObject["type"].toString(), "error");
        QCOMPARE(responseObject["key"].toString(), "async.cancel");
        QCOMPARE(responseObject["error"], QJsonValue{QJsonValue::Null});

        // test failed call

        adapter.handleMessage(QJsonDocument{{{"type", "call"}, {"key", "async.fail"}, {"args", QJsonArray{}}}}.toJson());

        QTRY_COMPARE(messages.count(), 1);
        responseObject = QJsonDocument::fromJson(messages.takeFirst().at(0).toByteArray()).object();
        QCOMPARE(responseObject["type"].toString(), "error");
        QCOMPARE(responseObject["key"].toString(), "async.fail");
        QCOMPARE(responseObject["error"], QJsonValue{"2112"});
    }
};

#include "jsonadapter-test.moc"

QTEST_MAIN(JSONAdapterTest)