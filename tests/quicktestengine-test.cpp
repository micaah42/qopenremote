#include <QtTest/QTest>
#include <QQmlApplicationEngine>
#include <QQuickItem>
#include <QQuickWindow>
#include <QUrl>

#include "quicktestengine.h"

class QuickTestEngineTest : public QObject
{
    Q_OBJECT

private slots:
    void matchingFiltersBasic()
    {
        QObject object;
        object.setObjectName("target");

        QVERIFY(QuickTestEngine::isMatching(&object, {.objectName = "target"}));
        QVERIFY(!QuickTestEngine::isMatching(&object, {.objectName = "schmarget"}));
        QVERIFY(QuickTestEngine::isMatching(&object, {.typeName = "QObject"}));
        QVERIFY(!QuickTestEngine::isMatching(&object, {.typeName = "SchmObject"}));
    }

    void matchingFiltersCombined()
    {
        QObject object;
        object.setObjectName("target");

        QVERIFY(QuickTestEngine::isMatching(&object, {.typeName = "QObject", .objectName = "target"}));
        QVERIFY(!QuickTestEngine::isMatching(&object, {.typeName = "QObject", .objectName = "schmarget"}));
        QVERIFY(!QuickTestEngine::isMatching(&object, {.typeName = "SchmObject", .objectName = "target"}));
    }

    void findRecursivelyFindsItem()
    {
        QQuickWindow window;
        QQuickItem parent{window.contentItem()};
        QQuickItem target{&parent};
        target.setObjectName("target");
        window.show();

        QuickTestEngine::PathPart windowPart;
        windowPart.typeName = "QQuickWindow";
        QuickTestEngine::PathPart itemPart;
        itemPart.objectName = "target";

        QuickTestEngine engine;
        const auto result = engine.find({windowPart, itemPart});

        QCOMPARE(result.value<QObject *>(), static_cast<QObject *>(&target));
    }

    void findReturnsPropertyValue()
    {
        QQuickWindow window;
        QQuickItem target{window.contentItem()};
        target.setObjectName("target");
        target.setProperty("answer", 42);
        window.show();

        QList<QuickTestEngine::PathPart> path{
            {.typeName = "QQuickWindow"},
            {.objectName = "target"},
            {.propertyName = "answer"},
        };

        QuickTestEngine engine;
        QCOMPARE(engine.find(path).toInt(), 42);
    }

    void findRejectsInvalidPath()
    {
        QuickTestEngine engine;
        QVERIFY(!engine.find({}).isValid());
    }

    void findWorksWithQmlLoadedWindow()
    {
        QQmlApplicationEngine engine;
        engine.load(QUrl::fromLocalFile(QOPENREMOTE_TESTS_DIR "/quicktestengine-test.qml"));
        QVERIFY(!engine.rootObjects().isEmpty());

        auto window = qobject_cast<QQuickWindow *>(engine.rootObjects().first());
        QVERIFY(window);
        QVERIFY(QTest::qWaitForWindowExposed(window));

        QList<QuickTestEngine::PathPart> path{
            {.id = "_window"},
            {.id = "_target"},
            {.propertyName = "answer"},
        };

        QuickTestEngine testEngine;
        QCOMPARE(testEngine.find(path).toInt(), 42);
    }

    void findWorksWithDeeplyNestedQmlComponents()
    {
        QQmlApplicationEngine engine;
        engine.load(QUrl::fromLocalFile(QOPENREMOTE_TESTS_DIR "/quicktestengine-test.qml"));
        QVERIFY(!engine.rootObjects().isEmpty());

        auto window = qobject_cast<QQuickWindow *>(engine.rootObjects().first());
        QVERIFY(window);
        QVERIFY(QTest::qWaitForWindowExposed(window));
        QTest::qWait(50); // let the ListView populate its delegates

        QuickTestEngine testEngine;

        // Repeater item, nested one level deeper
        QCOMPARE(testEngine
                     .find({
                         {.id = "_window"},
                         {.objectName = "nestedInRepeaterItem1"},
                         {.propertyName = "label"},
                     })
                     .toString(),
                 "nested 1");

        // ListView delegate, selected by objectName
        QCOMPARE(testEngine
                     .find({
                         {.id = "_window"},
                         {.objectName = "listViewDelegate2"},
                         {.propertyName = "value"},
                     })
                     .toString(),
                 "third");

        // Container's contentItem, nested two levels deep
        QCOMPARE(testEngine
                     .find({
                         {.id = "_window"},
                         {.objectName = "deeplyNestedItem"},
                         {.propertyName = "depth"},
                     })
                     .toInt(),
                 3);
    }
};

#include "quicktestengine-test.moc"

QTEST_MAIN(QuickTestEngineTest)
