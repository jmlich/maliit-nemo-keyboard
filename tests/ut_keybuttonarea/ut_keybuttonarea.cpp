/* * This file is part of m-keyboard *
 *
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
 * All rights reserved.
 * Contact: Nokia Corporation (directui@nokia.com)
 *
 * If you have questions regarding the use of this file, please contact
 * Nokia at directui@nokia.com.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation
 * and appearing in the file LICENSE.LGPL included in the packaging
 * of this file.
 */

#include "ut_keybuttonarea.h"
#include "mvirtualkeyboardstyle.h"
#include "singlewidgetbuttonarea.h"
#include "singlewidgetbutton.h"
#include "flickupbutton.h"
#include "flickgesturerecognizer.h"
#include "../ut_flickrecognizer/flickutil.h"
#include "keyboarddata.h"
#include "vkbdatakey.h"
#include "mplainwindow.h"
#include "popupbase.h"

#include <MApplication>
#include <MScene>
#include <MSceneManager>
#include <MTheme>

#include <QDir>
#include <QGraphicsLayout>
#include <QGraphicsSceneMouseEvent>
#include <QTouchEvent>

#include <algorithm>

namespace
{
    const int LongPressTime = 0; // same as in keybuttonarea.cpp
}

Q_DECLARE_METATYPE(KeyEvent);
Q_DECLARE_METATYPE(IKeyButton*);
Q_DECLARE_METATYPE(const IKeyButton*);

typedef KeyButtonArea *(*KBACreator)(MVirtualKeyboardStyleContainer *styleContainer,
                                     QSharedPointer<const LayoutSection> section,
                                     KeyButtonArea::ButtonSizeScheme buttonSizeScheme,
                                     bool usePopup,
                                     QGraphicsWidget *parent);

Q_DECLARE_METATYPE(KBACreator);
Q_DECLARE_METATYPE(IKeyButton::ButtonState);
Q_DECLARE_METATYPE(QList<KeyBinding::KeyAction>);

KeyButtonArea *createSingleWidgetKeyButtonArea(MVirtualKeyboardStyleContainer *styleContainer,
                                               QSharedPointer<const LayoutSection> section,
                                               KeyButtonArea::ButtonSizeScheme buttonSizeScheme = KeyButtonArea::ButtonSizeEqualExpanding,
                                               bool usePopup = false,
                                               QGraphicsWidget *parent = 0)
{
    return new SingleWidgetButtonArea(styleContainer, section, buttonSizeScheme, usePopup, parent);
}


void Ut_KeyButtonArea::initTestCase()
{
    static int argc = 1;
    static char *app_name[1] = { (char *) "ut_keybuttonarea" };

    // Avoid waiting if im server is not responding
    MApplication::setLoadMInputContext(false);
    app = new MApplication(argc, app_name);

    style = new MVirtualKeyboardStyleContainer;
    style->initialize("MVirtualKeyboard", "MVirtualKeyboardView", 0);

    qRegisterMetaType<KeyEvent>();
    qRegisterMetaType<const IKeyButton*>();
    qRegisterMetaType<IKeyButton*>();
    qRegisterMetaType<IKeyButton::ButtonState>();

    new MPlainWindow; // Create singleton

    FlickGestureRecognizer::registerSharedRecognizer();

    MPlainWindow::instance()->grabGesture(FlickGestureRecognizer::sharedGestureType());
    MPlainWindow::instance()->show();
    QTest::qWaitForWindowShown(MPlainWindow::instance());
}

void Ut_KeyButtonArea::cleanupTestCase()
{
    FlickGestureRecognizer::unregisterSharedRecognizer();
    delete MPlainWindow::instance();
    delete style;
    style = 0;
    delete app;
    app = 0;
}

void Ut_KeyButtonArea::init()
{
    subject = 0;
    keyboard = 0;
}

void Ut_KeyButtonArea::cleanup()
{
    delete subject;
    delete keyboard;
    keyboard = 0;
}

void Ut_KeyButtonArea::testLandscapeBoxSize_data()
{
    QTest::addColumn<KBACreator>("createKba");
    QTest::newRow("SingleWidgetArea") << &createSingleWidgetKeyButtonArea;
}

void Ut_KeyButtonArea::testLandscapeBoxSize()
{
    QFETCH(KBACreator, createKba);

    QSize box;
    QDir dir("/usr/share/meegotouch/virtual-keyboard/layouts/");
    QStringList filters;
    int fileCount = 0;

    changeOrientation(M::Angle0);

    filters << "??.xml";
    foreach(QFileInfo info, dir.entryInfoList(filters)) {
        delete subject;
        subject = 0;
        delete keyboard;
        keyboard = new KeyboardData;
        qDebug() << "Loading layout file" << info.fileName();
        QVERIFY(keyboard->loadNokiaKeyboard(info.fileName()));
        subject = createKba(style,
                            keyboard->layout(LayoutData::General, M::Landscape)->section(LayoutData::mainSection),
                            KeyButtonArea::ButtonSizeEqualExpanding,
                            false, 0);
        MPlainWindow::instance()->scene()->addItem(subject);

        subject->resize(defaultLayoutSize());

        for (int level = 0; level < 2; level++) {
            subject->switchLevel(level);

            box = keyAt(0, 0)->buttonRect().size();
            qDebug() << "Current level" << level << "; Box size=" << box;
            QVERIFY(box.height() >= 55 && box.height() <= 80);
            QVERIFY(box.width() >= 70 && box.width() <= 86);
        }
        ++fileCount;
    }
    QVERIFY(fileCount > 0);
}

void Ut_KeyButtonArea::testPortraitBoxSize_data()
{
    QTest::addColumn<KBACreator>("createKba");
    QTest::newRow("SingleWidgetArea") << &createSingleWidgetKeyButtonArea;
}

void Ut_KeyButtonArea::testPortraitBoxSize()
{
    QFETCH(KBACreator, createKba);

    QSize box;
    QDir dir("/usr/share/meegotouch/virtual-keyboard/layouts/");
    QStringList filters;
    int fileCount = 0;

    changeOrientation(M::Angle90);

    filters << "??.xml";
    foreach(QFileInfo info, dir.entryInfoList(filters)) {
        delete subject;
        subject = 0;
        delete keyboard;
        keyboard = new KeyboardData;
        qDebug() << "Loading layout file" << info.fileName();
        QVERIFY(keyboard->loadNokiaKeyboard(info.fileName()));
        subject = createKba(style, keyboard->layout(LayoutData::General, M::Portrait)->section(LayoutData::mainSection),
                            KeyButtonArea::ButtonSizeEqualExpanding,
                            false, 0);
        MPlainWindow::instance()->scene()->addItem(subject);

        subject->resize(defaultLayoutSize());
        for (int level = 0; level < 2; level++) {
            subject->switchLevel(level);

            box = keyAt(0, 0)->buttonRect().size();
            qDebug() << "Current level" << level << "; Box size=" << box << subject->size();
            QVERIFY(box.height() >= 60 && box.height() <= 80);
            QVERIFY(box.width() >= 35 && box.width() <= 50);
        }
        ++fileCount;
    }
    QVERIFY(fileCount > 0);
}

void Ut_KeyButtonArea::testLabelPosition_data()
{
    QTest::addColumn<KBACreator>("createKba");
    QTest::newRow("SingleWidgetArea") << &createSingleWidgetKeyButtonArea;
}

void Ut_KeyButtonArea::testLabelPosition()
{
    QFETCH(KBACreator, createKba);

    QList<QPoint> positions;
    QList<const VKBDataKey *> outcome;
    QSize buttonSize;
    int startX;
    const int rowIndex = 2;
    const IKeyButton *button = 0;

    keyboard = new KeyboardData;
    QVERIFY(keyboard->loadNokiaKeyboard("en_us.xml"));
    subject = createKba(style, keyboard->layout(LayoutData::General, M::Landscape)->section(LayoutData::mainSection),
                        KeyButtonArea::ButtonSizeEqualExpanding,
                        false, 0);
    MPlainWindow::instance()->scene()->addItem(subject);

    subject->resize(defaultLayoutSize());

    const IKeyButton *firstButton = keyAt(0, 0);

    button = keyAt(2, 0); // Third row's first button
    buttonSize = button->buttonBoundingRect().size();
    startX = button->buttonBoundingRect().x();
    positions << QPoint(-50, -50)                       // not on any key
              << firstButton->buttonRect().topLeft() + QPoint(-1, -1) // reactive margin should get this
              << QPoint(-buttonSize.width(), 0)         // not on any key
              << QPoint(0, -buttonSize.height())        // not on any key
              << QPoint(1, 100 * buttonSize.height())   // not on any key
              << QPoint(startX + buttonSize.width() + (*style)->spacingHorizontal() + 1,
                        (*style)->paddingTop() + (buttonSize.height() + (*style)->spacingVertical()) * rowIndex + 1)
              << QPoint(startX + buttonSize.width() * 100 + 1, buttonSize.height() * rowIndex + 1); // not on any key
    outcome << 0
            << subject->section->getVKBKey(0, 0)
            << 0
            << 0
            << 0
            << subject->section->getVKBKey(rowIndex, 1)
            << 0;
    QVERIFY(positions.count() == outcome.count());
    for (int n = 0; n < positions.count(); ++n) {
        qDebug() << "test position" << positions.at(n);
        button = subject->keyAt(positions.at(n));
        const VKBDataKey *result = (button ? &button->key() : 0);
        QCOMPARE(result, outcome.at(n));
    }
}

void Ut_KeyButtonArea::testFlickCheck_data()
{
    QTest::addColumn<bool>("directMode");
    QTest::newRow("direct mode off") << false;
    QTest::newRow("direct mode on") << true;
}

void Ut_KeyButtonArea::testFlickCheck()
{
    QFETCH(bool, directMode);

    keyboard = new KeyboardData;
    QVERIFY(keyboard->loadNokiaKeyboard("en_us.xml"));
    subject = createSingleWidgetKeyButtonArea(style,
                                              keyboard->layout(LayoutData::General, M::Landscape)->section(LayoutData::mainSection),
                                              KeyButtonArea::ButtonSizeEqualExpanding,
                                              false, 0);
    KeyButtonArea::setInputMethodMode(directMode ? M::InputMethodModeDirect : M::InputMethodModeNormal);
    MPlainWindow::instance()->scene()->addItem(subject);
    subject->resize(defaultLayoutSize());

    subject->setPos(0, 0);

    subject->grabGesture(FlickGestureRecognizer::sharedGestureType());

    QSignalSpy leftSwipeSpy(subject, SIGNAL(flickLeft()));
    QSignalSpy rightSwipeSpy(subject, SIGNAL(flickRight()));
    QSignalSpy upSwipeSpy(subject, SIGNAL(flickUp(const KeyBinding &)));
    QSignalSpy downSwipeSpy(subject, SIGNAL(flickDown()));

    const int numDirs = 5;
    FlickGesture::Direction directions[numDirs] = {
        FlickGesture::Left,
        FlickGesture::Right,
        FlickGesture::Up,
        FlickGesture::Down,
        FlickGesture::NoDirection
    };

    QPoint flickPoints[numDirs][2] = {
        { keyAt(0, 9)->buttonBoundingRect().center(),
          keyAt(0, 0)->buttonBoundingRect().center() },
        { keyAt(0, 0)->buttonBoundingRect().center(),
          keyAt(0, 9)->buttonBoundingRect().center() },
        { keyAt(2, 4)->buttonBoundingRect().center(),
          keyAt(0, 4)->buttonBoundingRect().center() },
        { keyAt(0, 4)->buttonBoundingRect().center(),
          keyAt(2, 4)->buttonBoundingRect().center() },
        { QPoint(), QPoint() }
    };

    for (int i = 0; i < numDirs; ++i) {
        FlickGesture::Direction dir = directions[i];

        subject->grabMouse();
        doMouseSwipe(subject, flickPoints[i][0], flickPoints[i][1], 0, 5);
        subject->ungrabMouse();

        QCOMPARE(leftSwipeSpy.count(),
                 (!directMode && dir == FlickGesture::Left) ? 1 : 0);
        QCOMPARE(rightSwipeSpy.count(),
                 (!directMode && dir == FlickGesture::Right) ? 1 : 0);
        QCOMPARE(upSwipeSpy.count(),
                 (!directMode && dir == FlickGesture::Up) ? 1 : 0);
        QCOMPARE(downSwipeSpy.count(),
                 (!directMode && dir == FlickGesture::Down) ? 1 : 0);

        leftSwipeSpy.clear();
        rightSwipeSpy.clear();
        upSwipeSpy.clear();
        downSwipeSpy.clear();
    }
}

void Ut_KeyButtonArea::testSceneEvent_data()
{
    QTest::addColumn<KBACreator>("createKba");
    QTest::newRow("SingleWidgetArea") << &createSingleWidgetKeyButtonArea;
}

void Ut_KeyButtonArea::testSceneEvent()
{
    QFETCH(KBACreator, createKba);

    //initialization
    keyboard = new KeyboardData;
    QVERIFY(keyboard->loadNokiaKeyboard("en_us.xml"));
    subject = createKba(style, keyboard->layout(LayoutData::General, M::Landscape)->section(LayoutData::mainSection),
                        KeyButtonArea::ButtonSizeEqualExpanding,
                        false, 0);
    MPlainWindow::instance()->scene()->addItem(subject);
    subject->resize(defaultLayoutSize());

    // Skip test for multitouch, since there are no mouse events:
    if (subject->acceptTouchEvents()) {
        return;
    }

    QGraphicsSceneMouseEvent *press = new QGraphicsSceneMouseEvent(QEvent::GraphicsSceneMousePress);
    QGraphicsSceneMouseEvent *release = new QGraphicsSceneMouseEvent(QEvent::GraphicsSceneMouseRelease);
    QGraphicsSceneMouseEvent *move = new QGraphicsSceneMouseEvent(QEvent::GraphicsSceneMouseMove);
    QSignalSpy spy(subject, SIGNAL(keyClicked(const IKeyButton*, const QString&, bool)));
    QSignalSpy spyPressed(subject, SIGNAL(keyPressed(const IKeyButton*, const QString&, bool)));

    QVERIFY(spy.isValid());
    QVERIFY(spyPressed.isValid());

    press->setPos(QPoint(1, 1));
    release->setPos(QPoint(10, 10));
    move->setPos(QPoint(10, 10));

    //actual testing
    subject->sceneEvent(press);
    QCOMPARE(spyPressed.count(), 1);
    QCOMPARE(spy.count(), 0);
    subject->sceneEvent(move);
    QCOMPARE(spy.count(), 0);
    subject->sceneEvent(release);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spyPressed.count(), 1);

    delete press;
    delete move;
    delete release;
}

void Ut_KeyButtonArea::testPaint_data()
{
    QTest::addColumn<KBACreator>("createKba");
    QTest::newRow("SingleWidgetArea") << &createSingleWidgetKeyButtonArea;
}

void Ut_KeyButtonArea::testPaint()
{
    QFETCH(KBACreator, createKba);

    //at least we should not chrash here
    QImage *image = new QImage(QSize(864, 480), QImage::Format_ARGB32_Premultiplied);
    QPainter painter;
    QVERIFY(painter.begin(image));

    //initialization
    keyboard = new KeyboardData;
    QVERIFY(keyboard->loadNokiaKeyboard("en_us.xml"));
    subject = createKba(style, keyboard->layout(LayoutData::General, M::Landscape)->section(LayoutData::mainSection),
                        KeyButtonArea::ButtonSizeEqualExpanding,
                        false, 0);
    subject->resize(defaultLayoutSize());
    MPlainWindow::instance()->scene()->addItem(subject);

    subject->touchPointPressed(QPoint(20, 20), 0); // top left button
    //actual testing
    subject->paint(&painter, 0, 0);
}

void Ut_KeyButtonArea::testDeadkeys_data()
{
    QTest::addColumn<KBACreator>("createKba");
    QTest::newRow("SingleWidgetArea") << &createSingleWidgetKeyButtonArea;
}

void Ut_KeyButtonArea::testDeadkeys()
{
    QFETCH(KBACreator, createKba);

    keyboard = new KeyboardData;
    QVERIFY(keyboard->loadNokiaKeyboard("fr.xml"));
    subject = createKba(style, keyboard->layout(LayoutData::General, M::Landscape)->section(LayoutData::mainSection),
                        KeyButtonArea::ButtonSizeEqualExpanding,
                        false, 0);
    MPlainWindow::instance()->scene()->addItem(subject);
    subject->resize(defaultLayoutSize());
    QSignalSpy spy(subject, SIGNAL(keyClicked(const IKeyButton*, const QString&, bool)));
    IKeyButton *key = 0;
    QList<int> positions;
    int i;
    positions << 0 << 1 << 2 << 5 << 6;

    QVERIFY(spy.isValid());

    //!!! z is the character that won't be changed when deadkey is locked
    QStringList lowerUnicodes;
    lowerUnicodes << QChar('a') << QChar('z') << QChar('e') << QChar('y') << QChar('u');
    QStringList upperUnicodes; //upper case
    foreach(QString c, lowerUnicodes)
    upperUnicodes << c.toUpper();

    //Unicode for alphas with deadkeys:  á << Unicod é  unicode for  ý<<< unicode for  ú
    QStringList lowerDKUnicodes;
    lowerDKUnicodes << QChar(0x00e1) << QChar('z') << QChar(0x00e9) << QChar(0x00fd) << QChar(0x00fa);
    QStringList upperDKUnicodes; // upper case
    foreach(QString c, lowerDKUnicodes)
    upperDKUnicodes << c.toUpper();

    //test for unlock deadkey status
    for (i = 0; i < positions.count(); i++) {
        QCOMPARE(keyAt(0, positions[i])->label(), lowerUnicodes.at(i));
    }

    key = keyAt(2, 7); // row 3, column 8
    QVERIFY(key != 0);
    QVERIFY(key->isDeadKey());
    QString c = QChar(0x00B4);
    QCOMPARE(key->label(), c);
    subject->click(key);
    //click at deadkey for the first time, just lock the deadkey, won't emit cliked() signal
    QCOMPARE(spy.count(), 0);

    //check the alphas, should be with deadkey
    for (i = 0; i < positions.count(); i++) {
        QCOMPARE(keyAt(0, positions[i])->label(), lowerDKUnicodes.at(i));
    }

    //test for shift status
    subject->switchLevel(1);
    for (i = 0; i < positions.count(); i++) {
        QCOMPARE(keyAt(0, positions[i])->label(), upperDKUnicodes.at(i));
    }

    //after unlock the dead key, test the shift status
    subject->unlockDeadkeys();
    for (i = 0; i < positions.count(); i++) {
        QCOMPARE(keyAt(0, positions[i])->label(), upperUnicodes.at(i));
    }

    //test for shift off status
    subject->switchLevel(0);
    for (i = 0; i < positions.count(); i++) {
        QCOMPARE(keyAt(0, positions[i])->label(), lowerUnicodes.at(i));
    }

    // Lock deadkey again.
    subject->click(key);
    for (i = 0; i < positions.count(); i++) {
        QCOMPARE(keyAt(0, positions[i])->label(), lowerDKUnicodes.at(i));
    }

    subject->click(keyAt(0, 0));
    //key release on not deadkey, will emit clicked() signal
    QCOMPARE(spy.count(), 1);
    //any keypress, the deadkey should be unlocked
    for (i = 0; i < positions.count(); i++) {
        QCOMPARE(keyAt(0, positions[i])->label(), lowerUnicodes.at(i));
    }
}

void Ut_KeyButtonArea::testSelectedDeadkeys()
{
    keyboard = new KeyboardData;
    QVERIFY(keyboard->loadNokiaKeyboard("fr.xml"));
    subject = createSingleWidgetKeyButtonArea(style, keyboard->layout(LayoutData::General, M::Landscape)->section(LayoutData::mainSection),
                                              KeyButtonArea::ButtonSizeEqualExpanding,
                                              false, 0);
    MPlainWindow::instance()->scene()->addItem(subject);

    // Pick two deadkeys to play around with.
    IKeyButton *deadkey1 = keyAt(2, 7); // row 3, column 7
    IKeyButton *deadkey2 = keyAt(2, 8); // row 3, column 8
    IKeyButton *regularKey = keyAt(0, 0); // first key, top left

    QVERIFY(deadkey1 && deadkey1->isDeadKey());
    QVERIFY(deadkey2 && deadkey2->isDeadKey());
    QVERIFY(regularKey && !regularKey->isDeadKey());

    QCOMPARE(deadkey1->state(), IKeyButton::Normal);
    QCOMPARE(deadkey2->state(), IKeyButton::Normal);
    QCOMPARE(regularKey->state(), IKeyButton::Normal);

    // Press dead key down
    subject->click(deadkey1);
    QCOMPARE(deadkey1->state(), IKeyButton::Selected);

    // Release it by clicking regular key
    subject->click(regularKey);
    QCOMPARE(deadkey1->state(), IKeyButton::Normal);

    // Down again
    subject->click(deadkey1);
    QCOMPARE(deadkey1->state(), IKeyButton::Selected);

    // Release it by clicking itself again.
    subject->click(deadkey1);
    QCOMPARE(deadkey1->state(), IKeyButton::Normal);

    // Down again
    subject->click(deadkey1);
    QCOMPARE(deadkey1->state(), IKeyButton::Selected);

    // Release it by clicking the other dead key.
    subject->click(deadkey2);
    QCOMPARE(deadkey1->state(), IKeyButton::Normal);
    QCOMPARE(deadkey2->state(), IKeyButton::Selected);
}

void Ut_KeyButtonArea::testImportedLayouts_data()
{
    QTest::addColumn<KBACreator>("createKba");
    QTest::newRow("SingleWidgetArea") << &createSingleWidgetKeyButtonArea;
}

void Ut_KeyButtonArea::testImportedLayouts()
{
    QFETCH(KBACreator, createKba);

    // This test uses files test-importer.xml, test-import1.xml, and test-import2.xml.
    // The first imported file test-import1.xml defines some landscape and portrait stuff, while
    // the second imported file test-import2.xml redefines the portrait stuff.

    keyboard = new KeyboardData;
    QVERIFY(keyboard->loadNokiaKeyboard("test-importer.xml"));
    const LayoutData *model = keyboard->layout(LayoutData::General, M::Landscape);
    QVERIFY(model);
    subject = createKba(style, model->section(LayoutData::mainSection),
                        KeyButtonArea::ButtonSizeEqualExpanding,
                        false, 0);

    // Check that language and title information are not overwritten by imported files.
    QCOMPARE(keyboard->title(), QString("test-importer-title"));
    QCOMPARE(keyboard->language(), QString("test-importer-language"));

    // First imported defines landscape layout with key labeled "1".
    // Second one doesn't define landscape layout.
    changeOrientation(M::Angle0);
    QCOMPARE(keyAt(0, 0)->label(), QString("1"));
    QCOMPARE(model->section(LayoutData::functionkeySection)->getVKBKey(0, 0)->binding(false)->label(),
             QString("func1"));

    // Second imported defines portrait layout with key labeled "2"
    // and also it defines portrait functionkeys which overwrites
    // the first one's functionkeys.
    delete subject;
    model = keyboard->layout(LayoutData::General, M::Portrait);
    subject = createKba(style, model->section(LayoutData::mainSection),
                        KeyButtonArea::ButtonSizeEqualExpanding,
                        false, 0);
    changeOrientation(M::Angle90);
    QCOMPARE(keyAt(0, 0)->label(), QString("2"));
    QCOMPARE(model->section(LayoutData::functionkeySection)->getVKBKey(0, 0)->binding(false)->label(),
             QString("func2"));
}

void Ut_KeyButtonArea::testAccurateMode_data()
{
    QTest::addColumn<KBACreator>("createKba");
    QTest::newRow("SingleWidgetArea") << &createSingleWidgetKeyButtonArea;
}

void Ut_KeyButtonArea::testAccurateMode()
{
    QFETCH(KBACreator, createKba);

    keyboard = new KeyboardData;
    QVERIFY(keyboard->loadNokiaKeyboard("en_us.xml"));
    subject = createKba(style, keyboard->layout(LayoutData::General, M::Landscape)->section(LayoutData::mainSection),
                        KeyButtonArea::ButtonSizeEqualExpanding, true, 0);
    MPlainWindow::instance()->scene()->addItem(subject);
    subject->resize(defaultLayoutSize());

    subject->accurateStop();
    QVERIFY(!subject->isAccurateMode());

    // Test starting accurate mode

    // direct call
    subject->accurateStart();
    QVERIFY(subject->isAccurateMode());
    subject->accurateStop();

    // make long press

    const QPoint mousePos(20, 20); // approximately the top left key on layout
    const int touchId = 0;

    //popup should be shown immediately, so we comment out this part of test
#if 0
    subject->touchPointPressed(mousePos, touchId);
    QTest::qWait(LongPressTime - 100); // not enough time
    subject->touchPointReleased(mousePos, touchId);
    QVERIFY(!subject->isAccurateMode());
#endif

    subject->touchPointPressed(mousePos, touchId);
    QTest::qWait(LongPressTime + 100); // long enough

    // When accurate mode is on and mouse down we should have popup enabled
    QVERIFY(subject->isPopupActive());

    subject->touchPointReleased(mousePos, touchId);
    QVERIFY(subject->isAccurateMode());
}

void Ut_KeyButtonArea::testPopup_data()
{
    QTest::addColumn<KBACreator>("createKba");
    QTest::newRow("SingleWidgetArea") << &createSingleWidgetKeyButtonArea;
}

void Ut_KeyButtonArea::testPopup()
{
    QFETCH(KBACreator, createKba);

    keyboard = new KeyboardData;
    QVERIFY(keyboard->loadNokiaKeyboard("en_us.xml"));
    subject = createKba(style, keyboard->layout(LayoutData::General, M::Landscape)->section(LayoutData::mainSection),
                        KeyButtonArea::ButtonSizeEqualExpanding, true, 0);
    MPlainWindow::instance()->scene()->addItem(subject);
    subject->resize(defaultLayoutSize());

    const QPoint mousePos(20, 20); // approximately the top left key on layout
    const int touchId = 0;

    // Test popup activation

    // direct call

    // Popup won't show up unless it is given a position. We give it via a mouse press.

    //popup should be shown immediately, so we comment out this part of test
#if 0
    subject->touchPointPressed(mousePos, touchId);
    subject->popupStart();
    QVERIFY(subject->isPopupActive());
    subject->popup->hidePopup();
    QVERIFY(!subject->isPopupActive());
    subject->touchPointReleased(mousePos, touchId);

    // make long press
    subject->touchPointPressed(mousePos, touchId);
    QTest::qWait(LongPressTime - 100); // not enough time
    QVERIFY(!subject->isPopupActive());
    subject->touchPointReleased(mousePos, touchId);
#endif

    subject->touchPointPressed(mousePos, touchId);
    QTest::qWait(LongPressTime + 100); // long enough
    QVERIFY(subject->isPopupActive());
    subject->touchPointReleased(mousePos, touchId);
}

void Ut_KeyButtonArea::testInitialization_data()
{
    QTest::addColumn<KBACreator>("createKba");
    QTest::newRow("SingleWidgetArea") << &createSingleWidgetKeyButtonArea;
}

void Ut_KeyButtonArea::testInitialization()
{
    QFETCH(KBACreator, createKba);
    keyboard = new KeyboardData;
    QVERIFY(keyboard->loadNokiaKeyboard("en_us.xml"));
    subject = createKba(style, keyboard->layout(LayoutData::General, M::Landscape)->section(LayoutData::mainSection),
                        KeyButtonArea::ButtonSizeEqualExpanding,
                        false, 0);
    subject->resize(defaultLayoutSize());

}

void Ut_KeyButtonArea::testFunctionRowAlignmentBug_data()
{
    QTest::addColumn<KBACreator>("createKba");
    QTest::newRow("SingleWidgetArea") << &createSingleWidgetKeyButtonArea;
}

// This tests the bug where function row alignment in number portrait keyboard
// didn't respect the right alignment attribute read from xml.
void Ut_KeyButtonArea::testFunctionRowAlignmentBug()
{
    QFETCH(KBACreator, createKba);
    QGraphicsScene scene(QRectF(0, 0, 1000, 1000));

    keyboard = new KeyboardData;
    QVERIFY(keyboard->loadNokiaKeyboard("number.xml"));
    const LayoutData *numberLayout = keyboard->layout(LayoutData::Number, M::Portrait);
    QVERIFY(numberLayout);
    QSharedPointer<const LayoutSection> functionRowSection = numberLayout->section(LayoutData::functionkeySection);

    if (!(functionRowSection->horizontalAlignment() & Qt::AlignRight)) {
        QSKIP("Skipping because the loaded number layout is not right-aligned.", SkipSingle);
    }

    // set equalButtonWidth = false, so button's own width is used
    subject = createKba(style, functionRowSection, KeyButtonArea::ButtonSizeFunctionRow, false, 0);
    scene.addItem(subject);

    // The size is not important as long as there's space for about two buttons.
    const int width = 500;
    subject->resize(width, 1); // KeyButtonArea ignores height

    const QPoint buttonLeftPos = QPoint(20, 20);
    const QPoint buttonRightPos = QPoint(width - 20, 20);

    const bool buttonFoundFromLeft = (subject->keyAt(buttonLeftPos) != 0);
    const bool buttonFoundFromRight = (subject->keyAt(buttonRightPos) != 0);

    scene.removeItem(subject);

    QVERIFY(!buttonFoundFromLeft); // button should not be found from left side
    QVERIFY(buttonFoundFromRight); // button should be found from right side
}

void Ut_KeyButtonArea::testShiftCapsLock()
{
    // Load any layout that has function row with shift
    keyboard = new KeyboardData;
    QVERIFY(keyboard->loadNokiaKeyboard("en_us.xml"));
    const LayoutData *layout = keyboard->layout(LayoutData::General, M::Landscape);
    QVERIFY(layout);
    QSharedPointer<const LayoutSection> functionRowSection = layout->section(LayoutData::functionkeySection);

    subject = createSingleWidgetKeyButtonArea(style, functionRowSection,
                                              KeyButtonArea::ButtonSizeFunctionRow,
                                              false, 0);

    SingleWidgetButton *shiftButton = static_cast<SingleWidgetButtonArea *>(subject)->shiftButton;
    QVERIFY(shiftButton);
    QVERIFY(shiftButton->state() == IKeyButton::Normal);

    subject->setShiftState(ModifierLockedState);
    QVERIFY(shiftButton->state() == IKeyButton::Selected);

    subject->setShiftState(ModifierClearState);
    QVERIFY(shiftButton->state() == IKeyButton::Normal);
}

void Ut_KeyButtonArea::testMultiTouch()
{
    keyboard = new KeyboardData;
    QVERIFY(keyboard->loadNokiaKeyboard("en_us.xml"));
    const LayoutData *layout = keyboard->layout(LayoutData::General, M::Landscape);
    QVERIFY(layout);
    QSharedPointer<const LayoutSection> functionRowSection = layout->section(LayoutData::mainSection);

    subject = createSingleWidgetKeyButtonArea(style, functionRowSection,
                                              KeyButtonArea::ButtonSizeFunctionRow,
                                              false, 0);
    MPlainWindow::instance()->scene()->addItem(subject);
    subject->resize(defaultLayoutSize());

    const IKeyButton *key0 = keyAt(0, 0);
    const IKeyButton *key1 = keyAt(1, 0);
    const IKeyButton *key2 = keyAt(0, 1);

    QVERIFY(key0);
    QVERIFY(key1);
    QVERIFY(key2);

    QSignalSpy pressed(subject, SIGNAL(keyPressed(const IKeyButton*, const QString&, bool)));
    QSignalSpy released(subject, SIGNAL(keyReleased(const IKeyButton*, const QString&, bool)));
    QSignalSpy clicked(subject, SIGNAL(keyClicked(const IKeyButton*, const QString&, bool )));

    QVERIFY(pressed.isValid());
    QVERIFY(released.isValid());
    QVERIFY(clicked.isValid());

    const QPoint pos0 = key0->buttonRect().center();
    const QPoint pos1 = key1->buttonRect().center();
    const QPoint pos2 = key2->buttonRect().center();

    /*
     * Verify following conditions:
     * 1) signals are emitted in correct order
     * 2) every signal corresponds to correct key
     */
    subject->touchPointPressed(pos0, 0);
    QCOMPARE(pressed.count(), 1);
    QVERIFY(pressed.at(0).first().value<const IKeyButton*>() == key0);
    QCOMPARE(released.count(), 0);
    QCOMPARE(clicked.count(), 0);

    subject->touchPointPressed(pos1, 1);
    QCOMPARE(pressed.count(), 2);
    QVERIFY(pressed.at(1).first().value<const IKeyButton*>() == key1);
    QCOMPARE(released.count(), 0);
    QCOMPARE(clicked.count(), 0);

    subject->touchPointReleased(pos0, 0);
    subject->touchPointReleased(pos1, 1);
    QCOMPARE(pressed.count(), 2);
    QCOMPARE(released.count(), 2);
    QVERIFY(released.at(0).first().value<const IKeyButton*>() == key0);
    QVERIFY(released.at(1).first().value<const IKeyButton*>() == key1);
    QCOMPARE(clicked.count(), 2);
    QVERIFY(clicked.at(0).first().value<const IKeyButton*>() == key0);
    QVERIFY(clicked.at(1).first().value<const IKeyButton*>() == key1);

    pressed.clear();
    released.clear();
    clicked.clear();

    // Verify if could click on some keys while other key is pressed
    subject->touchPointPressed(pos0, 0);
    subject->touchPointPressed(pos1, 1);
    subject->touchPointReleased(pos0, 0);
    subject->touchPointPressed(pos2, 0);
    subject->touchPointReleased(pos2, 0);
    subject->touchPointReleased(pos1, 1);

    QCOMPARE(pressed.count(), 3);
    QCOMPARE(released.count(), 3);
    QCOMPARE(clicked.count(), 3);

    QVERIFY(pressed.at(0).first().value<const IKeyButton*>() == key0);
    QVERIFY(pressed.at(1).first().value<const IKeyButton*>() == key1);
    QVERIFY(pressed.at(2).first().value<const IKeyButton*>() == key2);

    QVERIFY(released.at(0).first().value<const IKeyButton*>() == key0);
    QVERIFY(released.at(1).first().value<const IKeyButton*>() == key2);
    QVERIFY(released.at(2).first().value<const IKeyButton*>() == key1);

    QVERIFY(clicked.at(0).first().value<const IKeyButton*>() == key0);
    QVERIFY(clicked.at(1).first().value<const IKeyButton*>() == key2);
    QVERIFY(clicked.at(2).first().value<const IKeyButton*>() == key1);
}

void Ut_KeyButtonArea::testRtlKeys_data()
{
    QTest::addColumn<KBACreator>("createKba");
    QTest::addColumn<M::Orientation>("orientation");
    QTest::addColumn<QString>("fileName");
    QTest::addColumn<QList<KeyBinding::KeyAction> >("expectedRtlKeys");

    const QString ar("ar.xml");
    const QString en_gb("en_gb.xml");
    QList<KeyBinding::KeyAction> rtlKeys;
    const QList<KeyBinding::KeyAction> nothing;

    rtlKeys << KeyBinding::ActionBackspace;

    QTest::newRow("SingleWidgetArea Landscape Arabic")
        << &createSingleWidgetKeyButtonArea
        << M::Landscape
        << ar
        << rtlKeys;

    QTest::newRow("SingleWidgetArea Portrait Arabic" )
        << &createSingleWidgetKeyButtonArea
        << M::Portrait
        << ar
        << rtlKeys;

    QTest::newRow("SingleWidgetArea Landscape English")
        << &createSingleWidgetKeyButtonArea
        << M::Landscape
        << en_gb
        << nothing;

    QTest::newRow("SingleWidgetArea Portrait English" )
        << &createSingleWidgetKeyButtonArea
        << M::Portrait
        << en_gb
        << nothing;
}

void Ut_KeyButtonArea::testRtlKeys()
{
    QFETCH(KBACreator, createKba);
    QFETCH(M::Orientation, orientation);
    QFETCH(QString, fileName);
    QFETCH(QList<KeyBinding::KeyAction>, expectedRtlKeys);

    keyboard = new KeyboardData;
    QVERIFY(keyboard->loadNokiaKeyboard(fileName));
    subject = createKba(style,
                        keyboard->layout(LayoutData::General, orientation)->section(LayoutData::functionkeySection),
                        KeyButtonArea::ButtonSizeEqualExpanding,
                        false, 0);

    SingleWidgetButtonArea *buttonArea = dynamic_cast<SingleWidgetButtonArea *>(subject);

    QVERIFY2(buttonArea != 0, "Unknown type of button area");
    for (int row = 0; row < buttonArea->rowCount(); ++row) {
        for (int column = 0; column < buttonArea->sectionModel()->columnsAt(row); ++column) {
            SingleWidgetButton *button = buttonArea->rowList[row].buttons[column];
            QVERIFY(button != 0);
            if (expectedRtlKeys.contains(button->key().binding()->action())) {
                QVERIFY(button->key().rtl());
                QVERIFY2(button->iconId().contains("-rtl-"), "This is not RTL icon");
            } else {
                QVERIFY(!button->key().rtl());
                QVERIFY2(!button->iconId().contains("-rtl-"), "This is not LTR icon");
            }
        }
    }
}

void Ut_KeyButtonArea::changeOrientation(M::OrientationAngle angle)
{
    if (MPlainWindow::instance()->orientationAngle() != angle) {
        MPlainWindow::instance()->setOrientationAngle(angle);
        //timeout depends on duration of orientation animation
        QTest::qWait(1000);
    }
}

QSize Ut_KeyButtonArea::defaultLayoutSize()
{
    // Take visible scene size as layout size, but reduce keyboard's paddings first from its width.
    // The height value is ignored since KeyButtonAreas determine their own height.
    return MPlainWindow::instance()->visibleSceneSize()
            - QSize((*style)->paddingLeft() + (*style)->paddingRight(), 0);
}

// Helper method to get key in certain row and column from current subject.
IKeyButton *Ut_KeyButtonArea::keyAt(unsigned int row, unsigned int column) const
{
    // If this fails there is something wrong with the test.
    Q_ASSERT(subject
             && (row < static_cast<unsigned int>(subject->rowCount()))
             && (column < static_cast<unsigned int>(subject->sectionModel()->columnsAt(row))));

    IKeyButton *key = 0;

    SingleWidgetButtonArea *buttonArea = dynamic_cast<SingleWidgetButtonArea *>(subject);
    if (buttonArea) {
        key = buttonArea->rowList[row].buttons[column];
    }

    return key;
}

QTEST_APPLESS_MAIN(Ut_KeyButtonArea);
