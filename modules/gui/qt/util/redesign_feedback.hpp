/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *****************************************************************************/
#pragma once

#include <QObject>
#include <QString>

#include "qt.hpp"

// "Suggest an idea" and "Report a problem" (⋯ menu, About): the links to
// the forms of the GitHub repository, the report prefilled with what helps
// to find a problem (versions, Windows, graphics, screens, look). The card
// shows that text first; the browser opens on the form and the user submits
// it there: nothing is sent from here. No personal data: no file names or
// paths, no user or computer name, no address, no library, no log.
class RedesignFeedback : public QObject
{
    Q_OBJECT

public:
    explicit RedesignFeedback(qt_intf_t *intf, QObject *parent = nullptr);

    // The system information of the report, one "Name: value" per line, in
    // English (read by the maintainers). The look is the interface's:
    // colorScheme as ColorSchemeModel (System, Day, Night), whether it is
    // dark now, the accent ("#d37918").
    Q_INVOKABLE QString reportText(int colorScheme, bool darkTheme, const QString &accent) const;
    // The bug form prefilled with it (cut at a line to stay under 8,000
    // characters)
    Q_INVOKABLE QString reportUrl(const QString &report) const;
    // The suggestion form, with the version
    Q_INVOKABLE QString ideaUrl() const;
    Q_INVOKABLE void copyText(const QString &text) const;

    // opens the card (SetupDialogs.qml)
    Q_INVOKABLE void openReport() { emit reportRequested(); }

signals:
    void reportRequested();

private:
    qt_intf_t *m_intf;
};
