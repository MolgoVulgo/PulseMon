# Linux backend

The backend is a Python/FastAPI service that collects Linux telemetry and exposes it through local HTTP endpoints.

## Runtime responsibilities

The backend must:

- collect CPU, memory, AMD GPU and fan metrics;
- normalize units;
- apply stable fallback rules;
- keep missing values explicit;
- maintain a current snapshot;
- maintain bounded in-memory history;
- serialize compact JSON;
- expose diagnostics without flooding logs;
- avoid heavy sensor access in HTTP handlers.

## Recommended module boundaries

A clean implementation separates:

- configuration loading;
- domain and API models;
- sensor collectors;
- normalization;
- snapshot and history stores;
- services that assemble responses;
- HTTP route handlers;
- diagnostics and raw capture tools.

## Sampling and publishing

The backend uses two runtime cadences:

- sensor acquisition, controlled by `STATS_SAMPLE_INTERVAL_S`;
- snapshot/history publication, controlled by `STATS_PUBLISH_INTERVAL_S`.

The default behavior can sample faster than it publishes, then expose display-ready values from memory.

## History

History is kept in memory with bounded capacity controlled by `STATS_HISTORY_CAPACITY`.

History responses must remain compact, aligned and bounded. The embedded client must receive arrays that can be displayed without realignment.

## Smoothing

Display smoothing can be applied to percentage values before publication. The documented active behavior uses a median window followed by EMA smoothing controlled by `STATS_DISPLAY_EMA_ALPHA`.

Raw values remain useful for diagnostics. Display values are preferred for UI stability.

## Sensor failure policy

A failure on one metric must not collapse the whole snapshot.

Rules:

- keep the field present;
- mark the metric invalid or return `null` depending on payload form;
- preserve the last coherent service state;
- log actionable diagnostics;
- avoid fabricating values.

## Authentication

Authentication is optional for private LAN use. When `STATS_API_KEY` is configured, API routes require the configured header, defaulting to `X-API-Key`.
