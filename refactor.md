# CLAUDE.md — Dominion BBS Refactoring Guide

## Philosophy

This is a legacy C BBS codebase (~14 years old) undergoing aggressive structural refactoring following **John Lakos's principles of physical design**: acyclic dependencies, explicit dependency tracking, single-purpose modules, and types that encapsulate behavior. We are incrementally migrating from C to C++.

**Breaking existing patterns is the point, not a risk to avoid.** This codebase is full of globals, 3-letter function names, hidden dependencies, and tangled includes. Your job is to untangle them, not preserve them.

---

## Refactoring Posture

- Do NOT preserve existing `#include` structures out of caution.
- Do NOT leave a cycle in place because "it works currently."
- Do NOT leave raw operations exposed when an abstraction exists.
- If something compiles but violates the dependency layering, it is **wrong** even if it was there before you touched it.
- You have permission to make bold structural changes. I will review them. I would rather reject an ambitious proposal than approve a timid one.

---

## Dependency Rules (Lakos-style, non-negotiable)

### Direction
- Dependencies flow strictly downward. No cycles. Ever.
- A component at level N may only depend on components at level N-1 or below.
- If you need to add a dependency that would create a cycle, **STOP and ask me.** We need to extract a new lower-level component.

### Component Structure
- Every component is a `.h`/`.cpp` pair (or `.h`/`.c` for legacy code not yet migrated) with the same basename.
- The `.h` includes ONLY what its interface requires. Forward-declare everything else.
- The `.cpp` includes its own `.h` FIRST (compile-time dependency check).
- No component may `#include` a higher-level component's header.

### Package Structure
- Each package (directory) has a single cohesive purpose stated in its README.md.
- Package-level dependencies mirror component-level: acyclic, downward only.
- Test drivers live alongside their component and depend only on that component + test infrastructure.

### Dependency Map
Consult `dependency-layers.md` in the project root before proposing any structural change. If your change would alter the layering, update the map as part of the proposal.

---

## C → C++ Migration Strategy

New code is always C++. Existing C files get migrated when we touch them for refactoring.

Migration means:
1. Rename `.c` → `.cpp`.
2. Globals that the file owns → become a class with those as members.
3. Functions operating on those globals → become methods.
4. Callers get a reference to the new object instead of touching globals.
5. Use the C++ type system: `enum class`, `std::string`, RAII wrappers, value types.
6. The C/C++ boundary is managed with `extern "C"` where needed. Keep this boundary small and shrinking.

**Do NOT do a "thin C++ wrapper over C internals."** If we're touching it, we're actually restructuring it.

### What the Migration Looks Like

```c
// BEFORE: hidden dependencies, magic numbers, global state
init_con();
g_baud = 9600;
g_mode = MODE_ANSI;
snd(MSG_WELCOME);
```

```cpp
// AFTER: every dependency is in the construction chain
Connection conn(port, BaudRate::B9600);
Terminal term(conn, DisplayMode::ANSI);
term.sendWelcome();
```

---

## Global Elimination Protocol

Every global is a dependency in disguise. When you encounter one:

1. Trace every function that reads or writes it.
2. That global is a parameter those functions are hiding. Make it explicit.
3. Group the global + its functions → that's a class waiting to be born.
4. The class owns the state. The methods take what they need via parameters or member access. The global dies.

**Do NOT** just wrap globals in a singleton. That's the same thing with extra steps.

**Do NOT** pass 12 parameters to replace 12 globals. If a function needs that many things, it's doing too much — split it first.

---

## Renaming Legacy Functions

Many functions have cryptic 2-3 letter names (`snd`, `rcv`, `dsp`, `chk`, etc.). When refactoring these:

1. Read the implementation. Determine what it **actually does**, not what the name suggests.
2. If it does multiple things, **split it first**, then name the pieces.
3. Name functions as verb phrases: `sendMessageToUser()`, `checkSessionTimeout()`.
4. If you can't name it clearly, you don't understand it yet. Ask me.

### Example

```c
// BEFORE
int snd(int t) {
    if (g_mode == 3) {
        fmt(g_buf, t);
        return tx(g_sock);
    }
    return dsp(t);
}
```

```cpp
// AFTER
class MessageRouter {
public:
    explicit MessageRouter(Connection& conn, DisplayMode mode);
    SendResult sendToRemote(MessageType type);
    DisplayResult displayLocal(MessageType type);
private:
    Connection& connection_;
    DisplayMode mode_;
    FormatBuffer buffer_;
};
```

What happened:
- `g_mode` became a member with a real type (`DisplayMode`, not `int`)
- `g_buf` became an encapsulated `FormatBuffer`
- `g_sock` became a `Connection&` passed at construction
- The branching behavior split into two named methods
- `snd(int)` became two functions whose names explain the branch

---

## Type Design

- Use value types to encapsulate behavior. No god objects.
- Prefer concrete types over inheritance hierarchies.
- If a class has more than one "reason to change," it needs to be split.
- Magic `int` values become `enum class` types.
- Raw buffers become owning types with clear semantics.
- RAII handles resource lifetime — no manual cleanup paths.

---

## Abstraction Completeness Rule

When you create or refactor an interface that encapsulates an operation (file access, database queries, protocol handling, etc.):

1. **BEFORE writing any new code**, grep the entire codebase for every place that performs the raw operation. List them all. Every single one.
2. The refactoring is **not done** until EVERY callsite goes through the new interface. No exceptions.
3. After migration, the raw operation (direct file open, raw struct walk, etc.) should be **impossible** outside the owning module. In C++, make internals private. In C, remove the header exposure.
4. **Verify**: grep again for the raw patterns. If anything still appears outside the abstraction boundary, you missed one.

**A new abstraction that doesn't have a monopoly on its operation is not an abstraction. It's a suggestion.**

---

## Refactoring Workflow

### Phase 1: Inventory (do this FIRST, always)
1. Grep/search for ALL raw usages of the thing being abstracted.
2. Present the full list with `file:line` references.
3. Identify every dependency (includes, globals read/written, functions called).
4. Identify any cycles. List them explicitly.

### Phase 2: Design
1. Propose the target dependency structure and where it sits in the layer map.
2. For each cycle, propose one of:
   - Extract shared dependency downward into a new leaf component.
   - Demote one side of the cycle to use a callback/interface.
   - Merge if they're actually one cohesive concept.
3. Show before/after dependency diagram (ASCII is fine).
4. **Wait for my approval before writing code.**

### Phase 3: Execute
1. Implement the new interface / class / module.
2. Migrate EVERY callsite from the Phase 1 inventory. Check them off.
3. Remove or hide the raw access so new code **can't** bypass the interface.
4. Grep again to confirm zero remaining raw usages.
5. Run `./check-cycles.sh` and fix any violations.

### Self-Review Before Proposing Changes
Ask yourself:
- Did I leave any cycle I could have broken? If yes, break it.
- Did I forward-declare where I could have instead of `#including`? If not, fix it.
- Is there a header including something only its `.cpp` needs? Move it.
- Did I leave a component doing two things because splitting felt "too invasive"? Split it. That's the job.
- Did I leave any raw callsites that bypass an abstraction I built or extended? Find them and migrate them.

---

## For Truly Gnarly Files

If a file is so tangled that incremental refactoring produces half-measures, use this approach:

> Rewrite this component from scratch against this interface contract: [public API]. Use the existing implementation as reference for behavior but do not try to preserve its structure. Levelize it properly from zero.

This is always an option. Don't be afraid of it.

---

## Verification

After any structural change:
```bash
./check-cycles.sh
```
Fix any violations before considering the task complete.
