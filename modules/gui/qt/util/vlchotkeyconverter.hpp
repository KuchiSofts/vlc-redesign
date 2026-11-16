#ifndef VLC_HOTKEY_CONVERTER_HPP
#define VLC_HOTKEY_CONVERTER_HPP

#include <QObject>
#include <QPoint>
#include <QtQml/qqmlregistration.h>

/* VLC Key/Wheel hotkeys interactions */

Q_MOC_INCLUDE("QKeyEvent")
Q_MOC_INCLUDE("QWheelEvent")

class QKeyEvent;
class QWheelEvent;
class QInputEvent;

int qtKeyModifiersToVLC( const QInputEvent& e );
int qtEventToVLCKey( QKeyEvent *e );
/* whether a hotkey of VLC (not global) takes this key */
bool isVLCKeyBound( struct vlc_object_t *obj, int vlcKey );
/* the action of VLC (not global) this key has, as the core maps it: its name
 * without "key-" ("jump+short"), empty for none */
QString vlcKeyAction( struct vlc_object_t *obj, int vlcKey );
int qtWheelEventToVLCKey( const QWheelEvent& e );
QString VLCKeyToString( unsigned val, bool );

/**
 * @brief The WheelToVLCConverter class aggregates wheel events and
 * emit a signal once it gathers a full scroll step, as VLC doesn't handle
 * fractionnal scroll events
 */
class WheelToVLCConverter : public QObject
{
    Q_OBJECT
    QML_ELEMENT

public:
    using QObject::QObject;

public:
    Q_INVOKABLE Qt::Orientations getWheelOrientation(int x, int y);

    /**
     * "Reverse scroll direction" (Settings, Controls, Mouse: the option
     * qt-wheel-reverse): every plain wheel the other way round, whatever it
     * does here (the volume, seeking, the speed). A wheel with a modifier is
     * a shortcut that names its direction, and a drag on a touch screen is
     * not a wheel: both as they are.
     */
    static void setReversed(bool reversed);
    Q_INVOKABLE bool reversed() const;

signals:
    /**
     * @param vlcKey the VLC hotkey representation
     */
    void vlcWheelKey(int vlcKey);
    /**
     * @param steps: positive value indicated UP wheel events, negative DOWN wheel event
     * @param modifiers are keyboard pressed modifiers
     */
    void wheelUpDown(int steps, Qt::KeyboardModifiers modifiers);
    /**
     * @param steps: positive value indicated UP wheel events, negative DOWN wheel event
     * @param modifiers are keyboard pressed modifiers
     */
    void wheelLeftRight(int steps, Qt::KeyboardModifiers modifiers);

public slots:
    /**
     * @brief qmlWheelEvent handles wheel events as emitted by QWidget
     * @param e the wheel event
     */
    void wheelEvent(const QWheelEvent* e);

    /**
     * @brief qmlWheelEvent handles wheel events as emitted by QML
     * @param wheelEvent qml wheel event
     */
    void qmlWheelEvent(const QObject* wheelEvent);

    void customWheelEvent(const QPoint& pixelDelta,
                          const QPoint& angleDelta,
                          const Qt::MouseButtons buttons = {},
                          const Qt::KeyboardModifiers modifiers = {},
                          const bool inverted = false);

private:
    void handleWheel(const QWheelEvent* e, bool reversible);

    QPoint m_scrollAmount = {};
    Qt::KeyboardModifiers m_modifiers = {};
    Qt::MouseButtons m_buttons = {};
};

#endif // VLC_HOTKEY_CONVERTER_HPP
