import { test, expect } from '@playwright/test';
import * as fs from 'fs';
import * as path from 'path';

// Strict, browser-free regression guard for the QWebChannel contract between
// settings.html and the C++ bridge.
//
// The behavioural Playwright spec uses an "everything is callable" Proxy, which
// unconditionally succeeds and therefore CANNOT catch a bridge mismatch - that
// is exactly how `passwords.decryptFailed(...)` (a Q_PROPERTY called as a
// method) shipped and silently aborted the whole settings callback. This spec
// instead compares every `channel.objects.X.member` usage against the real
// Q_INVOKABLE / public-slot / Q_PROPERTY / signal surface parsed from the
// headers, so an invented or misused member fails the build.

const ROOT = path.resolve(__dirname, '..', '..', '..');

// channel object name -> header that declares its class
const OBJECT_HEADER: Record<string, string | null> = {
  browserSettings: 'BrowserSettings.h',
  theme: 'SafariTheme.h',
  passwords: 'PasswordStore.h',
  profileManager: 'ProfileManager.h',
  account: 'Account.h',
  extensions: 'ExtensionManager.h',
  permissions: 'PermissionsBridge.h',
  webauthn: 'WebAuthn.h',
  updater: 'UpdateChecker.h',
  syncManager: 'SyncManager.h',
  appearance: 'AppearanceManager.h',
  settingsDialog: null, // inner class defined in BrowserWindow.cpp
};

// SyncManager is a documented no-op stub. The sync pane probes it with the
// richer API it was originally written against, but only behind `syncAvailable`
// truthiness checks, so those members are allowed to be absent.
const GUARDED = new Set([
  'getStats', 'getEnabledDataTypes', 'setDataTypeEnabled', 'signIn', 'syncNow',
  'getPendingConflicts', 'resolveConflict', 'getDevices', 'getDeviceId',
  'revokeDevice', 'exportBackup', 'importBackup', 'setConflictStrategy',
  'getSyncEngine',
]);

interface Api {
  methods: Set<string>;
  props: Set<string>;
  signals: Set<string>;
}

function parseHeader(src: string): Api {
  const methods = new Set<string>();
  const props = new Set<string>();
  const signals = new Set<string>();

  for (const m of src.matchAll(/Q_INVOKABLE\s+([^;]*?)\(/g)) {
    const ids = m[1].match(/[A-Za-z_]\w*/g);
    if (ids && ids.length) methods.add(ids[ids.length - 1]);
  }
  for (const m of src.matchAll(/Q_PROPERTY\(\s*\S+\s+(\w+)/g)) props.add(m[1]);

  for (const kw of ['signals\\s*:', 'public\\s+slots\\s*:']) {
    const re = new RegExp(
      kw + '([\\s\\S]*?)(?:\\n\\s*(?:private|public|protected|signals)\\s*:|\\n\\s*\\};)',
      'g',
    );
    for (const s of src.matchAll(re)) {
      const isSignal = kw.startsWith('signals');
      for (const mm of s[1].matchAll(/\b(\w+)\s*\(/g)) {
        if (isSignal) signals.add(mm[1]);
        else methods.add(mm[1]);
      }
    }
  }
  return { methods, props, signals };
}

function buildApi(): Record<string, Api> {
  const api: Record<string, Api> = {};
  for (const [obj, file] of Object.entries(OBJECT_HEADER)) {
    if (!file) continue;
    api[obj] = parseHeader(fs.readFileSync(path.join(ROOT, file), 'utf8'));
  }
  api.settingsDialog = {
    methods: new Set([
      'currentPageUrl', 'closeSettingsDialog', 'hideSettingsDialog',
      'toggleSettingsMaximize',
    ]),
    props: new Set(),
    signals: new Set(),
  };
  return api;
}

function audit(html: string): string[] {
  const api = buildApi();
  const scripts = [...html.matchAll(/<script(?![^>]*\bsrc=)[^>]*>([\s\S]*?)<\/script>/g)]
    .map((m) => m[1])
    .join('\n');
  // strip comments so prose such as "passwords.decryptFailed(cb) threw" is not
  // mistaken for a real call
  const js = scripts
    .replace(/\/\*[\s\S]*?\*\//g, '')
    .replace(/(^|[^:])\/\/[^\n]*/g, '$1');

  const varToObj: Record<string, string> = {};
  for (const m of js.matchAll(/const\s+(\w+)\s*=\s*channel\.objects\.(\w+)\s*;/g)) {
    varToObj[m[1]] = m[2];
  }

  const problems: string[] = [];
  for (const m of js.matchAll(/\b(\w+)\.(\w+)\s*(\(?)/g)) {
    const [, objVar, member, paren] = m;
    const obj = varToObj[objVar];
    if (!obj) continue;
    const a = api[obj];
    if (!a) continue;

    const isMethod = a.methods.has(member);
    const isProp = a.props.has(member);
    const isSignal = a.signals.has(member);

    if (!isMethod && !isProp && !isSignal) {
      if (!GUARDED.has(member)) {
        problems.push(`${objVar}.${member} - ${obj} has no such member`);
      }
    } else if (isProp && paren) {
      problems.push(`${objVar}.${member}(...) - '${member}' is a PROPERTY, called as a function`);
    } else if (isSignal && paren) {
      problems.push(`${objVar}.${member}(...) - '${member}' is a SIGNAL, called as a function`);
    }
  }
  return [...new Set(problems)];
}

test.describe('settings.html <-> C++ bridge contract', () => {
  test('every channel object member used exists with the right kind', () => {
    const html = fs.readFileSync(path.join(ROOT, 'settings.html'), 'utf8');
    expect(audit(html)).toEqual([]);
  });
});
