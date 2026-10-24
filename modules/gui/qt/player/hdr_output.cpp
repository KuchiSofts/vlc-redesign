/* Copyright (C) 2026 VLC authors and VideoLAN. GPL version 2 or later. */
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include "hdr_output.hpp"
#include "maininterface/mainctx.hpp"
#include "qt.hpp"
#include <vlc_configuration.h>
#include <vlc_plugin.h>
#include <cstdlib>
#include <iterator>
#include <QSet>
#include <QSettings>
HdrOutput::HdrOutput(QObject* parent) : QObject(parent)
{
    QSet<int> nativeValues;
    if (config_FindConfig("pl-output-hint")) {
        int64_t* values = nullptr;
        char** labels = nullptr;
        const ssize_t count = config_GetIntChoices("pl-output-hint", &values, &labels);
        for (ssize_t i = 0; i < count; ++i) { nativeValues.insert(values[i]); free(labels[i]); }
        free(values); free(labels);
    }
    m_available = !nativeValues.isEmpty();
    // Only output modes actually implemented by the libplacebo module have a value.
    const int values[] = {0, 1, 2, -1, -1, 3};
    for (const int value : values)
        m_choices.append(QVariantMap{{QStringLiteral("value"), value}, {QStringLiteral("enabled"), nativeValues.contains(value)}});
    retranslate();
}
void HdrOutput::retranslate()
{
    const QString labels[] = {tr("Auto"), tr("Off"), QStringLiteral("HDR10"), QStringLiteral("HDR10+"), QStringLiteral("Dolby Vision"), QStringLiteral("HLG")};
    for (int i = 0; i < m_choices.size() && i < int(std::size(labels)); ++i) {
        QVariantMap choice = m_choices[i].toMap();
        const int value = choice.value(QStringLiteral("value")).toInt();
        choice.insert(QStringLiteral("label"), labels[i]);
        choice.insert(QStringLiteral("reason"), choice.value(QStringLiteral("enabled")).toBool() ? QString() : value < 0
            // HDR10+ and Dolby Vision: listed as the design does
            ? tr("Windows offers no way for a player to send this format to a display.")
            : m_available
            ? tr("This renderer does not expose this output format.")
            : tr("The libplacebo HDR renderer is not installed in this build."));
        m_choices[i] = choice;
    }
}
QString HdrOutput::message() const
{
    switch (m_messageKind) {
    case MessageSaved: return tr("HDR output saved. Restart playback to use it.");
    case MessageNotSaved: return tr("Could not save HDR output. Your previous setting was kept.");
    default:
        return m_available ? tr("Uses libplacebo on next playback. HDR requires a compatible display and driver.")
                           : tr("HDR output is unavailable with the installed video renderer.");
    }
}
void HdrOutput::setCtx(MainCtx* ctx)
{
    if (m_ctx == ctx) return;
    if (m_ctx) disconnect(m_ctx, &MainCtx::interfaceLanguageChanged, this, nullptr);
    m_ctx = ctx;
    // the texts in another language of the interface
    if (m_ctx) connect(m_ctx, &MainCtx::interfaceLanguageChanged, this, [this]() { retranslate(); emit changed(); });
    emit changed();
}
MainCtx* HdrOutput::ctx() const { return m_ctx; }
namespace {
// The HDR formats need the libplacebo renderer; Auto and Off give the
// renderer used before back, kept here meanwhile.
const char previousOutputKey[] = "redesign/hdr-previous-vout";
enum { ChoiceAuto = 0, ChoiceOff = 1 };
QString configString(const char* name)
{
    char* value = config_FindConfig(name) ? config_GetPsz(name) : nullptr;
    const QString result = QString::fromUtf8(value ? value : "");
    free(value);
    return result;
}
// the first module of a module list ("libplacebo,any")
QString firstModule(const QString& list) { return list.section(QLatin1Char(','), 0, 0).trimmed(); }
bool usesLibplacebo() { return firstModule(configString("vout")) == QLatin1String("libplacebo"); }
// Direct3D 11 (picked by default on Windows) has an HDR output of its own
bool usesD3D11()
{
    if (!config_FindConfig("d3d11-hdr-mode")) return false;
    const QString vout = firstModule(configString("vout"));
    return vout.isEmpty() || vout == QLatin1String("any") || vout == QLatin1String("direct3d11");
}
}
int HdrOutput::selectedIndex() const
{
    if (!m_available) return -1;
    int current = config_GetInt("pl-output-hint");
    if (!usesLibplacebo()) {
        // The HDR formats are not used: Auto or Off, as the renderer in use does
        if (usesD3D11())
            current = configString("d3d11-hdr-mode") == QLatin1String("never") ? ChoiceOff : ChoiceAuto;
        else if (current != ChoiceAuto)
            current = ChoiceOff;
    }
    for (int i = 0; i < m_choices.size(); ++i)
        if (m_choices[i].toMap().value(QStringLiteral("value")).toInt() == current) return i;
    return -1;
}
bool HdrOutput::select(int index)
{
    if (!m_ctx || !m_ctx->getIntf() || index < 0 || index >= m_choices.size()
        || !m_choices[index].toMap().value(QStringLiteral("enabled")).toBool()) return false;
    QSettings* settings = m_ctx->getIntf()->mainSettings;
    const int value = m_choices[index].toMap().value(QStringLiteral("value")).toInt();
    const bool hdrFormat = value != ChoiceAuto && value != ChoiceOff;
    const bool hasD3D11Mode = config_FindConfig("d3d11-hdr-mode") != nullptr;
    const int beforeHint = config_GetInt("pl-output-hint");
    const QByteArray beforeOutput = configString("vout").toUtf8();
    const QByteArray beforeD3D11Mode = configString("d3d11-hdr-mode").toUtf8();
    const bool wasLibplacebo = usesLibplacebo();

    config_PutInt("pl-output-hint", value);
    if (hdrFormat) {
        if (!wasLibplacebo)
            settings->setValue(previousOutputKey, QString::fromUtf8(beforeOutput));
        config_PutPsz("vout", "libplacebo");
    } else {
        // Back to the renderer used before the HDR formats (when they changed it)
        if (wasLibplacebo && settings->contains(previousOutputKey))
            config_PutPsz("vout", settings->value(previousOutputKey).toString().toUtf8().constData());
        if (hasD3D11Mode)
            config_PutPsz("d3d11-hdr-mode", value == ChoiceOff ? "never" : "auto");
    }
    const bool saved = config_SaveConfigFile(m_ctx->getIntf()) == 0;
    if (saved) {
        if (!hdrFormat)
            settings->remove(previousOutputKey);
    } else {
        config_PutInt("pl-output-hint", beforeHint);
        config_PutPsz("vout", beforeOutput.constData());
        if (hasD3D11Mode)
            config_PutPsz("d3d11-hdr-mode", beforeD3D11Mode.constData());
        if (hdrFormat && !wasLibplacebo)
            settings->remove(previousOutputKey);
    }
    m_messageKind = saved ? MessageSaved : MessageNotSaved;
    emit changed();
    return saved;
}
