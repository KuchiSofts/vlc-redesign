# Settings: Network

**⋯ → Settings → Network** (or <kbd>Ctrl</kbd>+<kbd>P</kbd>). Buffering of network streams, a proxy, and the security of encrypted connections. The page is titled *Network Connections & Security*.

Rows under **Advanced Options** are in the collapsed bar at the bottom of the page. This page has no rows reserved to Developer mode. See [how the Settings window works](Settings).

### Buffering
- **Buffering**: dropdown: Lowest latency / Low / Normal / High / Custom. Default: Normal. How much of a media VLC reads ahead before it plays, for the four kinds of source below at once. More buffering plays more smoothly on a slow or unsteady connection; less buffering starts faster and keeps live streams closer to real time. Each preset sets the four delays, in milliseconds:

  | Preset | Local files | Discs | Network streams | Live capture |
  |---|---|---|---|---|
  | Lowest latency | 100 | 100 | 150 | 100 |
  | Low | 300 | 150 | 500 | 150 |
  | Normal (VLC's defaults) | 1000 | 300 | 1000 | 300 |
  | High | 3000 | 1000 | 3000 | 1000 |

  Delays that match no preset show as Custom. Not a VLC option of its own: it writes the four options below.
- **Local files**: number field, in ms (0 to 60000). Default: 1000. Buffering of local files. *(Dimmed unless Buffering is Custom.)* <sub>VLC option: `file-caching`</sub>
- **Discs**: number field, in ms (0 to 60000). Default: 300. Buffering of optical discs (DVD, Blu-ray, audio CD). *(Dimmed unless Buffering is Custom.)* <sub>VLC option: `disc-caching`</sub>
- **Network streams**: number field, in ms (0 to 60000). Default: 1000. Buffering of network resources (web streams and the other network protocols). *(Dimmed unless Buffering is Custom.)* <sub>VLC option: `network-caching`</sub>
- **Live capture**: number field, in ms (0 to 60000). Default: 300. Buffering of cameras and microphones. *(Dimmed unless Buffering is Custom.)* <sub>VLC option: `live-caching`</sub>

### Proxy
*(This section appears on Windows only: VLC has these options only there.)*
- **HTTP proxy**: text field. Default: empty (the field shows "System proxy"). The HTTP proxy VLC connects through, in the form `http://[user@]myproxy.mydomain:myport/`. Set it when your network only reaches the internet through a proxy. Left empty, VLC uses the proxy of Windows: its manual proxy setting (Windows Settings > Network & internet > Proxy), read with WinHTTP, for each kind of address (http, https) and with its list of addresses that go without the proxy. An automatic setup script (PAC file) is not followed. <sub>VLC option: `http-proxy`</sub>
- **HTTP proxy password**: password field. Default: empty. The password of the proxy above, if it asks for one. It is used when the proxy address does not already hold a password. <sub>VLC option: `http-proxy-pwd`</sub>

### Security
- **Verify certificates against the system trust store**: toggle. Default: on. Trusts the root certificates of the Certificate Authorities stored in the operating system to authenticate encrypted (TLS) connections, such as HTTPS streams. *(Needs the GnuTLS module.)* <sub>VLC option: `gnutls-system-trust`</sub>
- **Cipher preference**: dropdown: Fastest / Balanced / Secure (128-bit) / Most secure (256-bit). Default: Balanced. Which ciphers, key exchange methods and hash functions VLC prefers on encrypted connections. *Fastest* prefers the faster ciphers; *Secure (128-bit)* asks for 128-bit or stronger; *Most secure (256-bit)* leaves out the 128-bit ciphers. A stricter choice may fail with servers that do not offer it. *(Needs the GnuTLS module.)* <sub>VLC option: `gnutls-priorities` (`PERFORMANCE` / `NORMAL` / `SECURE128` / `SECURE256`)</sub>

### Advanced Options
- **Connection timeout**: number field, in ms (0 or more). Default: 5000. How long VLC waits for a network (TCP) connection to open before giving up. Raise it for slow or distant servers. <sub>VLC option: `ipv4-timeout`</sub>
- **HTTP user agent**: text field. Default: empty (VLC sends its own name and version). Overrides the application name and version VLC sends to HTTP servers (the HTTP "User-Agent"). Name and version are separated by a forward slash, for example `FooBar/1.2.3`. Some servers only answer certain players. <sub>VLC option: `http-user-agent`</sub>
- **Trusted certificates folder**: text field. Default: empty. A folder of root certificates of Certificate Authorities that VLC also trusts to authenticate encrypted (TLS) connections, for example a private authority of your company. *(Needs the GnuTLS module.)* <sub>VLC option: `gnutls-dir-trust`</sub>

### Options of VLC behind these settings

| Row | VLC option |
|---|---|
| Buffering | `file-caching`, `disc-caching`, `network-caching`, `live-caching` (the four at once) |
| Local files | `file-caching` |
| Discs | `disc-caching` |
| Network streams | `network-caching` |
| Live capture | `live-caching` |
| HTTP proxy | `http-proxy` |
| HTTP proxy password | `http-proxy-pwd` |
| Verify certificates against the system trust store | `gnutls-system-trust` |
| Cipher preference | `gnutls-priorities` |
| Connection timeout | `ipv4-timeout` |
| HTTP user agent | `http-user-agent` |
| Trusted certificates folder | `gnutls-dir-trust` |

---

← [Library](Settings-Library) · [All settings](Settings) · [Performance](Settings-Performance) →
