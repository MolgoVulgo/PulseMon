// @ts-check
const { defineConfig } = require('@playwright/test');

module.exports = defineConfig({
  testDir: './tests/e2e',
  fullyParallel: false,
  workers: 1,
  timeout: 30000,
  retries: 0,
  use: {
    baseURL: 'http://127.0.0.1:18000',
    headless: true,
  },
  webServer: {
    command: '.venv/bin/python -m uvicorn app.main:app --host 127.0.0.1 --port 18000',
    url: 'http://127.0.0.1:18000/ui',
    reuseExistingServer: true,
    timeout: 120000,
  },
});
