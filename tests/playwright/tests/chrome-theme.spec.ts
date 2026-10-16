import { test, expect } from '@playwright/test';
import * as fs from 'fs';
import * as path from 'path';

// Static regression guard: every CSS custom property referenced by a bundled
// page must have a Chrome-mode override, otherwise Chrome layout silently
// inherits the Safari palette for that token (e.g. invisible scroll chevrons,
// wrong hover/border colors).
//
// This mirrors tests/settings-api.spec.ts: no browser needed, fails the build
// the moment a page starts using a token chrome-theme.css doesn't retoken.

const ROOT = path.resolve(__dirname, '..', '..', '..');

const PAGES = [
  'settings.html',
  'startpage_enhanced.html',
  'bookmarks.html',
  'history.html',
  'privacyreport.html',
  'features.html',
  'crash.html',
  'onboarding_experience.html',
];

function usedVars(html: string): Set<string> {
  const out = new Set<string>();
  for (const m of html.matchAll(/var\(\s*(--[\w-]+)/g)) out.add(m[1]);
  return out;
}

function chromeDefinedVars(css: string): Set<string> {
  const out = new Set<string>();
  // Only blocks gated on the chrome layout attribute count as overrides.
  for (const m of css.matchAll(/([^{}]+)\{([^{}]*)\}/g)) {
    if (!m[1].includes('data-ui-layout')) continue;
    for (const p of m[2].matchAll(/(--[\w-]+)\s*:/g)) out.add(p[1]);
  }
  return out;
}

test.describe('chrome layout theme contract', () => {
  test('every page var() has a chrome-mode override', () => {
    const needed = new Set<string>();
    for (const page of PAGES) {
      const html = fs.readFileSync(path.join(ROOT, page), 'utf8');
      for (const v of usedVars(html)) needed.add(v);
    }
    const css = fs.readFileSync(path.join(ROOT, 'chrome-theme.css'), 'utf8');
    const defined = chromeDefinedVars(css);
    const missing = [...needed].filter((v) => !defined.has(v)).sort();
    expect(missing).toEqual([]);
  });
});
