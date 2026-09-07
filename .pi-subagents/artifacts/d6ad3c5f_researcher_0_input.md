# Task for researcher

Audit the browser project at C:/Codes/BLACK for flaws and readiness to become an industry-level private browser for Windows. 

Working directory: C:/Codes/BLACK

Evidence to gather:
1. Architecture & build: Read CMakeLists.txt, BrowserWindow.cpp/h, SafariWebView.cpp/h, main.cpp. Identify Qt modules, threading model, process model, GPU acceleration flags, and build dependencies.
2. Core browser features: Inspect navigation, tab management, back/forward, session restore, crash recovery. Look for stubs, TODOs, missing implementations.
3. Privacy/security: Read TrackerBlocker.cpp/h, SafeBrowsing.cpp/h, VaultCrypto.cpp/h, PasswordStore.cpp/h, ExtensionManager.cpp/h. Check what is actually implemented vs. what is claimed in README or HTML pages.
4. Responsive UI: Read PlatformAdaptor.cpp/h, responsive.css, and a sample of HTML pages (settings.html, bookmarks.html, startpage_enhanced.html). Check if breakpoints, touch targets, and safe-area support are complete.
5. Performance/smoothness: Look for async loading, caching, memory management, render loop hints, network-thread separation.
6. Windows experience: Inspect installer scripts (installer.iss, deploy.ps1), high-DPI handling, touch support, Windows-specific code.
7. Documentation/claims: Read README.md and report.txt. List every explicit or implied claim about being private, fast, or industry-ready.

Return a structured report with:
- Executive summary
- Architecture findings
- Feature completeness matrix
- Privacy/security gap list
- UI/ responsiveness status
- Performance risks
- Windows-specific gaps
- Missing code / stubs / TODOs
- Reproduction risks
- Sources (file paths with line references where possible)

Be thorough. Do not smooth over missing implementations. Distinguish between "implemented", "partially implemented", "stub/placeholder", and "missing".

---
**Output:**
Write your findings to exactly this path: C:\Codes\BLACK\.pi-subagents\artifacts\outputs\d6ad3c5f\research.md
This path is authoritative for this run.
Ignore any other output filename or output path mentioned elsewhere, including output destinations in the base agent prompt, system prompt, or task instructions.

## Acceptance Contract
Acceptance level: attested
Completion is not accepted from prose alone. End with a structured acceptance report.

Criteria:
- criterion-1: Return concrete findings with file paths and severity when applicable

Required evidence: review-findings, residual-risks

Finish with a fenced JSON block tagged `acceptance-report` in this shape:
Use empty arrays when no items apply; array fields contain strings unless object entries are shown.
`criteriaSatisfied[].status` must be exactly one of: satisfied, not-satisfied, not-applicable.
`commandsRun[].result` must be exactly one of: passed, failed, not-run.
`manualNotes` and `notes` are optional strings; an empty string means no note and does not satisfy `manual-notes` evidence.
```acceptance-report
{
  "criteriaSatisfied": [
    {
      "id": "criterion-1",
      "status": "satisfied",
      "evidence": "specific proof"
    }
  ],
  "changedFiles": [
    "src/file.ts"
  ],
  "testsAddedOrUpdated": [
    "test/file.test.ts"
  ],
  "commandsRun": [
    {
      "command": "command",
      "result": "passed",
      "summary": "short result"
    }
  ],
  "validationOutput": [
    "validation output or concise summary"
  ],
  "residualRisks": [
    "none"
  ],
  "noStagedFiles": true,
  "diffSummary": "short description of the diff",
  "reviewFindings": [
    "blocker: file.ts:12 - issue found, or no blockers"
  ],
  "manualNotes": "anything else the parent should know"
}
```