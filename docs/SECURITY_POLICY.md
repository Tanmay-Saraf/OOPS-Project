# Security & Defense v1 policy

## Request boundary

`SecurityEngine::processRequest` accepts the parsed canonical `oops::Request`. It does not parse client text, use sockets, send network responses, or execute database operations. Security decisions are approvals/denials only; an allowed request still may fail in the Memory Engine.

## Command handling

- `GET`, `EXISTS`, `KEYS`, `HISTORY`, `GET_VERSION`, and `STATUS` are permitted while the state is not `LOCKDOWN`.
- During `LOCKDOWN`, `GET` is the only command approved by the security engine.
- `SET` and `DELETE` are analyzed by the canary, modification-activity, and entropy detectors as applicable.
- `SAVE`, `RECOVER`, and `INVALID` are denied because privileged authorization is not implemented.
- `PING` and `QUIT` are protocol-only commands and are denied by this engine. Network Guard should handle them itself and must not forward them to Memory Engine.
- Unknown enum values are denied by the default branch.

## State transitions

- Initial state is `RUNNING`.
- A canary modification/deletion transitions to `LOCKDOWN`.
- A modification-rate or unique-key-rate alert transitions to `SUSPECTED` unless combined with a high-entropy value, in which case it transitions to `LOCKDOWN`.
- `SUSPECTED` and `LOCKDOWN` block writes. `LOCKDOWN` allows only `GET` through this engine.
- State transitions are monotonic. There is no client-facing unlock/recovery method; leaving `LOCKDOWN` requires a future trusted recovery API.

## Decision reasons and logging

- `SecurityDecision.reason` uses caller-safe descriptions and does not expose the detailed internal detector explanation.
- Detailed detection reasons are written to the internal security log.
- Log initialization failure causes engine construction to fail.
- If logging fails while processing a canary/rate alert, the request remains denied and any state transition remains in effect.
- Entropy-only alerts are allowed only if the alert is successfully logged; otherwise the current write is denied.
- Ordinary policy denials are still denied if the denial log write fails.

## Detector defaults

- Modification burst: 100 `SET`/`DELETE` requests within one second.
- Overall modification volume: 1,000 `SET`/`DELETE` requests within one minute.
- Distinct modified keys: 100 non-empty keys within one minute.
- Entropy analysis: minimum value length 16 and threshold 4.2 bits per character.

These defaults are initial policy values, not empirically calibrated production thresholds. The modification detector's limits are configurable through its constructor.

## Security limitations

No trusted client authentication/authorization or privileged recovery mechanism is implemented in this module. Do not permit a client to unlock the system by issuing a command string. The security engine does not itself guarantee database persistence, rollback, or ransomware recovery.
