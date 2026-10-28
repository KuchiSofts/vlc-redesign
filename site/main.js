// VLC Redesign's website: offers the download for the visitor's system, from
// platforms.json (written by extras/juxtopposed/platforms/render.py from
// platforms.yml). Without JavaScript, or if the file cannot be read, the page
// keeps the Windows download it was rendered with. Nothing is stored or sent.
(function () {
  'use strict';

  function detect() {
    // ?os=macos shows the page as on that system (to check each one)
    var forced = /[?&]os=([a-z0-9-]+)/.exec(location.search);
    if (forced) return Promise.resolve(forced[1]);
    var ua = navigator.userAgent || '';
    var hint = (navigator.userAgentData && navigator.userAgentData.platform) || '';
    var os;
    if (/Tizen/i.test(ua)) os = 'tizen';
    else if (/Android/i.test(ua) || hint === 'Android') os = 'android';
    else if (/iPhone|iPad|iPod/i.test(ua)) os = 'ios';
    // an iPad asks for the desktop site and says it is a Mac: it has touch
    else if (/Macintosh|Mac OS X/i.test(ua) || hint === 'macOS')
      os = navigator.maxTouchPoints > 1 ? 'ios' : 'macos';
    else if (/Windows/i.test(ua) || hint === 'Windows')
      os = /ARM64|aarch64/i.test(ua) ? 'windows-arm64' : 'windows';
    else if (/CrOS/i.test(ua) || hint === 'Chrome OS') os = 'linux';
    else if (/Linux|X11|BSD/i.test(ua) || hint === 'Linux') os = 'linux';
    else os = '';
    // Windows on ARM tells its architecture only through the client hints
    if (os === 'windows' && navigator.userAgentData && navigator.userAgentData.getHighEntropyValues) {
      return navigator.userAgentData.getHighEntropyValues(['architecture']).then(function (v) {
        return v && v.architecture === 'arm' ? 'windows-arm64' : 'windows';
      }, function () { return os; });
    }
    return Promise.resolve(os);
  }

  function el(tag, attrs, text) {
    var e = document.createElement(tag);
    for (var k in attrs) if (Object.prototype.hasOwnProperty.call(attrs, k)) e.setAttribute(k, attrs[k]);
    if (text) e.textContent = text;
    return e;
  }

  var MONTHS = ['January', 'February', 'March', 'April', 'May', 'June', 'July', 'August', 'September',
                'October', 'November', 'December'];
  function longDate(iso) {
    var p = iso.split('-');
    return Number(p[2]) + ' ' + MONTHS[Number(p[1]) - 1] + ' ' + p[0];
  }

  function meta(p) {
    var parts = ['Version ' + p.version];
    if (p.date) parts.push(longDate(p.date));
    if (p.requires) parts.push(p.requires);
    return parts.join(' · ');
  }

  function render(data, os) {
    var list = data.platforms || [];
    var mine = null, released = null;
    for (var i = 0; i < list.length; i++) {
      if (!mine && list[i].os === os) mine = list[i];
      if (!released && list[i].available && list[i].downloads.length) released = list[i];
    }
    if (!mine || !released) return; // an unknown system: the page as rendered

    var card = document.getElementById('platform-' + mine.id);
    if (card) card.classList.add('current');

    var title = document.getElementById('download-title');
    var buttons = document.getElementById('download-buttons');
    var info = document.getElementById('download-meta');
    if (!title || !buttons || !info) return;
    buttons.textContent = '';

    if (mine.available && mine.downloads.length) {
      title.textContent = 'VLC Redesign for ' + mine.name;
      mine.downloads.forEach(function (d, n) {
        buttons.appendChild(el('a', { 'class': 'button large' + (n === 0 ? ' accent' : ''), href: d.url },
                               n === 0 ? 'Download for ' + mine.short : d.label));
      });
      info.textContent = meta(mine);
      return;
    }

    // Not out yet here: say so, link the progress, and offer what exists
    title.textContent = 'VLC Redesign for ' + mine.name + ' is ' +
      (mine.status === 'later' ? 'planned for later' : 'coming soon');
    if (mine.page) buttons.appendChild(el('a', { 'class': 'button accent large', href: mine.page }, 'Follow the progress'));
    var other = released.downloads[0];
    buttons.appendChild(el('a', { 'class': 'button large', href: other.url }, 'Download for ' + released.short));
    var note = mine.id === 'windows-arm64'
      ? 'The Windows x64 version runs on Windows 11 ARM64 under emulation.'
      : (released.short + ' ' + released.version + ' is out now' + (mine.notes ? '. ' + mine.name + ': ' + mine.notes.charAt(0).toLowerCase() + mine.notes.slice(1) + '.' : '.'));
    info.textContent = note;
  }

  function start() {
    if (!window.fetch) return;
    Promise.all([
      fetch('platforms.json', { credentials: 'omit' }).then(function (r) {
        if (!r.ok) throw new Error(r.status);
        return r.json();
      }),
      detect()
    ]).then(function (v) { render(v[0], v[1]); }, function () { /* keep the rendered page */ });
  }

  if (document.readyState === 'loading') document.addEventListener('DOMContentLoaded', start);
  else start();
}());
