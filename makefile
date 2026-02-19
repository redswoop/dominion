TC  = c:\bc
OBJ = obj\\
EXE = c:\\dom\\
LIB = c:\bc\Lib

!if $d(87)
OPS87=-f87
LIB87=fp87
!else
LIB87=emu
!endif
!if $(DB)
DEBUG = -y -v
TLDB = -v -l
!endif

.noswap

OVER_START = -o
OVER_END = -o-
OVER_LIB = $(LIB)\overlay

TCC_NRM = bcc +dombcc.cfg $(DEBUG)$(TCOPTS)$(TCC_OPTS) -Y $&.c
TCC_OVL = bcc +dombcc.cfg $(DEBUG)$(TCOPTS)$(TCC_OPTS) -Yo $&.c
TCC_VER = bcc +dombcc.cfg $(DEBUG)$(TCOPTS)$(TCC_OPTS) -Y version.c

.c.obj :
     $(TCC_NRM)

BBS_NRM = \
        $(OBJ)swap.obj     $(OBJ)bbs.obj      $(OBJ)conio.obj\
        $(OBJ)bbsutl.obj     \
        $(OBJ)file.obj     $(OBJ)file1.obj    $(OBJ)mm.obj\
        $(OBJ)com.obj      $(OBJ)utility.obj  $(OBJ)extrn.obj\
        $(OBJ)mm1.obj      $(OBJ)x00com.obj\
        $(OBJ)jam.obj

BBS_OVL = \
        $(OBJ)mm2.obj      $(OBJ)msgbase.obj\
        $(OBJ)disk.obj     $(OBJ)timest.obj   $(OBJ)utility1.obj\
        \
        $(OBJ)file2.obj    $(OBJ)file3.obj    \
        $(OBJ)archive.obj  $(OBJ)filesys.obj\
        \
        $(OBJ)menued.obj  \
        $(OBJ)uedit.obj    $(OBJ)diredit.obj  $(OBJ)subedit.obj\
        $(OBJ)stringed.obj \
        \
        $(OBJ)bbsutl2.obj\
        $(OBJ)sysopf.obj   $(OBJ)modem.obj    $(OBJ)xinit.obj\
        $(OBJ)personal.obj $(OBJ)misccmd.obj  $(OBJ)config.obj\
        \
        $(OBJ)lilo.obj     $(OBJ)error.obj    $(OBJ)chat.obj\
        $(OBJ)nuv.obj\
        $(OBJ)dv.obj       $(OBJ)newuser.obj\
        $(OBJ)regis.obj


$(EXE)dom.exe: $(BBS_NRM) $(BBS_OVL) version.c
  $(TCC_VER)
  tlink $(TLDB) -c -x -yx  @&&^
$(LIB)\c0h.obj $(OBJ)version.obj $(BBS_NRM) $(OVER_START) $(BBS_OVL) $(OVER_END)
$(EXE)dom
dom.map
$(LIB)\$(LIB87) $(LIB)\mathh $(OVER_LIB) $(LIB)\ch $(LIB)\bph.lib $(LIB)\jamcapih.lib
^

# $(LIB)\mb_lib_h.lib



install.exe : $(OBJ)install.obj
        tcc -n.\ $(OBJ)install.obj
 

$(OBJ)install.obj : install.c
        tcc -c -n$(OBJ)  install.c

$(OBJ)bbs.obj     : bbs.c      vars.h vardec.h
  $(TCC_NRM)

$(OBJ)jam.obj     : jam.c
  $(TCC_NRM)

$(OBJ)jam1.obj     : jam1.c
  $(TCC_NRM)

$(OBJ)tcmod.obj   : tcmod.c
  $(TCC_NRM)

$(OBJ)hudson.obj   : hudson.c
  $(TCC_NRM)

$(OBJ)hudson1.obj   : hudson1.c
  $(TCC_NRM)

$(OBJ)disk.obj    : disk.c     vars.h vardec.h
  $(TCC_OVL)

$(OBJ)timest.obj  : timest.c   vars.h vardec.h
  $(TCC_OVL)

$(OBJ)modem.obj   : modem.c    vars.h vardec.h
  $(TCC_OVL)
$(OBJ)bbsutl.obj  : bbsutl.c   vars.h vardec.h
  $(TCC_NRM)
$(OBJ)bbsutl1.obj : bbsutl1.c  vars.h vardec.h
  $(TCC_NRM)
$(OBJ)com.obj     : com.c      vars.h vardec.h
  $(TCC_NRM)
$(OBJ)x00com.obj     : x00com.c      vars.h vardec.h
  $(TCC_NRM)
$(OBJ)conio.obj   : conio.c    vars.h vardec.h
  $(TCC_NRM)
$(OBJ)connect1.obj: connect1.c vars.h vardec.h
  $(TCC_OVL)
$(OBJ)extrn.obj   : extrn.c    vars.h vardec.h
  $(TCC_NRM)
$(OBJ)msgbase.obj : msgbase.c  vars.h vardec.h
  $(TCC_NRM)
$(OBJ)msgbase1.obj: msgbase1.c vars.h vardec.h
  $(TCC_NRM)
$(OBJ)msgbase2.obj: msgbase2.c vars.h vardec.h
  $(TCC_OVL)
$(OBJ)newuser.obj : newuser.c  vars.h vardec.h
  $(TCC_OVL)
$(OBJ)sysopf.obj  : sysopf.c   vars.h vardec.h
  $(TCC_OVL)
$(OBJ)utility.obj : utility.c  vars.h vardec.h
  $(TCC_NRM)
$(OBJ)utility1.obj : utility1.c  vars.h vardec.h
  $(TCC_OVL)
$(OBJ)file.obj    : file.c     vars.h vardec.h
  $(TCC_NRM)
$(OBJ)netsup.obj  : netsup.c   vars.h vardec.h
  $(TCC_OVL)
$(OBJ)voteedit.obj: voteedit.c vars.h vardec.h
  $(TCC_OVL)
$(OBJ)tedit.obj   : tedit.c    vars.h vardec.h
  $(TCC_OVL)
$(OBJ)uedit.obj   : uedit.c    vars.h vardec.h
  $(TCC_OVL)
$(OBJ)diredit.obj : diredit.c  vars.h vardec.h
  $(TCC_OVL)
$(OBJ)subedit.obj : subedit.c  vars.h vardec.h
  $(TCC_OVL)
$(OBJ)file3.obj : file3.c  vars.h vardec.h
  $(TCC_OVL)
$(OBJ)multmail.obj: multmail.c vars.h vardec.h
  $(TCC_OVL)
$(OBJ)personal.obj: personal.c vars.h vardec.h
  $(TCC_OVL)
$(OBJ)misccmd.obj : misccmd.c  vars.h vardec.h
  $(TCC_OVL)
$(OBJ)xinit.obj   : xinit.c    vars.h vardec.h
  $(TCC_OVL)
$(OBJ)filetmp.obj : filetmp.c  vars.h vardec.h
  $(TCC_OVL)
$(OBJ)batch.obj   : batch.c    vars.h vardec.h
  $(TCC_OVL)
$(OBJ)lilo.obj    : lilo.c     vars.h vardec.h
  $(TCC_OVL)
$(OBJ)mm.obj      : mm.c vars.h vardec.h
  $(TCC_NRM)
$(OBJ)mm1.obj     : mm1.c vars.h vardec.h
  $(TCC_NRM)
$(OBJ)mm2.obj     : mm2.c vars.h vardec.h
  $(TCC_OVL)
$(OBJ)menued.obj  : menued.c  vardec.h vars.h
  $(TCC_OVL)
$(OBJ)file1.obj   : file1.c vardec.h vars.h
  $(TCC_NRM)
$(OBJ)file2.obj   : file2.c vardec.h vars.h
  $(TCC_OVL)
$(OBJ)minidos.obj : minidos.c vars.h
  $(TCC_OVL)
$(OBJ)bbsutl2.obj : bbsutl2.c vardec.h  vars.h
  $(TCC_OVL)
$(OBJ)mouse.obj   : mouse.c vardec.h  vars.h
  $(TCC_OVL)
$(OBJ)dv.obj      : dv.c
  $(TCC_OVL)
$(OBJ)error.obj   : error.c
  $(TCC_OVL)
$(OBJ)stringed.obj: stringed.c
  $(TCC_OVL)
$(OBJ)fido.obj    : fido.c vardec.h vars.h
  $(TCC_OVL)
$(OBJ)chat.obj    : chat.c
  $(TCC_OVL)
$(OBJ)swap.obj    : swap.asm
  tasm swap -mx -d_Large,$(OBJ)swap.obj
$(OBJ)dq.obj      : dq.c vardec.h vars.h qwk.h
  $(TCC_OVL)
$(OBJ)qwk.obj     : qwk.c writline.c strips.c qwk.h
  $(TCC_OVL)
$(OBJ)config.obj  : config.c vardec.h
  $(TCC_OVL)
$(OBJ)nuv.obj     : nuv.c nuv.h
  $(TCC_OVL)
$(OBJ)archive.obj : archive.c
  $(TCC_OVL)
$(OBJ)filesys.obj : filesys.c
  $(TCC_OVL)
$(OBJ)regis.obj   : regis.c
  $(TCC_OVL)


fcns:
  strip fcns.h &&^
$(BBS_NRM) $(BBS_OVL)
^

indent:
  cindentl &&^
$(BBS_NRM) $(BBS_OVL)
^
