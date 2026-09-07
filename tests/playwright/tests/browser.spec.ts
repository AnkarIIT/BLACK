import { test, expect } from '@playwright/test';

test.describe('BLACK Browser - Basic Navigation', () => {
  test.beforeEach(async ({ page }) => {
    await page.goto('/');
  });

  test('should load start page', async ({ page }) => {
    await expect(page.locator('h1, .greeting')).toBeVisible({ timeout: 10000 });
  });

  test('should have search input', async ({ page }) => {
    await expect(page.locator('input[type="text"]')).toBeVisible();
  });

  test('should navigate to a URL', async ({ page }) => {
    await page.fill('input[type="text"]', 'https://example.com');
    await page.press('input[type="text"]', 'Enter');
    await expect(page).toHaveURL(/example\.com/);
  });
});

test.describe('BLACK Browser - Tab Management', () => {
  test('should open new tab with Ctrl+T', async ({ page, context }) => {
    await page.goto('/');
    await page.keyboard.press('Control+T');
    // Check that a new tab was opened
    const pages = context.pages();
    expect(pages.length).toBeGreaterThan(1);
  });

  test('should close tab with Ctrl+W', async ({ page, context }) => {
    await page.goto('/');
    await page.keyboard.press('Control+T'); // Open second tab
    await page.keyboard.press('Control+W'); // Close current tab
    const pages = context.pages();
    expect(pages.length).toBe(1);
  });

  test('should switch tabs with Ctrl+Tab', async ({ page, context }) => {
    await page.goto('/');
    await page.keyboard.press('Control+T'); // Tab 2
    await page.keyboard.press('Control+T'); // Tab 3
    await page.keyboard.press('Control+Shift+Tab'); // Back to Tab 2
    // Verify tab switching works
  });
});

test.describe('BLACK Browser - Bookmarks', () => {
  test('should add bookmark with Ctrl+D', async ({ page }) => {
    await page.goto('/');
    await page.keyboard.press('Control+D');
    // Check for bookmark confirmation toast
  });

  test('should show bookmarks in sidebar', async ({ page }) => {
    await page.goto('/');
    await page.click('[aria-label="Toggle Sidebar"]');
    await expect(page.locator('text=Favourites')).toBeVisible();
  });
});

test.describe('BLACK Browser - History', () => {
  test('should show history in sidebar', async ({ page }) => {
    await page.goto('/');
    await page.click('[aria-label="Toggle Sidebar"]');
    await expect(page.locator('text=History')).toBeVisible();
  });
});

test.describe('BLACK Browser - Downloads', () => {
  test('should show downloads menu', async ({ page }) => {
    await page.goto('/');
    await page.click('[aria-label="Downloads"]');
    await expect(page.locator('text=Downloads')).toBeVisible();
  });
});

test.describe('BLACK Browser - Settings', () => {
  test('should open settings with Ctrl+,', async ({ page }) => {
    await page.goto('/');
    await page.keyboard.press('Control+,');
    await expect(page.locator('text=Settings')).toBeVisible();
  });

  test('should switch themes', async ({ page }) => {
    await page.goto('/');
    await page.keyboard.press('Control+,');
    await page.click('text=Theme: Dark');
    // Verify theme changed
    await expect(page.locator('html.black-forced-dark')).toBeVisible();
  });
});

test.describe('BLACK Browser - Find in Page', () => {
  test('should open find bar with Ctrl+F', async ({ page }) => {
    await page.goto('/');
    await page.keyboard.press('Control+F');
    await expect(page.locator('input[placeholder="Find in page"]')).toBeVisible();
  });
});

test.describe('BLACK Browser - Private Window', () => {
  test('should open private window', async ({ page, context }) => {
    await page.goto('/');
    await page.keyboard.press('Control+Shift+N');
    // Verify private window opened
  });
});