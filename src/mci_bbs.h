/*
 * mci_bbs.h — BBS-specific MCI resolver
 *
 * Registers the BBS resolver callback and ${name} mappings.
 * Call mci_bbs_init() once at startup after io_init().
 */

#ifndef MCI_BBS_H_
#define MCI_BBS_H_

void mci_bbs_init(void);

#endif /* MCI_BBS_H_ */
