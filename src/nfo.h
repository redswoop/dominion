/* � Online NFO System.  C Structures.                                    �
 * 읕컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴켸
 * The Online NFO Systems allow the creator of an archive to store maximal
 * data in the file, in a standardized data format.  This data will be then
 * usable by the BBS the file is on, as a source of extended data.  This
 * will greatly standardize things, making a file appear the same no
 * matter where it is.
 *
 * Online NFO is Copyright 1993 by Dominous.
 * This system is Public Domain, and we encourage the integration of this
 * system into other softwares.  A editor for this data is available via
 * Dominous Support Boards.
 * 컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴
 */

typedef struct {
       unsigned char                    // Character Strings
                                        // Unsigned to allow Hign Ascii

             fn[12],                    // Original File Name
             desc[MAX_PATH_LEN],                  // Description of contents
             group[MAX_PATH_LEN];                 // Group/Author releasing it

        int
             total,                     // Total Number of Disks
             current;                   // What disk this is.

        unsigned long                   // Unsigned Longs to allow for as
                                        // much data as possible

             attr,                      // Bit Mapped information,
             sound,                     // Sound options
             video,                     // Video Options
             input,                     // Input Options
             os,                        // Operating System

             size,                      // Original Archive size
             numf,                      // Original Number of files
             completesize,              // Size when completely installed

             fileidlen,                 // If File_Id.Diz is present, it will
                                        // be appended to the end of the data
                                        // file.  The length will be stored
                                        // here, so it can easily be retrieved.
             fileidoffset;              // Posistion File ID is stored at.


             struct date rdate;         // Date on which it was released
             struct time rtime;         // Time at which it was released
} nforec;


#define sound_std 0x0001                // Internal Speaker
#define sound_adlib 0x0002              // Adlib
#define sound_sb 0x0004                 // Sound Blaster
#define sound_sbpro 0x0008              // Sound Blaster Pro
#define sound_gravis 0x0010             // Gravis UltraSound
#define sound_LAPC1 0x0020              // Roland LAPC-1 type card
#define sound_proaudio 0x0040           // Proaudio Spectrum
#define sound_canvas 0x0080             // Roland Sound Canvas
#define sound_turtle 0x0100             // Turtle Beach

#define input_joystick 0x0001           // Joystick
#define input_thrust 0x0002             // Thrustmaster and the sort
#define input_mouse 0x0004              // Mouse
#define input_modem 0x0008              // Modem Support
#define input_keyb 0x0010               // Keyboard
#define input_scanner 0x0020            // Scanner

#define video_vga800 0x0001             // Vga 800x600
#define video_vga1024 0x0002            // VGA 1024x768
#define video_16c 0x0004                // 16 colors
#define video_256c 0x0008               // 256 colors
#define video_extc 0x0010               // Extended Colors
#define video_vga320 0x0020             // Vga 320x200
#define video_vga640 0x0040             // Vga 640x480
#define video_ega 0x0080                // Ega
#define video_cga 0x0100                // Cga

#define os_DOS 0x0001                   // DOS
#define os_OS2 0x0002                   // OS2
#define os_WIN 0x0004                   // Windows
#define os_NT 0x0008                    // NT
#define os_NEXT 0x0010                  // NeXTSTEP
#define os_GEO 0x0020                   // GeoWorks
#define os_NOVELL 0x0040                // Novell
#define os_XWIN 0x0080                  // Xwindows
#define os_UNIX 0x0100                  // UNIX
