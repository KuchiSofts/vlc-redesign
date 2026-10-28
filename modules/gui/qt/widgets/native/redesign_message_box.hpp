/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/
#ifndef REDESIGN_MESSAGE_BOX_HPP
#define REDESIGN_MESSAGE_BOX_HPP

#include <QDialog>
#include <QString>
#include <QVariantMap>

class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QPushButton;
struct qt_intf_t;

/**
 * A message, a question or a prompt for a text, as the dialogs of the
 * redesign show it (Widgets.RedesignDialog): the title bar (the title and the
 * close button), a heading and a line of explanation, an optional field, and
 * the footer (the cancel button on the left, the action on the right).
 */
struct RedesignMessage
{
    enum Answer
    {
        Cancelled = 0, // the cancel button, the close button, Escape
        Accepted = 1,  // the action (on the right)
        Extra = 2,     // the second action, before the main one
    };
    enum Input
    {
        NoInput = 0,
        LineInput = 1,
        TextInput = 2, // several lines
    };

    QString title;       // in the title bar
    QString heading;     // Inter 16, may wrap
    QString text;        // the explanation under it, Inter 14
    QString acceptText;  // the action
    QString cancelText;  // empty: "Cancel"
    QString extraText;   // empty: none
    bool cancelVisible = true;
    // Return gives the action (else it is the cancel button: a question
    // that deletes something)
    bool defaultAccept = true;
    Input input = NoInput;
    QString inputText;
    QString placeholder;

    QVariantMap toVariantMap() const;
};

/**
 * The same dialog drawn with widgets: for what the window of the interface
 * cannot show (a classic window asking, no main interface, the interface
 * hidden). Frameless, in the colours, sizes and font of RedesignDialog.qml,
 * following the theme and the accent of the interface.
 */
class RedesignMessageBox : public QDialog
{
public:
    RedesignMessageBox(qt_intf_t *p_intf, const RedesignMessage &message, QWidget *parent = nullptr);

    // RedesignMessage::Answer
    int answer() const { return m_answer; }
    QString inputText() const;

    // Shown modal (over parent, else over the window of the interface);
    // returns a RedesignMessage::Answer
    static int ask(qt_intf_t *p_intf, const RedesignMessage &message, QWidget *parent = nullptr,
                   QString *input = nullptr);

    // The common message boxes of the classic windows
    static void warning(qt_intf_t *p_intf, QWidget *parent, const QString &title, const QString &text);
    static bool question(qt_intf_t *p_intf, QWidget *parent, const QString &title, const QString &text,
                         const QString &acceptText, bool defaultAccept = false);

    // Kept over the window of the interface (no parent given)
    void setInterfaceTransientParent();

    void done(int r) override;

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void showEvent(QShowEvent *event) override;

private:
    void finish(int answer);

    qt_intf_t *const p_intf;
    const RedesignMessage m_message;
    QWidget *m_titleBar = nullptr;
    QLineEdit *m_line = nullptr;
    QPlainTextEdit *m_multiLine = nullptr;
    int m_answer = RedesignMessage::Cancelled;
    bool m_answered = false;
};

#endif // REDESIGN_MESSAGE_BOX_HPP
