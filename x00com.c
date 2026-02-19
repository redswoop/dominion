#include "vars.h"
#pragma hdrstop

void dtr(int i)
/* This function sets the DTR pin to the status given */
{

    if(!ok_modem_stuff)  return;
    regs.x.dx = X00port;
    regs.h.al = i;
    regs.h.ah = 0x06;
    int86(0x14,&regs,&regs);
}

void outcomch(char ch)
/* This function outputs one character to the com port */
{

    if(!ok_modem_stuff)  return;
    regs.x.dx=X00port;
    regs.h.al=ch;
    regs.h.ah=0x01;  /* Transmit character and wait */
    int86(0x14,&regs,&regs);
}

char peek1c()
{

    if(!ok_modem_stuff)  return 0;
    regs.x.dx=X00port;
    regs.h.ah=0x0c; /* Non-Destructive Read-Ahead (peek) */
    int86(0x14,&regs,&regs);

    if (regs.h.ah==0)
        return(regs.h.al);
    else
        return(0);
}

char get1c()
/* This function returns one character from the com port, or a zero if
 * no character is waiting
 */
{
    char c1;

    if(!ok_modem_stuff)  return 0;
    c1 = 0;               /* Set default return code */
    regs.x.dx = X00port;
    regs.h.ah = 0x03;     /* Serial port status request */
    int86(0x14,&regs,&regs);
    if (regs.h.ah & 0x01)  /* Input data available */
    {
        regs.x.dx = X00port;
        regs.h.ah = 0x02;  /* Serial port read */
        int86(0x14,&regs,&regs);
        c1=regs.h.al;
    }
    return(c1);
}

int comhit()
/* This returns a value telling if there is a character waiting in the com
 * buffer.
 */
{
    if(!ok_modem_stuff) return 0;

    regs.x.dx = X00port;
    regs.h.ah = 0x03;     /* Serial port status request */
    int86(0x14,&regs,&regs);
    return(regs.h.ah & 0x01);
}

void dump()
/* This function clears the com buffer */
{

    if(!ok_modem_stuff)  return;
    regs.x.dx = X00port;
    regs.h.ah = 0x0a;     /* Purge input buffer */
    int86(0x14,&regs,&regs);
}



void set_baud(unsigned int rate)
/* This function sets the com speed to that passed */
{
    unsigned char parm;

    if(!ok_modem_stuff)  return;
    switch(rate) {
    case 300:   
        parm=0x43; 
        break;  /* 01000011 */
    case 600:   
        parm=0x63; 
        break;  /* 01100011 */
    case 1200:  
        parm=0x83; 
        break;  /* 10000011 */
    case 2400:  
        parm=0xa3; 
        break;  /* 10100011 */
    case 4800:  
        parm=0xc3; 
        break;  /* 11000011 */
    case 9600:  
        parm=0xe3; 
        break;  /* 11100011 */
    case 19200: 
        parm=0x03; 
        break;  /* 00000011 */
    case 38400: 
        parm=0x23; 
        break; 
    }/* 00100011 */

    regs.x.dx=X00port;
    regs.h.al = parm;
    regs.h.ah = 0x00;  /* Set communication parameters */
    int86(0x14,&regs,&regs);
}


void initport(int port_num)
/* This function initializes the com buffer, setting up the interrupt,
 * and com parameters
 */
{
    int temp;


    if(!ok_modem_stuff)  return;

    base=port_num;  /* Reuse variable for shrink routine */
    X00port=port_num-1;
    regs.x.dx=X00port;
    regs.h.ah=0x04;
    int86(0x14,&regs,&regs);
    if(regs.x.ax!=0x1954) {
        printf("\n\nFossil Driver not found!!!\n\n");
        err(5,"","In InitPort");
    }
    if (flow_control)  /* Turn on RTS/CTS flow control */
    {
        regs.x.dx=X00port;
        regs.h.al=0x0a;  /* 00001010 */
        regs.h.ah=0x0f;  /* Flow Control for serial I/O */
        int86(0x14,&regs,&regs);
    }
    set_baud(syscfg.baudrate[syscfg.primaryport]);
    dtr(1);
}

void closeport()
/* This function closes out the com port, removing the interrupt routine,
 * etc.
 */
{

    if(!ok_modem_stuff)  return;
    regs.x.dx=X00port;
    regs.h.ah=0x1d;  /* Deactivate Port */
    int86(0x14,&regs,&regs);

}


int cdet()
/* This returns the status of the carrier detect lead from the modem */
{

    if(!ok_modem_stuff)  return 0;
    regs.x.dx = X00port;
    regs.h.ah=0x03;  /* Return Serial Port Status */
    int86(0x14,&regs,&regs);
    return(regs.h.al & 0x80);
}
