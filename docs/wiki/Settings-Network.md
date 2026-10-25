# Settings: Network

**⋯ → Settings → Network** (<kbd>Ctrl</kbd>+<kbd>P</kbd>; the header reads *Network Connections & Security*). How
much VLC buffers before it plays, the proxy it goes through, and how it checks encrypted connections. No row of this
page is reserved to Developer mode. See [how the Settings window works](Settings#how-the-window-works).

## Buffering

| Option | Choices / default | What it does | VLC option |
|---|---|---|---|
| **Buffering** | Dropdown: *Lowest latency* / *Low* / *Normal* / *High* / *Custom*. Default: *Normal*. | How much of a media VLC reads ahead before it starts to play, for the four kinds of source below at once. More buffering plays more smoothly on a slow or unsteady connection or drive; less starts and seeks faster and keeps live streams closer to real time. Each preset sets the four delays (table below); *Custom* lets you type them, and delays that match no preset show as *Custom*. | none: it writes the four rows below |
| **Local files** | Number field, in ms, 0 to 60000. Default: 1000. Dimmed and empty unless Buffering is *Custom*. | Buffering of files on the computer, and of shared folders opened as files (`\\server\share`). | `file-caching` |
| **Discs** | Number field, in ms, 0 to 60000. Default: 300. Dimmed and empty unless *Custom*. | Buffering of optical discs (DVD, Blu-ray, audio CD, VCD). | `disc-caching` |
| **Network streams** | Number field, in ms, 0 to 60000. Default: 1000. Dimmed and empty unless *Custom*. | Buffering of network resources: web streams (HTTP, HLS, DASH), RTSP and the other network protocols. | `network-caching` |
| **Live capture** | Number field, in ms, 0 to 60000. Default: 300. Dimmed and empty unless *Custom*. | Buffering of capture devices: cameras, microphones, capture cards, the screen. | `live-caching` |

The presets, in milliseconds:

| Preset | Local files | Discs | Network streams | Live capture |
|---|---|---|---|---|
| Lowest latency | 100 | 100 | 150 | 100 |
| Low | 300 | 150 | 500 | 150 |
| Normal (VLC's defaults) | 1000 | 300 | 1000 | 300 |
| High | 3000 | 1000 | 3000 | 1000 |

## Proxy

*(This section appears on Windows only: VLC has these options only there.)*

| Option | Choices / default | What it does | VLC option |
|---|---|---|---|
| **HTTP proxy** | Text field. Default: empty; the field then reads *System proxy*. | The proxy VLC connects through for HTTP and HTTPS, in the form `http://[user@]myproxy.mydomain:myport/`. Set it when the proxy is one Windows does not know. Empty, VLC uses the proxy of Windows (below). *Changed in 0.3.1: before, an empty field meant no proxy.* | `http-proxy` |
| **HTTP proxy password** | Password field. Default: empty. | The password of the proxy set above, if it asks for one. It is added to the proxy address when the address does not hold one already. It is not used with the proxy of Windows. | `http-proxy-pwd` |

**The proxy of Windows.** While *HTTP proxy* is empty, VLC reads the manual proxy of Windows (**Settings → Network &
internet → Proxy → Manual proxy setup**, the same as *Internet Options → Connections → LAN settings*) each time it
opens an HTTP or HTTPS connection, so a change in Windows applies without restarting VLC.

- **One proxy for all protocols** is used for everything. A list per protocol (`http=proxy1:8080;https=proxy2:8443`)
  gives HTTPS addresses the `https=` entry and the others the `http=` entry (HTTPS takes the `http=` entry too when
  there is no `https=` one). An address without a scheme gets `http://` in front of it.
- **The addresses that go without the proxy** (Windows' *Use the proxy server except for addresses that start with
  the following entries* list) are honoured for exact host names, for entries starting with `*` (`*.example.com`:
  any host ending so), and for `<local>` (*Don't use the proxy server for local (intranet) addresses*: a host name
  without a dot). An entry with a `*` elsewhere (`192.168.*`) is not matched by VLC.
- **Automatic configuration is not followed:** *Automatically detect settings* and *Use setup script* (a PAC file)
  are ignored; VLC uses only the manual proxy. If your network needs a setup script, type the proxy it gives you in
  *HTTP proxy*.

## Security

| Option | Choices / default | What it does | VLC option |
|---|---|---|---|
| **Verify certificates against the system trust store** | Toggle. Default: on. | Trusts the root certificates of the certificate authorities stored in Windows to authenticate encrypted (TLS) connections, such as HTTPS streams. Off, only the certificates of *Trusted certificates folder* are trusted, so most HTTPS addresses fail. Needs the GnuTLS module. | `gnutls-system-trust` |
| **Cipher preference** | Dropdown: *Fastest* / *Balanced* / *Secure (128-bit)* / *Most secure (256-bit)*. Default: *Balanced*. | Which ciphers, key exchange methods and hash functions VLC offers on encrypted connections. *Fastest* puts the faster ciphers first; *Secure (128-bit)* asks for 128-bit or stronger; *Most secure (256-bit)* leaves out the 128-bit ciphers. A stricter choice can fail with servers that do not offer it. Needs the GnuTLS module. | `gnutls-priorities` (`PERFORMANCE` / `NORMAL` / `SECURE128` / `SECURE256`) |

## Advanced Options

| Option | Choices / default | What it does | VLC option |
|---|---|---|---|
| **Connection timeout** | Number field, in ms, 0 or more. Default: 5000. | How long VLC waits for a network (TCP) connection to open before it gives up. Raise it for slow or distant servers. | `ipv4-timeout` |
| **HTTP user agent** | Text field. Default: empty (VLC sends its own name and version). | Replaces the application name and version VLC sends to HTTP servers (the HTTP *User-Agent*). Name and version are separated by a forward slash, for example `FooBar/1.2.3`. Some servers only answer certain players. | `http-user-agent` |
| **Trusted certificates folder** | Text field (a folder path). Default: empty. | A folder of root certificates of certificate authorities that VLC trusts as well for encrypted (TLS) connections, for example the private authority of a company. Needs the GnuTLS module. | `gnutls-dir-trust` |

## Good to know

- **A network stream stutters or stops to buffer:** choose *High*, or *Custom* with a larger *Network streams* value
  (3000 to 10000 ms). A live stream then runs that much behind real time.
- **A file on a slow USB drive or a network share stutters:** raise *Local files*.
- **Seeking feels slow** on a fast drive or network: *Low* starts and seeks faster.
- **Behind a company proxy,** leaving *HTTP proxy* empty is usually enough, as VLC follows Windows. The (i) text of
  the row comes from VLC and mentions the `http_proxy` environment variable; this Windows build uses the proxy of
  Windows instead.
- **HTTPS fails with a certificate error:** keep *Verify certificates against the system trust store* on; for a
  server with a private certificate authority, put its root certificate in a folder and set *Trusted certificates
  folder*.
- **Casting** has its own local server, not these settings: its port is in [Recording](Settings-Recording), and the
  firewall question is answered in [Casting troubleshooting](Casting-Troubleshooting#the-firewall-question).

**Related pages:** [Library: Network](Library#network) (network shares and servers) · [Casting](Casting) · [Troubleshooting and FAQ](Troubleshooting-and-FAQ#video-problems-no-picture-black-screen-stutter-wrong-colours)

---

← [Library](Settings-Library) · [All settings](Settings) · [Performance](Settings-Performance) →
