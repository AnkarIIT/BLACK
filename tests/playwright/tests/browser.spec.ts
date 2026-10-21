import { test, expect, type Page } from '@playwright/test';

// Every page the browser ships in its Qt resource bundle.
const PAGES = [
  'startpage_enhanced.html',
  'bookmarks.html',
  'history.html',
  'features.html',
  'privacyreport.html',
  'safebrowsing_warning.html',
  'crash.html',
  'settings.html',
  'onboarding_experience.html',
];

// The `qrc:` WebChannel shim only exists inside QtWebEngine; outside it the
// <script src="qrc:///..."> fails to load, which Chromium reports as a console
// error. That is expected here and must not be treated as a page defect.
function collectErrors(page: Page): string[] {
  const errors: string[] = [];
  page.on('pageerror', (e) => errors.push(`pageerror: ${e.message}`));
  page.on('console', (m) => {
    if (m.type() !== 'error') return;
    const text = m.text();
    if (/qrc:|qwebchannel|Failed to load resource/i.test(text)) return;
    errors.push(`console: ${text}`);
  });
  return errors;
}

test.describe('pages parse and run cleanly', () => {
  for (const name of PAGES) {
    test(`${name} loads without script errors`, async ({ page }) => {
      const errors = collectErrors(page);
      await page.goto(`/${name}`);
      await page.waitForTimeout(300);
      expect(errors).toEqual([]);
    });
  }
});

test.describe('start page', () => {
  test('renders the greeting and the search field', async ({ page }) => {
    await page.goto('/startpage_enhanced.html');
    await expect(page.locator('#searchInput')).toBeVisible();
    await expect(page.locator('#greeting')).not.toHaveText('');
  });

  test('isSafeUrl only permits http(s)', async ({ page }) => {
    await page.goto('/startpage_enhanced.html');
    const r = await page.evaluate(() => {
      const fn = (window as any).isSafeUrl as (u: string) => boolean;
      return {
        js: fn('javascript:alert(1)'),
        data: fn('data:text/html,<b>x</b>'),
        file: fn('file:///etc/passwd'),
        vbscript: fn('vbscript:msgbox(1)'),
        http: fn('http://example.com'),
        https: fn('https://example.com'),
        bare: fn('example.com'),
      };
    });
    expect(r.js).toBe(false);
    expect(r.data).toBe(false);
    expect(r.file).toBe(false);
    expect(r.vbscript).toBe(false);
    expect(r.http).toBe(true);
    expect(r.https).toBe(true);
    expect(r.bare).toBe(false); // bare hostnames are not navigable as-is
  });

  test('normalizeUrl upgrades bare domains to https', async ({ page }) => {
    await page.goto('/startpage_enhanced.html');
    const out = await page.evaluate(() => {
      const fn = (window as any).normalizeUrl as (u: string) => string;
      return {
        bare: fn('example.com'),
        withPath: fn('example.com/a?b=1'),
      };
    });
    expect(out.bare).toBe('https://example.com');
    expect(out.withPath).toBe('https://example.com/a?b=1');
  });

  test('a javascript: entry is searched, never navigated to', async ({ page }) => {
    await page.goto('/startpage_enhanced.html');

    // Capture every off-origin navigation attempt instead of performing it, so
    // the test needs no network and cannot race a real page load.
    const attempted: string[] = [];
    await page.route('**/*', (route) => {
      const url = route.request().url();
      if (!url.startsWith('http://127.0.0.1:8080')) {
        attempted.push(url);
        return route.abort();
      }
      return route.continue();
    });

    await page.fill('#searchInput', 'javascript:alert(1)');
    await page.press('#searchInput', 'Enter');
    await page.waitForTimeout(500);

    expect(attempted.length).toBeGreaterThan(0);
    for (const url of attempted) {
      expect(url).not.toMatch(/^javascript:/i);
    }
    expect(attempted.some((u) => /^https?:\/\//.test(u))).toBe(true);
  });

  test('malicious bookmark titles/urls are rendered inert (no XSS)', async ({ page }) => {
    // Provide a WebChannel bridge whose bookmark list contains javascript: and
    // data: URLs plus an HTML-injection title, then assert the start page both
    // drops the unsafe URLs and renders the title as text.
    await page.addInitScript(() => {
      const noop = { connect() {} };
      const theme = { themeScheme: 0, themeSchemeChanged: noop };
      const browserSettings = { searchEngine: 'DuckDuckGo', settingsChanged: noop };

      const payloads = [
        { url: 'javascript:alert(1)', title: 'xss-js' },
        { url: 'data:text/html,<script>alert(2)</scr' + 'ipt>', title: 'xss-data' },
        { url: 'https://example.com', title: '<img src=x onerror="window.__xss=1">' },
      ];
      const bookmarks = {
        json(cb: (s: string) => void) { cb(JSON.stringify(payloads)); },
        changed: noop,
      };

      function magic(): any {
        const fn: any = () => magic();
        return new Proxy(fn, {
          get(_t, prop) {
            if (prop === 'connect' || prop === 'disconnect') return () => {};
            if (prop === Symbol.toPrimitive) return () => 0;
            if (prop === 'toString') return () => '';
            if (prop === 'then') return undefined;
            if (prop === 'length') return 0;
            return magic();
          },
          apply() { return magic(); },
        });
      }

      const known: Record<string, unknown> = { theme, browserSettings, bookmarks };
      const objects = new Proxy(known, {
        get: (t, prop) => (prop in t ? (t as any)[prop] : magic()),
      });

      (window as any).qt = { webChannelTransport: {} };
      (window as any).QWebChannel = function (_transport: unknown, cb: (c: unknown) => void) {
        cb({ objects });
      };
    });

    await page.goto('/startpage_enhanced.html');

    // Only the safe https:// bookmark survives.
    await expect(page.locator('#favoritesGrid .favorite-item')).toHaveCount(1);
    // ...and its hostile title is present as text, not as markup.
    await expect(page.locator('#favoritesGrid .favorite-name'))
      .toHaveText('<img src=x onerror="window.__xss=1">');
    expect(await page.evaluate(() => (window as any).__xss)).toBeUndefined();
  });
});

test.describe('crash page', () => {
  test('renders the ?url= parameter as inert text', async ({ page }) => {
    const payload = '<img src=x onerror="window.__xss=1">';
    await page.goto(`/crash.html?url=${encodeURIComponent(payload)}`);
    await expect(page.locator('#urlBox')).toHaveText(payload);
    expect(await page.evaluate(() => (window as any).__xss)).toBeUndefined();
  });

  test('survives a malformed (non-decodable) ?url=', async ({ page }) => {
    const errors = collectErrors(page);
    await page.goto('/crash.html?url=%E0%A4%A');
    await expect(page.locator('#urlBox')).toBeVisible();
    expect(errors).toEqual([]);
  });
});

test.describe('settings page', () => {
  test('its bridge callback body runs without throwing', async ({ page }) => {
    // The settings script does all of its work inside the QWebChannel onready
    // callback. Without a bridge that body never executes, so a mock is the
    // only way to cover it. A duplicated `const` or a dereferenced missing
    // element (both of which have shipped here before) surfaces as a pageerror.
    await page.addInitScript(() => {
      function magic(): any {
        const fn: any = () => magic();
        return new Proxy(fn, {
          get(_t, prop) {
            if (prop === 'connect' || prop === 'disconnect') return () => {};
            if (prop === Symbol.toPrimitive) return () => 0;
            if (prop === 'toString') return () => '';
            if (prop === 'then') return undefined;
            if (prop === 'length') return 0;
            return magic();
          },
          apply(_t, _this, args) {
            for (const a of args) if (typeof a === 'function') { try { a(magic()); } catch {} }
            return magic();
          },
        });
      }
      const objects = new Proxy({}, { get: (_t, prop) => magic() });
      (window as any).qt = { webChannelTransport: {} };
      (window as any).QWebChannel = function (_transport: unknown, cb: (c: unknown) => void) {
        cb({ objects });
      };
    });

    const errors = collectErrors(page);
    await page.goto('/settings.html');
    await page.waitForTimeout(500);
    expect(errors).toEqual([]);
  });
});
