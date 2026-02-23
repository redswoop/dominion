/*
 * mci.cpp — MCI expansion engine (Layer 1)
 *
 * Leaf module: no vars.h, no BBS globals.
 * Uses std::string internally, C-string API at boundary.
 */

#include "mci.h"
#include <string>
#include <unordered_map>
#include <cstring>
#include <cassert>

/* --- Resolver state --- */
static mci_resolve_fn s_resolver = nullptr;
static void *s_resolver_ctx = nullptr;

/* --- Named code registry: ${name} → backtick char --- */
static std::unordered_map<std::string, char> s_name_map;

/* --- Top-ten iterator state (was globals in bbsutl.c) --- */
static int s_topten_type = 0;
static int s_topten_which = 0;


/* ================================================================
 *  Registration
 * ================================================================ */

void mci_set_resolver(mci_resolve_fn fn, void *ctx)
{
    s_resolver = fn;
    s_resolver_ctx = ctx;
}

void mci_register_name(const char *name, char code)
{
    s_name_map[std::string(name)] = code;
}


/* ================================================================
 *  Top-ten state
 * ================================================================ */

void mci_topten_set_type(int type)
{
    s_topten_type = type;
    s_topten_which = 0;
}

void mci_topten_advance(void)
{
    s_topten_which++;
    if (s_topten_which >= 10)
        s_topten_which = 0;
}

int mci_topten_type(void)  { return s_topten_type; }
int mci_topten_which(void) { return s_topten_which; }


/* ================================================================
 *  Internal: resolve one code via callback
 * ================================================================ */

static bool resolve_code(char code, char *buf, int bufsize)
{
    if (!s_resolver)
        return false;
    return s_resolver(code, buf, bufsize, s_resolver_ctx);
}


/* ================================================================
 *  mci_resolve — single-code convenience wrapper
 * ================================================================ */

bool mci_resolve(char code, char *buf, int bufsize)
{
    return resolve_code(code, buf, bufsize);
}


/* ================================================================
 *  mci_expand — full-string expansion
 * ================================================================ */

int mci_expand(const char *input, char *output, int output_size)
{
    std::string result;
    result.reserve(256);

    int i = 0;
    while (input[i]) {

        /* --- ${name} syntax --- */
        if (input[i] == '$' && input[i + 1] == '{') {
            int start = i + 2;
            int end = start;
            while (input[end] && input[end] != '}')
                end++;
            if (input[end] == '}') {
                std::string name(input + start, end - start);
                auto it = s_name_map.find(name);
                if (it != s_name_map.end()) {
                    char buf[161];
                    if (resolve_code(it->second, buf, sizeof(buf))) {
                        result += buf;
                    } else {
                        /* Side-effect: leave as backtick sequence */
                        result += '`';
                        result += it->second;
                    }
                } else {
                    /* Unknown name: pass through literally */
                    result.append(input + i, end - i + 1);
                }
                i = end + 1;
                continue;
            }
            /* No closing brace: fall through to normal char */
        }

        /* --- Backtick code --- */
        if (input[i] == '`' && input[i + 1]) {
            char code = input[i + 1];
            char buf[161];
            if (resolve_code(code, buf, sizeof(buf))) {
                result += buf;
            } else {
                /* Side-effect or unknown: preserve backtick sequence */
                result += '`';
                result += code;
            }
            i += 2;
            continue;
        }

        /* --- Color codes: pass through untouched --- */
        if (((unsigned char)input[i] == 3 || (unsigned char)input[i] == 14)
            && input[i + 1]) {
            result += input[i];
            result += input[i + 1];
            i += 2;
            continue;
        }

        /* --- Normal character --- */
        result += input[i++];
    }

    /* Copy to output buffer — assert on overflow */
    int len = (int)result.size();
    assert(len < output_size && "mci_expand: result exceeds output buffer");
    std::memcpy(output, result.c_str(), len);
    output[len] = '\0';
    return len;
}


/* ================================================================
 *  mci_strlen — visible width after expansion
 * ================================================================ */

int mci_strlen(const char *s)
{
    int len = 0;
    int i = 0;

    while (s[i]) {

        /* Color escapes: 2-byte, zero visible width */
        if (((unsigned char)s[i] == 3 || (unsigned char)s[i] == 14)
            && s[i + 1]) {
            i += 2;
            continue;
        }

        /* Backtick MCI: expand and measure */
        if (s[i] == '`' && s[i + 1]) {
            char buf[161];
            if (resolve_code(s[i + 1], buf, sizeof(buf)))
                len += (int)std::strlen(buf);
            /* Side-effect codes contribute zero width */
            i += 2;
            continue;
        }

        /* ${name}: expand and measure */
        if (s[i] == '$' && s[i + 1] == '{') {
            int start = i + 2;
            int end = start;
            while (s[end] && s[end] != '}')
                end++;
            if (s[end] == '}') {
                std::string name(s + start, end - start);
                auto it = s_name_map.find(name);
                if (it != s_name_map.end()) {
                    char buf[161];
                    if (resolve_code(it->second, buf, sizeof(buf)))
                        len += (int)std::strlen(buf);
                }
                i = end + 1;
                continue;
            }
        }

        /* Normal visible character */
        len++;
        i++;
    }

    return len;
}
