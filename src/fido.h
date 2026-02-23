#ifndef _FIDO_H_
#define _FIDO_H_

/* FidoNet address — embedded in subboardrec (SUBS.DAT) and hdrinfo.
 * Must keep exact layout for binary file compatibility. */
typedef struct {
        int zone,net,node,point;
} addressrec;

/* Origin record — read from origin.dat by getorigin() in msgbase.c.
 * Must keep exact layout for binary file compatibility. */
typedef struct {
        char origin[MAX_PATH_LEN];
        char tear[MAX_PATH_LEN];
        char netname[41];
        addressrec add;
} originrec;

#endif /* _FIDO_H_ */
