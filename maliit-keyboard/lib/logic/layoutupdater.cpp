/*
 * This file is part of Maliit Plugins
 *
 * Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies). All rights reserved.
 *
 * Contact: Mohammad Anwari <Mohammad.Anwari@nokia.com>
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice, this list
 * of conditions and the following disclaimer.
 * Redistributions in binary form must reproduce the above copyright notice, this list
 * of conditions and the following disclaimer in the documentation and/or other materials
 * provided with the distribution.
 * Neither the name of Nokia Corporation nor the names of its contributors may be
 * used to endorse or promote products derived from this software without specific
 * prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
 * THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include "layoutupdater.h"
#include "style.h"
#include "logic/state-machines/shiftmachine.h"
#include "logic/state-machines/viewmachine.h"
#include "logic/state-machines/deadkeymachine.h"
#include "models/keyboard.h"
#include "models/keydescription.h"

namespace MaliitKeyboard {

namespace {

enum Transform {
    TransformToUpper,
    TransformToLower
};

bool verify(const QScopedPointer<KeyboardLoader> &loader,
            const SharedLayout &layout)
{
    if (loader.isNull() || layout.isNull()) {
        qCritical() << __PRETTY_FUNCTION__
                    << "Could not find keyboard loader or layout, forgot to set them?";
        return false;
    }

    return true;
}

// FIXME: only access settings *once*
QPoint computeAnchor(Layout::Orientation orientation)
{
    // A workaround for virtual desktops where QDesktopWidget reports
    // screenGeometry incorrectly: Allow user to override via settings.
    QSettings settings("maliit.org", "vkb");
    QPoint anchor(settings.value("anchor").toPoint());

    if (anchor.isNull()) {
        const QRect screen_area(QApplication::desktop() ? QApplication::desktop()->screenGeometry()
                                                        : QRect(0, 0, 480, 854));
        anchor = (orientation == Layout::Landscape)
                  ? QPoint(screen_area.width() / 2, screen_area.height())
                  : QPoint(screen_area.height() / 2, screen_area.width());

        // Enforce creation of settings file, otherwise it's too hard to find
        // the override (and get the syntax ride). I know, it's weird.
        settings.setValue("_anchor", anchor);
    }

    return anchor;
}

KeyArea transformKeyArea(const KeyArea &ka,
                         Transform t)
{
    KeyArea new_ka;
    new_ka.rect = ka.rect;

    foreach (Key key, ka.keys) {
        KeyLabel label(key.label());

        switch (t) {
        case TransformToUpper:
            label.setText(label.text().toUpper());
            break;

        case TransformToLower:
            label.setText(label.text().toLower());
            break;
        }

        key.setLabel(label);
        new_ka.keys.append(key);
    }

    return new_ka;
}

KeyArea replaceKey(const KeyArea &ka,
                   const Key &replace)
{
    KeyArea new_ka;
    new_ka.rect = ka.rect;

    foreach (const Key &key, ka.keys) {
        new_ka.keys.append((key.label().text() == replace.label().text()) ? replace : key);
    }

    return new_ka;
}

KeyArea createFromKeyboard(Style *style,
                           const Keyboard &source,
                           const QPoint &anchor,
                           Layout::Orientation orientation)
{
    // An ad-hoc geometry updater that also uses styling information.
    // Will only work for portrait mode (lots of hardcoded stuff).
    KeyArea ka;
    Keyboard kb(source);

    if (not style) {
        qCritical() << __PRETTY_FUNCTION__
                    << "No style given, aborting.";
        return ka;
    }

    style->setStyleName(kb.style_name);

    static SharedFont font(new QFont(style->fontName()));
    font->setPixelSize(style->fontSize());

    static SharedFont small_font(new QFont(style->fontName()));
    small_font->setPointSize(12);

    static const QMargins bg_margins(6, 6, 6, 6);

    const qreal max_width(style->keyAreaWidth(orientation));
    const qreal key_height(style->keyHeight(orientation));
    const qreal margin = style->keyMargin(orientation);
    const qreal padding = style->keyAreaPadding(orientation);

    QPoint pos(0, 0);
    int prev_row = 0;
    QVector<int> row_indices;
    int spacer_count = 0;


    for (int index = 0; index < kb.keys.count(); ++index) {
        row_indices.append(index);
        Key &key(kb.keys[index]);
        const KeyDescription &desc(kb.key_descriptions.at(index));
        int width = 0;
        pos.setY(key_height * desc.row);
        prev_row = desc.row;

        bool at_row_end((index + 1 == kb.keys.count())
                        || (index + 1 < kb.keys.count()
                            && kb.key_descriptions.at(index + 1).row > desc.row));

        if (desc.left_spacer || desc.right_spacer) {
            ++spacer_count;
        }

        key.setBackground(style->keyBackground(desc.style, KeyDescription::NormalState));
        key.setBackgroundBorders(bg_margins);

        width = style->keyWidth(orientation, desc.width);
        const bool use_left_padding(pos.x() == 0);
        const bool use_right_padding(pos.x() + width + margin + padding == max_width);

        const qreal key_margin((use_left_padding || use_right_padding) ? margin + padding : margin * 2);
        key.setRect(QRect(pos.x(), pos.y(), width + key_margin, key_height));
        key.setMargins(QMargins(use_left_padding ? padding : margin, margin,
                                use_right_padding ? padding : margin, margin));

        KeyLabel label(key.label());
        label.setFont(label.text().count() > 1 ? small_font : font);
        label.setColor(Qt::white);
        key.setLabel(label);

        // FIXME: Read from KeyDescription instead.
        if (key.label().text().isEmpty()) {
            switch (key.action()) {
            case Key::ActionShift:
                key.setIcon(style->icon(KeyDescription::ShiftIcon,
                                        KeyDescription::NormalState));
                break;

            case Key::ActionBackspace:
                key.setIcon(style->icon(KeyDescription::BackspaceIcon,
                                        KeyDescription::NormalState));
                break;

            case Key::ActionReturn:
                key.setIcon(style->icon(KeyDescription::ReturnIcon,
                                        KeyDescription::NormalState));
                break;

            default:
                break;
            }
        }

        pos.rx() += key.rect().width();

        if (at_row_end) {
            if (spacer_count > 0 && pos.x() < max_width + 1) {
                const int spacer_width = qMax<int>(0, max_width - pos.x()) / spacer_count;
                pos.setX(0);
                int right_x = 0;

                foreach (int row_index, row_indices) {
                    Key &k(kb.keys[row_index]);
                    const KeyDescription &d(kb.key_descriptions.at(row_index));

                    QRect r(k.rect());
                    QMargins m(k.margins());
                    int extra_width = 0;

                    if (d.left_spacer) {
                        m.setLeft(m.left() + spacer_width);
                        extra_width += spacer_width;
                    }

                    if (d.right_spacer) {
                        m.setRight(m.right() + spacer_width);
                        extra_width += spacer_width;
                    }

                    k.setMargins(m);

                    r.translate(right_x - r.left(), 0);
                    r.setWidth(r.width() + extra_width);
                    k.setRect(r);

                    right_x = r.right();
                }
            }

            row_indices.clear();
            pos.setX(0);
            spacer_count = 0;
        }
    }

    const int height = pos.y() + key_height;
    ka.keys = kb.keys;
    ka.rect =  QRectF(anchor.x() - max_width / 2, anchor.y() - height,
                      max_width, height);

    return ka;
}

}

class LayoutUpdaterPrivate
{
public:
    bool initialized;
    SharedLayout layout;
    QScopedPointer<KeyboardLoader> loader;
    ShiftMachine shift_machine;
    ViewMachine view_machine;
    DeadkeyMachine deadkey_machine;
    QPoint anchor;
    Style style;

    explicit LayoutUpdaterPrivate()
        : initialized(false)
        , layout()
        , loader(new KeyboardLoader)
        , shift_machine()
        , view_machine()
        , deadkey_machine()
        , anchor()
        , style()
    {
        style.setProfile("nokia-n9");
    }
};

LayoutUpdater::LayoutUpdater(QObject *parent)
    : QObject(parent)
    , d_ptr(new LayoutUpdaterPrivate)
{
    connect(d_ptr->loader.data(), SIGNAL(keyboardsChanged()),
            this,                 SLOT(onKeyboardsChanged()),
            Qt::UniqueConnection);
}

LayoutUpdater::~LayoutUpdater()
{}

void LayoutUpdater::init()
{
    Q_D(LayoutUpdater);

    d->anchor = computeAnchor(Layout::Landscape);
    d->shift_machine.setup(this);
    d->view_machine.setup(this);
    d->deadkey_machine.setup(this);
}

QStringList LayoutUpdater::keyboardIds() const
{
    Q_D(const LayoutUpdater);
    return d->loader->ids();
}

QString LayoutUpdater::activeKeyboardId() const
{
    Q_D(const LayoutUpdater);
    return d->loader->activeId();
}

void LayoutUpdater::setActiveKeyboardId(const QString &id)
{
    Q_D(LayoutUpdater);
    d->loader->setActiveId(id);
}

QString LayoutUpdater::keyboardTitle(const QString &id) const
{
    Q_D(const LayoutUpdater);
    return d->loader->title(id);
}

void LayoutUpdater::setLayout(const SharedLayout &layout)
{
    Q_D(LayoutUpdater);
    d->layout = layout;

    if (not d->initialized) {
        init();
        d->initialized = true;
    }
}

void LayoutUpdater::resetKeyboardLoader(KeyboardLoader *loader)
{
    Q_D(LayoutUpdater);
    d->loader.reset(loader);

    connect(loader, SIGNAL(keyboardsChanged()),
            this,   SLOT(onKeyboardsChanged()),
            Qt::UniqueConnection);
}

void LayoutUpdater::setOrientation(Layout::Orientation orientation)
{
    Q_D(LayoutUpdater);

    if (d->layout && d->layout->orientation() != orientation) {
        d->layout->setOrientation(orientation);

        // FIXME: reposition extended keys, too (and left/right?).
        // FIXME: consider shift state.
        d->anchor = computeAnchor(orientation);
        d->layout->setCenterPanel(createFromKeyboard(&d->style,
                                                     d->loader->keyboard(),
                                                     d->anchor,
                                                     orientation));

        d->layout->clearActiveKeys();
        emit layoutChanged(d->layout);
    }
}

void LayoutUpdater::onKeyPressed(const Key &key,
                                 const SharedLayout &layout)
{
    Q_D(LayoutUpdater);

    if (d->layout != layout) {
        return;
    }

    // FIXME: Remove test code
    // TEST CODE STARTS
    static SharedFont magnifier_font(new QFont);
    magnifier_font->setPixelSize(50);
    static const QMargins bg_margins(6, 6, 6, 6);
    // TEST CODE ENDS

    Key k(key);
    // FIXME: Use correct key style, but where to look it up?
    k.setBackground(d->style.keyBackground(KeyDescription::NormalStyle,
                                           KeyDescription::PressedState));
    k.setBackgroundBorders(bg_margins);
    layout->appendActiveKey(k);

    if (key.action() == Key::ActionInsert) {
        Key magnifier(key);
        magnifier.setBackground(d->style.keyBackground(KeyDescription::NormalStyle,
                                                       KeyDescription::PressedState));

        QRect magnifier_rect(key.rect().translated(0, -120).adjusted(-20, -20, 20, 20));
        const QRectF key_area_rect(d->layout->activeKeyArea().rect);
        if (magnifier_rect.left() < key_area_rect.left() + 10) {
            magnifier_rect.setLeft(key_area_rect.left() + 10);
        } else if (magnifier_rect.right() > key_area_rect.right() - 10) {
            magnifier_rect.setRight(key_area_rect.right() - 10);
        }

        magnifier.setRect(magnifier_rect);
        magnifier.setBackgroundBorders(bg_margins);

        KeyLabel magnifier_label(magnifier.label());
        magnifier_label.setColor(Qt::white);
        magnifier_label.setFont(magnifier_font);
        magnifier.setLabel(magnifier_label);
        layout->setMagnifierKey(magnifier);
    }

    switch (key.action()) {
    case Key::ActionShift:
        emit shiftPressed();
        break;

    case Key::ActionDead:
        d->deadkey_machine.setAccentKey(key);
        emit deadkeyPressed();
        break;

    default:
        break;
    }

    emit keysChanged(layout);
}

void LayoutUpdater::onKeyReleased(const Key &key,
                                  const SharedLayout &layout)
{
    Q_D(const LayoutUpdater);

    if (d->layout != layout) {
        return;
    }

    layout->removeActiveKey(key);
    layout->setMagnifierKey(Key());

    switch (key.action()) {
    case Key::ActionShift:
        emit shiftReleased();
        break;

    case Key::ActionInsert:
        if (d->shift_machine.inState("latched-shift")) {
            emit shiftCancelled();
        }

        if (d->deadkey_machine.inState("latched-deadkey")) {
            emit deadkeyCancelled();
        }

        break;

    case Key::ActionSym:
        emit symKeyReleased();
        break;

    case Key::ActionSwitch:
        emit symSwitcherReleased();
        break;

    case Key::ActionDead:
        emit deadkeyReleased();
        break;

    default:
        break;
    }

    emit keysChanged(layout);
}

void LayoutUpdater::switchLayoutToUpper()
{
    Q_D(const LayoutUpdater);

    if (not d->layout) {
        return;
    }

    d->layout->setActiveKeyArea(transformKeyArea(d->layout->activeKeyArea(), TransformToUpper));
    emit layoutChanged(d->layout);
}

void LayoutUpdater::switchLayoutToLower()
{
    Q_D(const LayoutUpdater);

    if (not d->layout) {
        return;
    }

    d->layout->setActiveKeyArea(transformKeyArea(d->layout->activeKeyArea(), TransformToLower));
    emit layoutChanged(d->layout);
}

void LayoutUpdater::onKeyboardsChanged()
{
    Q_D(LayoutUpdater);

    if (not verify(d->loader, d->layout)) {
        return;
    }

    d->layout->setCenterPanel(createFromKeyboard(&d->style,
                                                 d->loader->keyboard(),
                                                 d->anchor,
                                                 d->layout->orientation()));
    emit layoutChanged(d->layout);
}

void LayoutUpdater::switchToMainView()
{
    // This will undo the changes done by shift, which is perhaps what we want.
    // But if shift state is actually dependent on view state, then that's
    // needs to be modelled as part of the state machines, or not?
     onKeyboardsChanged();
}

void LayoutUpdater::switchToPrimarySymView()
{
    Q_D(LayoutUpdater);

    if (not verify(d->loader, d->layout)) {
        return;
    }

    d->layout->setCenterPanel(createFromKeyboard(&d->style,
                                                 d->loader->symbolsKeyboard(0),
                                                 d->anchor,
                                                 d->layout->orientation()));
    // Reset shift state machine, also see switchToMainView.
    d->shift_machine.restart();

    //d->shift_machine->start();
    emit layoutChanged(d->layout);
}

void LayoutUpdater::switchToSecondarySymView()
{
    Q_D(LayoutUpdater);

    if (not verify(d->loader, d->layout)) {
        return;
    }

    d->layout->setCenterPanel(createFromKeyboard(&d->style,
                                                 d->loader->symbolsKeyboard(1),
                                                 d->anchor,
                                                 d->layout->orientation()));
    emit layoutChanged(d->layout);
}

void LayoutUpdater::switchToAccentedView()
{
    Q_D(LayoutUpdater);

    if (not verify(d->loader, d->layout)) {
        return;
    }

    d->layout->setCenterPanel(createFromKeyboard(&d->style,
                                                 d->loader->deadKeyboard(d->deadkey_machine.accentKey()),
                                                 d->anchor,
                                                 d->layout->orientation()));

    emit layoutChanged(d->layout);
}

} // namespace MaliitKeyboard