/* Copyright (C) 2026 VLC authors and VideoLAN. GPL version 2 or later. */
#ifndef VLC_QT_HDR_OUTPUT_HPP
#define VLC_QT_HDR_OUTPUT_HPP
#include <QObject>
#include <QPointer>
#include <QVariantList>
class MainCtx;
Q_MOC_INCLUDE("maininterface/mainctx.hpp")
class HdrOutput : public QObject {
    Q_OBJECT
    Q_PROPERTY(MainCtx* ctx READ ctx WRITE setCtx NOTIFY changed)
    Q_PROPERTY(QVariantList choices READ choices NOTIFY changed)
    Q_PROPERTY(bool available READ available CONSTANT)
    Q_PROPERTY(int selectedIndex READ selectedIndex NOTIFY changed)
    Q_PROPERTY(QString message READ message NOTIFY changed)
public:
    explicit HdrOutput(QObject* parent = nullptr);
    MainCtx* ctx() const;
    void setCtx(MainCtx* ctx);
    QVariantList choices() const { return m_choices; }
    bool available() const { return m_available; }
    int selectedIndex() const;
    QString message() const;
    Q_INVOKABLE bool select(int index);
signals:
    void changed();
private:
    // The labels and the reasons of the choices in the language of the
    // interface (made again when it changes)
    void retranslate();

    QPointer<MainCtx> m_ctx;
    QVariantList m_choices;
    bool m_available = false;
    // What the message says: what the choice does, or how the last one went
    enum { MessageInfo, MessageSaved, MessageNotSaved } m_messageKind = MessageInfo;
};
#endif
