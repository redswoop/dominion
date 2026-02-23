/*
 * mci.h — MCI expansion engine (Layer 1)
 *
 * Pure string substitution module.  No BBS globals, no vars.h.
 * Dependency injection via resolver callback registered at startup.
 *
 * Backtick codes (`N, `T, etc.) and ${name} codes (${username}, etc.)
 * are both resolved through the same callback.  Color codes (char 3,
 * char 14, pipe |) pass through untouched.
 *
 * All callers are C++ (-x c++).  No extern "C" needed.
 * C-string API at the boundary (callers use char[]), std::string inside.
 * Asserts on buffer overflow — truncation would hide bugs.
 */

#ifndef MCI_H_
#define MCI_H_

/*
 * Resolver callback type.
 *
 * Called for each backtick code character during expansion.
 * Returns true if code was resolved — result written to buf.
 * Returns false if code is a side-effect or unknown — left as-is.
 *
 * buf is guaranteed at least bufsize bytes.  Resolver must NUL-terminate.
 * ctx: opaque pointer (NULL today, session_t* in the future).
 */
typedef bool (*mci_resolve_fn)(char code, char *buf, int bufsize, void *ctx);

/* Register the resolver callback (once at startup). */
void mci_set_resolver(mci_resolve_fn fn, void *ctx);

/*
 * Expand all resolvable MCI codes in a string.
 *
 * Pure data codes: replaced with resolved values.
 * Side-effect codes (resolver returns false): left as backtick sequences.
 * ${name} codes: mapped to backtick code, then resolved.
 * Color codes: pass through untouched.
 *
 * Asserts if expanded result exceeds output_size.
 * Returns number of chars written (excluding NUL).
 */
int mci_expand(const char *input, char *output, int output_size);

/*
 * Visible character count of a string after MCI expansion.
 *
 * Skips color codes (char 3 + byte, char 14 + byte).
 * Expands backtick/$ codes and counts their resolved length.
 * Side-effect codes contribute zero width.
 *
 * Replaces strlenc() in bbs_output.c.
 */
int mci_strlen(const char *s);

/*
 * Register a ${name} -> backtick code mapping.
 * e.g. mci_register_name("username", 'N') means ${username} -> `N.
 */
void mci_register_name(const char *name, char code);

/*
 * Resolve a single backtick code via the registered resolver.
 * Convenience wrapper used by setmci() in bbsutl.c.
 * Returns true if resolved (buf filled), false for side-effect/unknown.
 */
bool mci_resolve(char code, char *buf, int bufsize);

/* --- Top-ten iterator state (encapsulated) --- */
void mci_topten_set_type(int type);
void mci_topten_advance(void);
int  mci_topten_type(void);
int  mci_topten_which(void);

#endif /* MCI_H_ */
