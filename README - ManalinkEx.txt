************************************************************
*** Based on latest jatill version on old manalink forum ***
************************************************************

  Magic.exe is now patched to load ManalinkEx.dll, this dll
adds about 2mb of free space to code new cards.
  Also, the editor NEEDS this file to work now, along 
magic.exe and manalink.csv.
  The dll free space start at virtual address 0x01003839,
its loads at 0x01001000 (0x01000C00 for asm purposes). So,
just use the assembler part of editor, and work at 01003839.
  If anyone is insterested, the source code of dll is inside
editor folder, there is no trick involved at all. Oh, by the
way, is written in Delphi.
  The editor understand where you try to dissassembler, and
show either magic.exe code or manalinkex.dll code, based on
address. You don't need to worry about it, just apply patch,
edit, fill... whatever...

* News:
  Civic Wayfinder is the first card coded on the dll, is at
address 01003839, and ends at 01003AB0, the next card of 
course, will start after this address.

Good luck coders!