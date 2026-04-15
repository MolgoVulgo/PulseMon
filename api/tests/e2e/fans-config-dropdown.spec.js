const { test, expect } = require('@playwright/test');

test('fans config dropdown keeps selected fan reference while polling runs', async ({ page }) => {
  await page.goto('/ui');

  await page.getByRole('button', { name: 'Fans' }).click();
  await page.getByRole('button', { name: 'Config' }).click();

  const refSelect = page.locator("#fans-config-list select[data-field='reference_id'][data-index='0']");
  await expect(refSelect).toBeVisible();

  const optionCount = await refSelect.locator('option').count();
  expect(optionCount).toBeGreaterThan(1);

  const targetValue = await refSelect.locator('option').nth(1).getAttribute('value');
  expect(targetValue).toBeTruthy();

  await refSelect.selectOption(targetValue);
  await expect(refSelect).toHaveValue(targetValue);

  // Wait longer than dashboard poll interval; selection must not be reset.
  await page.waitForTimeout(1500);
  await expect(refSelect).toHaveValue(targetValue);
});
