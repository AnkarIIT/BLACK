import { defineConfig, devices } from '@playwright/test';

// These tests exercise the browser's HTML/CSS/JS front-end (the start page,
// settings sheet, crash page, ...) in a real browser engine. They deliberately
// do NOT try to drive the Qt shell: Playwright can only automate its own
// bundled Chromium, never QtWebEngine.
export default defineConfig({
  testDir: './tests',
  outputDir: './test-results',
  fullyParallel: true,
  forbidOnly: !!process.env.CI,
  retries: process.env.CI ? 2 : 0,
  workers: process.env.CI ? 1 : undefined,
  reporter: process.env.CI
    ? [['list'], ['html', { open: 'never' }]]
    : [['list']],
  use: {
    baseURL: 'http://127.0.0.1:8080',
    trace: 'on-first-retry',
  },
  projects: [
    {
      name: 'chromium',
      use: { ...devices['Desktop Chrome'] },
    },
  ],
  webServer: {
    command: 'node serve.mjs',
    url: 'http://127.0.0.1:8080/startpage_enhanced.html',
    reuseExistingServer: !process.env.CI,
    timeout: 30_000,
  },
});
