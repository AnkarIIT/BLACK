Subagent run failed before producing output.

Error:
Agent 'researcher' requested unavailable child tools: hf_dataset_info, hf_repo_files, hf_repo_read_file.
The `tools` field is a strict allowlist; it does not load extension code.
For extension tools, add the provider path to `subagentOnlyExtensions` (child-only), `extensions`, or as a path-like entry in `tools`, while keeping each registered tool name in `tools`.
For MCP tools, verify the MCP adapter configuration and selected tool names. For builtin tools, verify the name against the installed Pi version.

Transcript: C:\Codes\BLACK\.pi-subagents\artifacts\d6ad3c5f_researcher_0_transcript.jsonl
Metadata: C:\Codes\BLACK\.pi-subagents\artifacts\d6ad3c5f_researcher_0_meta.json