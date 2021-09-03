#!/bin/env perl

# Updates ManalinkEx.dll and Magic.exe in-place.
# 1. Eliminates first-strike icon when double-strike is present.
# 2. Rewrites the function at 0x48cb60 (which takes two card_instance_t*'s and determines whether their displayed abilities are the same) so that smallcards are repainted when new abilities are added or removed
# 3. Calls into C both for the initial read of ability tooltip names and for retrieving them.

use strict;
use warnings;

my $filename;

main();

sub seek_and_write
{
  my $file = shift;
  my $seekpos = shift;
  seek $file, $seekpos, 0 or die(sprintf "Couldn't seek to %x in $filename: %s", $seekpos, $?);
  print $file pack("C" . scalar @_, @_) or die (sprintf "Couldn't write" . (" 0x%02x" x scalar @_) . " at %x in $filename: %s", @_, $seekpos, $?);
}

sub main
{
  my $f;

  $filename = "ManalinkEx.dll";
  open($f, "+<", $filename) or die "Couldn't open $filename: ?!";
  binmode($f);

  #############################################################
  # Eliminate first-strike icon when double-strike is present #
  #############################################################

  # (These modify a previous injection, the entirety of which was moved in patch_orphan_manalinkex.pl)

  # Pass a third parameter, the bitfield for the standard icons to be drawn, to get_ability_image()
  #seek_and_write($f, 0x203dbb,
  #		 0x8b, 0x85, 0x4c, 0xff, 0xff, 0xff,	# mov eax, dword [ebp-0xb4]
  #		 0x50);					# push eax

  # Pop three parameters instead of two
  #seek_and_write($f, 0x203dd1,
  #		 0x0C);					# {add esp, 0x08} => {add esp, 0x0C}

  # Write return value to address of bitfield for standard icons, rather than or'ing it in
  #seek_and_write($f, 0x203dd4,
  #		 0x89);					# {or dword [ebp-0xb4], eax} => {mov dword [ebp-0xb4], eax}

  ####################################################################
  # Repaint cards when new ability icons need to be added or removed #
  ####################################################################

  # Replace function at 0x48cb60 with new version at 0x1270040
  #seek_and_write($f, 0x26f440,
  #		 0x55,					# push ebp			; save stack
  #		 0x89, 0xe5,				# mov ebp, esp			; new stack
  #		 0xba, 0x01, 0x00, 0x00, 0x00,		# mov edx, 0x1			; default 1

  #		 0x8b, 0x4d, 0x0c,			# mov ecx, dword [ebp+0xc]	; ecx = param2
  #		 0x8b, 0x45, 0x08,			# mov eax, dword [ebp+0x8]	; eax = param1
  #		 0x8b, 0x49, 0x28,			# mov ecx, dword [ecx+0x28]	; ecx = param2->card_instance_t::regen-status
  #		 0x39, 0x48, 0x28,			# cmp dword [eax+0x28],ecx	; compare *(eax+0x28) to ecx
  #		 0x75, 0x29,				# jnz set0			; if different, jump to set0

  #		 0x8b, 0x4d, 0x0c,			# mov ecx, dword [ebp+0xc]	; ecx = param2
  #		 0x8b, 0x89, 0xf8, 0x00, 0x00, 0x00,	# mov ecx, dword [ecx+0xf8]	; ecx = param2->card_instance_t::targets[16].card
  #		 0x39, 0x88, 0xf8, 0x00, 0x00, 0x00,	# cmp dword [eax+0xf8],ecx	; compare *(eax+0xf8) to ecx
  #		 0x75, 0x18,				# jnz set0			; if different, jump to set0

  #		 0x8b, 0x4d, 0x0c,			# mov ecx, dword [ebp+0xc]	; ecx = param2
  #		 0x8b, 0x49, 0x08,			# mov ecx, dword [ecx+0x8]	; ecx = param2->card_instance_t::state
  #		 0x81, 0xe1, 0x00, 0x20, 0x00, 0x00,	# and ecx, 0x2000		; eax &= STATE_VIGILANCE
  #		 0x8b, 0x40, 0x08,			# mov eax, dword [eax+0x8]	; eax = param1->card_instance_t::state
  #		 0x25, 0x00, 0x20, 0x00, 0x00,		# and eax, 0x2000		; eax &= STATE_VIGILANCE
  #		 0x39, 0xc8,				# cmp eax, ecx			; compare eax to ecx
  #		 0x74, 0x01,				# jz epilog			; if same, jump to epilog
#set0:
  #		 0x4a,					# dec edx			; --edx
#epilog:
  #		 0x89, 0xd0,				# mov eax,edx			; return edx
  #		 0xc9,					# leave
  #		 0xc3);					# ret

  ###########################################################
  # Mark rects of new icons for tooltips (injection target) #
  ###########################################################

  # Call 0x200224b5 (get_ability_image)
  seek_and_write($f, 0x26f400,
		 0x8b, 0x85, 0xf4, 0xfe, 0xff, 0xff,	# mov	eax, dword [ebp-0x10c]
		 0x50,					# push	eax
		 0x8b, 0x45, 0xf0,			# mov	eax, dword [ebp-0x10]
		 0x50,					# push	eax
		 0x8b, 0x45, 0xf8,			# mov	eax, dword [ebp-0x08]
		 0x50,					# push	eax
		 0xe8, 0xa1, 0x24, 0xd9, 0x00,		# call	0x20024b5		; get_ability_image()
		 0x83, 0xc4, 0x12,			# add	esp,0x12

		 0x89, 0x85, 0xf4, 0xfe, 0xff, 0xff,	# mov	dword [ebp-0x10c],eax
		 0xe9, 0x6f, 0x9e, 0x21, 0xff);		# jmp	0x489e91

  close $f or die "Couldn't close $filename: ?!";


  $filename = "Magic.exe";
  open($f, "+<", $filename) or die "Couldn't open $filename: ?!";
  binmode($f);

  ####################################################################
  # Repaint cards when new ability icons need to be added or removed #
  ####################################################################

  # Inject a call to 0x1270040, replacing the function at 0x48cb60.
  # Previous contents:
  # 48cb60:	55		push	ebp			; save stack
  # 48cb61:	8b ec		mov	ebp, esp		; new stack
  # 48cb63:	ba 01 00 00 00	mov	edx, 0x1		; default 1
  # 48cb68:	8b 4d 0c	mov	ecx, dword [ebp+0xc]	; ecx = param2
  # 48cb6b:	8b 45 08	mov	eax, dword [ebp+0x8]	; eax = param1
  # 48cb6e:	8b 49 28	mov	ecx, dword [ecx+0x28]	; ecx = param2->card_instance_t::regen_status
  # 48cb71:	39 48 28	cmp	dword [eax+0x28], ecx	; compare *(eax+0x28) to ecx
  # 48cb74:	74 01		je	0x48cb77		; if equal, goto 48cb77
  # 48cb76:	4a		dec	edx			; edx -= 1
  # 48cb77:	8b c2		mov	eax,edx			; 48cb77: return edx
  # 48cb79:	c9		leave
  # 48cb7a:	c3		ret

  #Obsolete - function moved into C
  #seek_and_write($f, 0x8cb60,
  #		 0xe9, 0xdb, 0x34, 0xde, 0x00,	# jmp 0x1270040
  #		 0x90, 0x90, 0x90);		# nop	(3 of them, so the following code is still aligned)

  #####################################################################################
  # Call initialize_ability_tooltip_names() in C instead of writing to a fixed buffer #
  #####################################################################################
  # Previous contents:
  # 464a3f:	c7 45 fc ff ff ff ff	mov	dword [ebp-0x4], 0xffffffff	; v2 = -1
  # 464a46:	ff 45 fc		inc	dword [ebp-0x4]			; ++v2
  # 464a49:	83 7d fc 12		cmp	dword [ebp-0x4], 0x12		; compare v2 to 18
  # 464a4d:	7d 2a			jge	0x464a79			; if >=, goto 0x464a79
  # 464a4f:	8b 45 fc		mov	eax, dword [ebp-0x4]		; eax = v2
  # 464a52:	69 c0 2c 01 00 00	imul	eax, eax, 0x12c			; eax *= 300
  # 464a58:	05 ac 5c 71 00		add	eax, 0x715cac			; eax += 0x715cac
  # 464a5d:	50			push	eax				; param2 = (0x715cac + 300*v2)
  # 464a5e:	8b 45 fc		mov	eax, dword [ebp-0x4]		; eax = v2
  # 464a61:	8d 04 80		lea	eax, [eax + eax*4]		; eax *= 5
  # 464a64:	8d 04 80		lea	eax, [eax + eax*4]		; eax *= 5
  # 464a67:	8d 04 45 60 82 73 00	lea	eax, [eax*2 + 0x738260]		; eax = 2*eax + 0x738260
  # 464a6e:	50			push	eax				; param1 = (0x738260 + 50*v2)
  # 464a6f:	e8 cc 07 07 00		call	0x4d5240			; internal_strcpy(param1, param2)
  # 464a74:	83 c4 08		add	esp, 0x8			; pop two parameters
  # 464a77:	eb cd			jmp	0x464a46			; loop back to 0x464a46
  # 464a79:	...
  seek_and_write($f, 0x64a3f,
		 0xe8, 0xa5, 0x5f, 0xba, 0x01,	# call 200a9e9
		 0xeb, 0x33);			# jmp 0x464a79

  ########################################
  # Mark rects of new icons for tooltips #
  ########################################

  # Inject a call to 0x1270000.  The statements it overwrites is a nonfunctional previous attempt to add a KEYWORD_SHROUD (or perhaps rampage?) bit.
  # Previous contents:
  # 489e81:	80 7e 26 00		cmp	byte [esi+0x26], 0x0
  # 489e85:	74 0a			je	0x489e91
  # 489e87:	81 8d f4 fe ff ff 00	or	dword [ebp-0x10c], 0x20000
  # 489e8e:	00 02 00
  # 489e91:	...
  seek_and_write($f, 0x89e81,
		 0xe9, 0x7a, 0x61, 0xde, 0x00,		# jmp 0x1270000
		 0x90, 0x90, 0x90, 0x90, 0x90,		# nop	(11 of them, for the rest of the statement)
		 0x90, 0x90, 0x90, 0x90, 0x90,
		 0x90);

  # Loop until >= 32 instead of >= 18 while checking for icon bounding boxes
  seek_and_write($f, 0x89eaf,
		 0x20);					# {cmp dword [ebp-0x7c], 0x12} => {cmp dword [ebp-0x7c], 0x20}

  # Retrieve tooltip name.
  # Previous contents:
  # 48a053:	8b 45 bc		mov	eax, dword [ebp-0x44]
  # 48a056:	8d 04 80		lea	eax, [eax + eax*4]
  # 48a059:	8d 04 80		lea	eax, [eax + eax*4]
  # 48a05c:	8d 04 45 60 82 73 00	lea	eax, [eax*2 + 0x738260]
  # 48a063:	50			push	eax
  # 48a064:	8d 85 08 ff ff ff	lea	eax, [ebp-0xf8]
  # 48a06a:	50			push	eax
  # 48a06b:	e8 d0 b1 04 00		call	0x4d5240
  # 48a070:	83 c4 08		add	esp, 0x8
  # 48a073:	...
  seek_and_write($f, 0x8a053,
		 0x8b, 0x45, 0xbc,			# mov	eax, dword [ebp-0x44]	; param2 = v78
		 0x50,					# push	eax
		 0x8d, 0x85, 0x08, 0xff, 0xff, 0xff,	# lea	eax, [ebp-0xf8]		; param1 = &Dest
		 0x50,					# push	eax
		 0xe8, 0x81, 0x09, 0xb8, 0x01,		# call	200a9e4			; fetch_ability_tooltip_name(&Dest, v78)
		 0x83, 0xc4, 0x08,			# add	esp, 0x8		; pop two parameters off stack
		 0x90, 0x90, 0x90, 0x90, 0x90,
		 0x90, 0x90, 0x90, 0x90, 0x90,		# nop	(13 of them, for the rest of the replaced statements)
		 0x90, 0x90, 0x90);

  close $f or die "Couldn't close $filename: ?!";
}
