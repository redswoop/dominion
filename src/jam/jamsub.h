/* jamsub.h — JAM message subsystem prototypes (reconstructed for macOS port) */
#ifndef _JAMSUB_H_
#define _JAMSUB_H_

int JamMsgInit(JAMAPIREC *pJam);
int JamMsgDeinit(JAMAPIREC *pJam);
int JamMsgAddSFldStr(JAMAPIREC *pJam, UINT16 SubFld, CHAR8 *Str, UINT32 *pSubFldPos);
int JamMsgWrite(JAMAPIREC *pJam, CHAR8 *pMsgTxt);

#endif
