# Builder Agent Notes

This file contains rules for the agent when scoped to the `builder/` directory.

## Scope and Rules
- `builder/` is a standalone program.
- When working within `builder/`, the agent must adhere to this file and its specific rules.
- The agent is allowed to scan and modify files in `builder/` only when explicitly scoped to do so.
- When explicitly scoped to `builder/`, the constraints in the root `AGENTS.md` against scanning this directory are temporarily lifted for the context of that specific task.

## Future Development: Linux Platform Access
- We will be working on Linux platform access in the future.
- The Linux platform access will be built using `builder/`.
- Project directories like `linux/` and `system/linux/` will be used for this purpose. (Note: Do not create these folders now, this is just a note for future reference).
