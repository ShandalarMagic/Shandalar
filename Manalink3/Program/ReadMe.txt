----------------------
     Manalink 3.0
 March 25 2015 Update
     Fate Reforged
----------------------


Install
-------
After extracting the archive, just navigate to the Manalink 3.0 folder and
click the "Install_Me_First!.cmd" file. It will run the Default Mods and
create the "Toggle_Sounds.cmd" and "Play_Manalink.cmd" files.
Full Install will require 505MiB of free space.

If you have any issues with text or art when first played, please reboot.

Windows Updates
---------------
Manalink 3.0 requires the Runtime Files for both Visual Basic 6.0 and
Visual C++ 2010 to be installed. If Manalink complains about missing .DLLs
and you need these for your system, they are found in the "WinUpdates" folder.

Mods
----
There are many "Mod.exe" files found in the ".\Mods" folder.
To use them, first exit the game, click on the Mod, then restart the game.

The Mods will identify your Manalink folder by first searching for the file
"Magic.exe" in the current directory tree. If not found, it will jump up one
directory and search again. If still not found most Mods will place all files
in the folder ".\Program", creating it if necessary. This extensive search
should enable these Mods to operate with most any Manalink 3.0 Installation.
They're actually WinNT BatchScripts that are 'compiled' to executables with
the archives embedded inside. The embedded source files can be extracted to
the current folder from any of the Mods by using the "ExtractFiles" or "ef"
commandline switch.
The Mods will not operate if the current folder is read-only.

There are several different types of Mods:

    ArtMods - These contain alternates to many of the in-game images found
              within Manalink. Most install completely silently, have no
              options, and will simply clobber any pre-existing files. The
              filename will usually indicate which images are replaced.
              If not, then multiple types of images are usually replaced.

    ArtMods - Manalink currently has a limit of 25 images per Basic Land.
  BasicLand   This mod will randomly select new card images from a pool
              of over 400 .JPGs. The first image for each land is replaced
              directly, as it's the reference image you see in DeckBuilder.
              The maximum number of each land to randomize is selectable
              on the commandline. The default is 25, the minimum is 3.

    ArtMods - These Mods contain PlayFaces (seen during Duels) and Rogues
     Rogues   (seen during Sealed Deck). There are a few options available
              which are presented by a small user interface.
              'Classic' Rogues will work with any version of Microprose MtG.
              'Modern' Rogues will only work with Manalink 3.0 or higher.
              If a commandline switch is used, then no UI is presented.
              All commandline options will eraticate any existing content.
              
                 playfaces - will extract only PlayFaces.
                    rogues - will extract only Rogues.
              AnyOtherWord - will extract both Rogues and PlayFaces.

  SoundMods - These contain in-game sounds, encoded with OggVorbis and decoded
              at runtime. Since the original sounds are so loud, you will
              be presented with the option to decrease their volume.
              If they're still too loud, you can simply run the Mod
              repeatedly until you find a satisfactory level.
              The level decrease is measured in negative decibels and may be
              provided on the commandline as a number between 0 and 99.
              If it is, the intial prompt is not given.
              
  VideoMods - The only VideoMod available will install the WMV version of the
              Magic the Gathering Tutorial from the Magic 2010 Custom ISO.
              You can access it from the Main Menu and it should run fine
              on Windows XP and up.

When initially installed, the default Mods are:

    .\Mods\ArtMod_Manalink3.0_Sonic2013.exe Default
    .\Mods\ArtMod_Rogues_Classic_MicroproseOriginals.exe Both
    .\Mods\SoundMod_Manalink3.0_Default.exe 8

Play_Manalink.cmd
-----------------
Click or call it to run Manalink.
It will find itself, enter the .\Program folder, and execute "Magic.exe".

Delete_Drafts.cmd
-----------------
Click it to remove all Human and Opponent Draft Decks from the Program folder.
    
Toggle_Sounds.cmd
-----------------
Click it when the Sounds are ON, and it'll turn them OFF.
Click it when the Sounds are OFF, and it'll turn them ON.

...hence the "Toggle".

This is a simple BatchFile to toggle the sounds ON and OFF. It does this by
ReNaming the "Sound" and "DuelSounds" folders and copying back the few 
sounds necessary for the game to run to the "Sound" folder; or it removes the
"Sound" folder and ReNames the original folders so that the game can find them.
It's safe, simple, quick, and non-destructive... listen to music while playing!
Works with any Microprose MtG install, read the BatchFile for more details.

PlayDeck Analyser
-----------------
A small multi-purpose app that corrects a folder of PlayDecks to match the
current Manalink Update. Good for checking your PlayDeck folder when a new
patch is released. It not only contains a small myriad of Search, Sort,
Substitute, Convert, and Replace functions for PlayDecks, but it also performs
several other Manalink-related tasks. See the ReadMe for more details.

Notes
-----
This ReadMe is just a stub. Manalink is currently in a state of flux, and there
are many aspects of the game that worked in v2.0, but are broken in v3.0. This
probably includes (but may not be limited to) Sealed Deck, Draft, Rotisserie,
Momir Basic, EDH, etc. Many of these may work, but with sporatic results.

Since these issues are expected to be eventually resolved, the composition of
a full ReadMe at this time is pointless. Development is ongoing and updates
are frequent. For the newest info on Patches, Updates, and WhatNot, please
visit the Manalink 3.0 Forum at http://www.slightlymagic.net/forum/

-------------------------------------------------------------------------------
-------------------------------------------------------------------------------

"No one ever reads the ReadMe..." -- CirothUngol

compiled May 9, 2015
-------------------------------------------------------------------------------
-------------------------------------------------------------------------------

June 20, 2009
From the CCGHQ Team at
Computer Card Game Headquarters
http://www.slightlymagic.net/forum/

Rescued from the defunct manalink.de,

THIS IS MOK'S ORIGINAL MANALINK 2.0 UPDATE OF
Microprose's Magic: The Gathering
Duels of the Planeswalkers with
Spells of the Ancients Expansion

======================================================
MANALINK DEVELOPMENT CONTINUES!
FOR THE LATEST MANALINK 2.0+ PATCHES BE SURE TO VISIT
http://www.slightlymagic.net/forum/viewforum.php?f=25
======================================================

NOTE: Most of the following links are by now obsolete.
Such is the nature of the internet. :(



	 MICROPROSE MAGIC THE GATHERING & MANALINK UNOFFICIAL UPDATE
				Version 1.3.2
		       by Mok <mok666@poczta.onet.pl>

THIS PROGRAM MAY NOT BE INCLUDED ON ANY CD-ROM COMPILATION, COVER DISK
OR SOLD IN ANY WAY.

This is 2nd release of an unofficial MPS Magic: The Gathering update.
It will allow direct TCP/IP connections over the internet, enable
all the expansion cards regardless of your version and add some
small modifications to the game. Questions/comments are welcome.

System Requirements:

- DirectX 7 or newer installed
- English version of Magic: The Gathering game with Manalink installed

 * if you have only MtG, you must install Manalink update (aprox. 60MB):
     ftp://ftp.microprose.com/pub/updates/manalink/us/mlus1_11.zip
     ftp://ftp.microprose.com/pub/updates/manalink/us/mlus2_11.zip
     ftp://ftp.microprose.com/pub/updates/manalink/us/mlus3_11.zip
     ftp://ftp.microprose.com/pub/updates/manalink/us/mlus4_11.zip
     ftp://ftp.microprose.com/pub/updates/manalink/us/mlus5_11.zip
     ftp://ftp.microprose.com/pub/updates/manalink/us/mlus6_11.zip

 * if you have MtG and Spells of the Ancients, you must install Manalink
   for SotA (aprox. 5MB):
     ftp://ftp.microprose.com/pub/updates/manalink/us/ml_sota_us.zip

 * if you have Duels of the Planewalkers, you don't need anything.

- Manalink v1.3 files are already included in this patch.

- If you already installed 1.3.1 update, you don't need to reinstall
  the game before patching to 1.3.2.

- The patch is network compatible with 1.3 and 1.3.1.

*WARNING*:
- This patch will NOT backup your old files.
- If you run it on foreign version of MtG, some text will be in english.

How to use TCP/IP play:
- start Manalink and select 4th option: "Internet TCP/IP". A dialogbox
  will ask you to type the IP address of the server.

- If you want to host a game, do not type anything and just press Enter.
  The other players must type your IP adress to connect.

That's all. Newest updates should be always available on this page:
http://members.home.com/adonald1/magic/default.htm


History
-------

Changes in version 1.3.2 - Second release
 (and probably the last one - I failed to get MtG source code from
  MPS for more enhancements so I lost interest in the whole project).

- All: hopefully finally fixed to work on Windows ME. Previous patch
  worked only in rare cases (my machine for example ;)

- All: experimental patch for Windows 2000. I didn't test it much,
  so please report any problems. Anyone willing to test it on Win XP?

- Shandalar: slowed down even more for the 1GHz+ machines.

- Shandalar: fixed crash when trying to sell Mana Crypt.

- Shandalar: fixed displaying wrong amulet color when offered
  by your opponent.

Changes in version 1.3.1 - First release

- Manalink: replaced serial connection with Internet TCP/IP connection.

- Magic: randomizing routines replaced. But it will NOT give you more mana
  in manalink. Not enough mana in manalink is only a myth. Actually, the game
  cheats and gives you MORE mana sources when playing against the computer :)
  and that's why you might get an impression that you are receiving not enough
  mana in manalink.

- All: fixed to run in Windows Millennium Edition. But if you are thinking
  about moving from Win 98 to ME, think again. ME is piece of s**t.

- All: enabled all the cards regardless of which expansions you have installed.

- All: CD check removed.

- Shandalar: slowed down to work on very fast computers at reasonable speed.

- Magic: debug menu removed.

- Deckbuilder: fixed "stats" of several cards (mana sources):
  - Badlands (black and red)
  - Mana Batteries (their appropriate colors)
  - Celestial Prism (all colors)
  - Sunglasses of Urza (no longer mana source)
  - Channel (colorless)
  - Drain Power (colorless)
  - Energy Tap (colorless)
  - Fastbond (colorless)
  - Reset (colorless)
  - Untamed Wilds (colorless)
  - Land Tax (no longer mana source)
  - Mana Flare (colorless)
  - Apprentice Wizard (colorless)
  - Coal Golem (red)
  - Elves of Deep Shadow (black)
  - Fire Sprites (red)
  - Ley Druid (colorless)
  - Gauntlet of Might (red)

------------------------------------------------------------------------------




===========================================================

         MAGIC: THE GATHERING
               ManaLink
             Version 1.3

              Readme File
             1 May 1998
              MicroProse
===========================================================

IMPORTANT NOTE: This ManaLink update is for owners of Duels of
the Planeswalkers and anyone who has installed the ManaLink
multiplayer module. This update will NOT work with the original
Magic: The Gathering game or Spells of the Ancients UNLESS you
have also installed the free ManaLink module.

ManaLink and its updates are available at 
www.gathering.net, 
www.ten.net and www.microprose.com.


How to Update to ManaLink Version 1.3
=====================================
1. Download the file appropriate to your language version of
   Magic: The Gathering (MTG_13US.EXE for English).

2. Double-click on the file and follow the directions in the
   pop-up dialog box. The updater will automatically update 
   the Magic files.

3. To verify that you have successfully updated to ManaLink
   version 1.3, start Magic: The Gathering. Then double-click
   the ManaLink icon and look for "1.3" at the bottom of the 
   window.


General Changes in ManaLink Version 1.3
=======================================
* The "AddCard" assertion errors in Shandalar have been fixed.

* The Info window in ManaLink now has a background graphic and
  a Close button.

* Saved games with a Vesuvan Doppelganger play now load correctly.


General Card Changes in ManaLink Version 1.3
============================================
* Cards that modify the hand size (such as Land Tax) now work
  correctly when used in conjunction with Balance.

* If you are forced to tap dual lands (by a Power Sink, for example),
  you can no longer cancel this action and avoid payment.


Specific Card Changes in ManaLink Version 1.3
=============================================
* Alchor's Tomb now has the correct card text.

* Conversion now works correctly with dual lands (such as the Bayou 
  and Taiga) in play.

* If more than one Fellwar Stone is in play, cards that are playable 
  now light up for casting or activation.

* Gauntlet of Might's known problems are fixed.

* Ifh-Biff Efreet's legacy card now correctly switches sides when the
  creature changes controller.

* Jade Statue lights up properly when it can be activated. In
  addition, it no longer remains tapped if a Meekstone is in play.

* Kudzu is no longer allowed to target the land that it is currently
  enchanting.

* Library of Leng's effect now resolves properly when the card
  leaves play.

* Lich no longer causes you to lose the game if it is counterspelled.

* Mishra's Factory will not turn into an Assembly Worker if it
  is tapped for mana.

* The Necropolis of Azar correctly calculates the number of counters 
  it has on it.

* Nevinyrral's Disk now works correctly when an untapped Guardian 
  Beast is in play.

* Personal Incarnation does not give its effect before resolving 
  (i.e., while still in the spell chain).

* Power Sink no longer forces you to tap lands that do not produce mana
  (such as the Oasis).

* Resurrection now works correctly when it is counterspelled.

* The Sengir Vampire no longer gets a counter when it deals lethal 
  damage to a Mishra's Factory (turned into an Assembly Worker) that 
  has been enchanged with a Consecrate Land.

* Spell Blast now properly calculates the addition costs imposed by
  Gloom.

* Su-Chi no longer gives mana when it is removed from play (vs. going
  to the graveyard).


Additional Cards for Duels of the Planeswalkers
===============================================
As a bonus for owners of Duels of the Planeswalkers, we added the
following 16 cards for your use:
  Amnesia
  Barl's Cage
  Blood Moon
  Book of Rass
  Coal Golem
  Elder Spawn
  Fire Sprites
  Force Spike
  Goblins of the Flarg
  Hyperion Blacksmith
  Martyr's Cry
  Moat
  Rabid Wombat
  Tracker
  Wall of Wonder
  Wormwood Treefolk

(Please note that these 16 new cards are only available if you
have Duels of the Planeswalkers installed. If you have only the 
original Magic: The Gathering game or Spells of the Ancients, you 
will not see these new cards in the Deck Builder. However, you 
will be able to duel someone who has these new cards without any 
problems.)


Changes in Previous ManaLink Updates
====================================
* The Deck Builder displays your deck name and deck type 
  (Unrestricted, Wild, Restricted, Tournament or Highlander)
  in the title bar when you click the Stats button.

* The lag in the Duel has been reduced to allow about six to
  eight times as much data to flow in the same amount of time. 
  This speed increase will vary from machine to machine, but 
  almost everyone should note an improvement in dueling speed.
 
* All of the logic that is responsible for posting the duel
  results to the GatheringNet servers for ranking information
  has been corrected.

* A problem was fixed in which ManaLink would try to establish
  multiple connections to your opponent when registering via
  GatheringNet.

* ManaLink is now better able to handle simultaneous invitations
  (where you are inviting someone who is inviting you at the 
  same time).

* On LAN, modem and serial connections, your name will dynamically
  update in the chat user list if you change your player name
  (via the Player sphere). Unfortunately, it is not possible to
  change your screen name on TEN dynamically.

* The user list in ManaLink will no longer "resort" after you click
  on a column. The user list will remain sorted per your preference
  until another column is selected. Dynamic updates to the list after
  a full retrieval are inserted and reordered according to your
  sort preference.

* If you are sending a message to someone in a duel with the F6 or
  Insert key, your cursor will remain in the message bar.

* If you have Plus! or font smoothing turned on, the text in the
  messages will now anti-alias to their background.

* The message bar at the top of the screen is now 20 pixels shorter
  so as to obscure less playing area.

* The Filter panel no longer closes by clicking on its background.
  Click on the Filter button again in the ManaLink window to close
  the Filter panel.

* If you click the Info button to get info about another player, their
  correct badge will display.

* You can not assign Friend or Mute badges to official online 
  representatives such as MicroProse employees, TEN guides, etc.

* ManaLink's Chat window can now handle more than 500 lines of text. If
  you are saving the contents of the Chat window, however, please be
  aware that the "oldest" lines after 4K of text are expired and are 
  removed from the window.

* Setting your status (Available, Don't Disturb or Need Help) now works
  correctly on GatheringNet. ManaLink now waits for the server to reply 
  with your real status and then updates it after receving this data. 

* You can now send invitations to duel with greater success.

* The messages no longer add an automatic period to the end of your
  sentence if it already has punctuation.

* If minimized, the Chat window's title bar will flash if you receive
  new text.

* The problem with receiving messages from players who did not send you
  a message has been fixed.

* The problem with receiving invitations from players who did not
  invite you has been fixed.


MicroProse, Inc.
2490 Mariner Square Loop
Alameda, CA 94501
www.microprose.com
(510)864-4550

===========================================================

         MAGIC: THE GATHERING 
        "Duels of the Planeswalkers"
             Version 3.0

             Readme File
            14 January 1998
===========================================================

Table of Contents
=================
1) Important Customer Support Note
2) FAQ (Frequently Asked Questions)
3) New Features
     Shandalar
     The Duel and Gauntlet
     Dueling Table
     Deck Builder
3) Card Notes
4) Tutorial Errata
5) IBM ThinkPad Compatibility
6) Technical Notes
7) Multiplayer Supplement


Important Customer Support Note
===============================
Do NOT call Wizards of the Coast Customer Service with questions, 
problems, or other issues related to this computer game; contact
MicroProse Customer Support. The WotC folks will happily address 
card issues, as always, but they will not help you with issues 
specific to the computer game.


FAQ (Frequently Asked Questions)
================================
For further help for Magic: The Gathering, please read the Magic
FAQ included at the end of this file. 
The FAQ includes help for installation, sound cards, video cards, 
graphics, gameplay, Shandalar and specific cards. 


New Features
============

Shandalar
---------
There's one new feature on the way to getting started in Shandalar. You are no longer
limited to choosing what your character looks like from a screen of pre-made images. Now,
you have much more control over your looks.

When you get to the point at which the big manual tells you to just pick a character, the
new Face Builder tool takes over. This allows you to choose a wizard (basic body and
face), then customize your look with various accoutrements and other variations.

The Duel and Gauntlet
---------------------
*  When you save a duel during a Gauntlet, you save the status
   of that current duel, and your progress through the Gauntlet.

*  The "Duels of the Planeswlakers" installer adds new decks to your 
   playdeck directory.

*  Multiplayer support has ben added in the form of Manalink. Please 
   consult the manual for more specific.

Dueling Table
-------------
A few reminders of often overlooked features:

*  If you do not put a Stop (the red marker) on a phase, play will
   bypass that phase without bothering to ask you if you want to 
   use optional effects (a Brass Man's untap or Land Tax, for 
   example). This is a handy way to prevent the duel from bogging 
   down, but if you are not careful, you could accidentally miss 
   an opportunity. Thus, if you plan to use an optional effect 
   (especially during the upkeep phase), make sure to Run To (by 
   clicking on the phase bar) or put a Stop on the phase you have 
   in mind.

*  Remember that creatures with negative power are treated as having
   a power of 0 for all purposes except raising the power. Thus,
   they are displayed during a duel as having 0 power. 

*  Remember that all of the floating windows -- including the Ante 
   and Mulligan displays, both hands, and the Situation Bar (but not 
   the Spell Chain and Combat windows) -- are movable. Just click 
   and drag them out of the way when necessary.

*  You can minimize both the Spell Chain and Combat windows. Just 
   click anywhere on the title bar of the window to do so. To 
   restore the Spell Chain window to full size, click on the star 
   icon on the phase bar. To restore the Combat window to full size, 
   click on the sword icon on the phase bar.

*  Hold the mouse over a particular thing (a card, part of the 
   dueling table, or an ability marker, for instance) to find out 
   what it is.

*  Right-click on things for a mini-menu of useful functions.

*  Click on things to use them. Double-click to auto-use; that is, to 
   have mana drawn from lands automatically (and indiscriminately).

Deck Builder
------------
The Deck Builder is still accessible from the Main Menu. To get to the Deck Builder,
click on Tools, then select the Deck Builder option.


* The "Basic Set" button determines which versions of the basic Magic card set are visible.
  This filter has three options, all of which are independent toggles. That is, you can
  turn any one on or off without affecting the status of the other. At least one of these
  must be selected at all times.
      
       "Unlimited" is the second collected release of the basic card set, which included
       cards from both the first (Limited or "alpha") and second ("beta") versions.
       "Revised Edition" is the third version of the basic card set.
       "Fourth Edition" is the fourth version of the basic card set.

* The "Astral" button still controls the inclusion of the Astral set of cards and now it also 
  includes the Promotional cards. (*Note: If you do not have
  Spells of the Ancients installed, you have no Promotional cards.)

* The "Arabian Nights" button filters for all of the cards from that expansion set. (If
  you do not have Spells of the Ancients installed, the only cards from this set that
  you'll have are those that also happen to be included in the Fourth Edition set.)

* The "Antiquities" button includes or excludes all of the cards from that expansion set.
  (If you do not have Spells of the Ancients installed, the only cards from this set that
  you'll have are those that also happen to be included in the Fourth Edition set.)

*  Moving multiple copies of a card into and out of your deck can 
   be a pain. Instead of moving one copy at a time, now you can move 
   as many as you need, all in one step. Hold down the Shift key 
   and double-click on the card. Enter the number you want moved. 

*  You can now right-click on the area in between the sets of Filter
   Buttons to open a mini-menu. This menu has only one option: the
   Main Buttons toggle. If you choose to toggle the Main Menu Buttons
   On, all of the filters EXCEPT the Other Filters become active (are
   turned on). If you toggle Main Menu Buttons Off, all of the Set
   Filters, Color Filters, and Type Filters are inactivated (switched
   off).

*  When you're editing your deck in Shandalar, you can also right-click 
   on the background area behind the cards in your deck to get another 
   mini-menu. This one has two options that let you to move cards according 
   to their color. Use Move By Color Into Deck to put all the cards of the 
   colors you choose from the inventory into the current deck, and Move By Color
   Out Of Deck to move cards from your deck into your inventory.
   You're prompted to choose which of the five colors, plus "artifacts"
   (not "colorless"), you want moved. Note that the basic lands that
   correspond to the colors you choose also move. (No lands move with
   artifacts, and you must move dual and special lands individually.)

*  The Deck Builder considers basic lands as having the same color 
   as the color of mana they produce for the purpose of deck stats. 
   (For example, 5 Islands count as 5 Blue cards.)


Card Notes
==========
*  A display issue: The name of artist Brian Snoddy should always
   have a line over the 'o'. (Sorry, Brian.)

*  In some places in the game, the name of El-Hajjaj is displayed
   incorrectly. There should always be an accent circumflex (caret)
   over the final 'a'.


Tutorial Errata
===============
*  Any time one of the wizards in the Tutorial mentions "points," he
   or she is being informal. There are actually no "points" in the 
   game of Magic: The Gathering. Thus:

   - In Chapters 4, 6, 8, 9 and anywhere else, when the wizards refer
     to "points of damage," they mean simply "damage."

   - In Chapter 4 (and anywhere else), when they speak of "points of
     mana," they really mean "mana."

   - In Chapter 8, one of them says, "...the ogre does two points
     against the knight's two points of toughness." That should be
     "...the ogre does two damage against the knight's two toughness."

   - In Chapters 4 and 7, the wizards talk of "life points." They 
     mean "life total."

*  In Chapter 7, the wizard mentions the "Heal Phase." Obviously, 
   his information is out-of-date. Under the 5th Edition rules, the 
   Heal Phase no longer exists. It has been replaced by the Cleanup 
   Phase.

*  In Chapter 8, the wizard says, "...other special powers include 
   regeneration, flight, and many more..." He means "...regeneration, 
   FLYING, and many more."


IBM ThinkPad Compatibility
--------------------------
*  Those of you installing this game on a ThinkPad might experience a
   problem when viewing the Tutorial videos. This is caused by the 
   video software attempting to use an incorrect driver. We've included 
   a program on the Original Magic: the Gathering CD-ROM that corrects 
   this problem.

   Run this file:

   autoplay\setindeo.exe

   and the Tutorial should work just fine.


Technical Notes
===============
*  Don't forget to take a look at the Magic FAQ (MAGICFAQ.TXT) on the
   program disc for further help with troubleshooting any problems.

*  If you normally use a screen saver, you should be aware that it is
   always running in the background. To prevent interference with the
   game, you must disable any screen saver before you start playing.

*  Some players might experience compatibility problems. Our tests
   lead us to believe that the most common cause of these problems
   is not the game itself, but out-of-date device drivers installed
   on the computer. If you experience compatibility problems, please
   make sure that you are using the most recent drivers provided for
   your hardware. If you have the correct drivers and the problem
   persists, call Customer Support.

*  If you experience problems with any part of this game after you
   install new hardware or software, you might have updated a sound
   driver, video driver, DirectX driver or Indeo Video driver to a
   version that is not compatible with the game. To fix this, you can 
   reinstall the DirectX drivers from the Magic: The Gathering CD-ROM.

*  We stongly suggest that you play this game in High Color (16-bit)
   video mode or better. If you are playing in a lesser mode (that is, 
   you are using 256 colors or less), you might experience transitory 
   graphic oddities. These are superficial and should not have any 
   lasting effect on the game. Some machines that are below the 
   recomended system requirements may experience problems with High 
   Color (16-bit) video modes when playing in Shandalar.

*  This game is designed to work with the standard issue Windows 95.
   We do NOT support ANY of the beta upgrades to Windows 95.

*  If you experience problems with flickering video in Shandalar, it's
   likely you need to update your video drivers. Most vendors now
   supply Windows 95 versions of their video drivers. You can normally 
   download these drivers from the vendor's BBS or Web site.

*  If you do not hear the sound effects in the game, you might not
   have selected to install the DirectX drivers. Rerun the install
   program, but do not select ANY game components to install. Allow
   the new DirectX drivers to be installed.

*  If you have the new Sound Blaster 16/AWE-32 driver for Windows 95
   (Revision 7), you may experience problems running this game. The
   new Sound Blaster driver updates the Audio Drive Component to 
   version 4.33.00.0012. This version causes the game to crash 
   with a fatal exception error in VXD VMM(01) and terminates the 
   application. Reinstall the DirectX drivers (as in the previous 
   paragraph). The DirectX Audio Drive Component is version 
   4.31.00.0068, which functions correctly. Creative Labs also 
   has a new driver, version 4.33.00.0014, which does not cause 
   the problem.

*  If you try to manually delete this game using Windows Explorer,
   you might experience problems. (We suggest you ALWAYS use the
   Uninstall feature instead.) When you run the game, some game files
   temporarily become part of the Windows 95 system resources. These
   files cannot be used or deleted if they are currently being used
   by your system. If you experience this type of problem, you can
   reboot your computer to free the affected files. You should then be
   able to delete the files.

*  The Uninstall feature removes only those files installed by the
   Install Shield program. Any files created by the game, such as
   saved games, will be left on the system after you run uninstall. 
   If you Uninstall "Duels of the Planeswalkers" you must also Uninstall 
   your Original Magic: the Gathering product. Some common files used
   by both applications are removed by the "Duels of the Planeswalkers" 
   Uninstaller. Please re-install the Original product before trying
   to re-install "Duels of the Planeswlakers". 



================================================
   Magic: The Gathering "Duels of the Planeswalkers"
   for Windows 95
   FAQ (Frequently Asked Questions and Answers)
   Last revised: 14 January 1998
================================================

System Requirements
===================
IBM PC 100MHz 80486DX/4 compatible or faster
Windows 95
16MB RAM
Quad-speed CD-ROM drive
Hard drive (25MB free)
Super VGA graphics for 640 x 480 x 256 colors 
  (must be compatible with DirectX)
Mouse
Local area Network with IPX protocol
Internet play requires TCP/IP connection. 
( Any ISP that offers a true PPP connection will suffice. 
  AOL 3.0 and MSN 2.0 offer a true PPP connection. )

Recommended
-----------
IBM PC 120MHz Pentium compatible or faster
32MB RAM
High Color graphics for 1024 x 768 x 16-bit color 
  (2MB video RAM minimum)
DirectX-compatible sound card

General
=======
Q:  What is the latest version of Magic: The Gathering? 

A:  Version 3.0 is the latest version of the game. This version
    is the "Duels of the Planeswalkers" standalone version of 
    Magic the Gathering. It includes all of the original Magic 
    features long with a Multiplayer module and 80 new cards.

Q:  Is Magic: The Gathering compatible with Windows 3.1, 
    Windows 3.11 or Windows NT?

A:  Unfortunately, because the program uses Microsoft's DirectX 
    version 5.0, it does not work properly with any operating 
    system except Windows 95.

Installation
============
Q:  I have the demo for Magic: The Gathering installed. Will 
    this cause any problems when I install the complete game?

A:  We recommend that you either delete the demo directory or 
    rename it before installing the complete game. By default, 
    the complete game uses the same directory name (\MAGIC) 
    that the demo uses.

Q:  While installing the game, my CD cannot be verified by the installer.

A:  If you have more than one CD ROM drive attached to your system 
    you must place the CD in the drive that appears first in the drive 
    list. i.e. You have two CD drives that are lettered D: and E: you 
    must place the CD into the D drive for verification. The second drive 
    ( in this case E: ) will not be checked.  

Q:  While installing the game, I got an error message of 
    "Assertion error, cannot open sprite file." I am using 
    Corel CD-ROM drivers.

A:  Unfortunately, the installer is not compatible with Corel 
    CD-ROM drivers. You must load Windows 95 drivers for your 
    CD-ROM drive.

Q:  I got an error while uninstalling or reinstalling the game 
    that says there is a file that is write-protected or otherwise 
    cannot be deleted. 

A:  Just reboot your computer and try again. Sometimes Windows 95 
    thinks one of the files is write-protected until the computer 
    is rebooted.

Troubleshooting
===============
Q:  I am experiencing DLL errors, assert errors or lockups.

A:  Do not run any other programs (such as Norton Crash Protector) 
    in the background. The only programs that should be loaded are 
    Magic: The Gathering and absolutely necessary Windows 95 programs.

    In addition, make sure that your hard drive has at least 30MB 
    free for the Windows 95 swap file. 

    Make sure that you have the latest versions of the DirectX 
    drivers for your hardware. In particular, make sure that your 
    sound card has the latest DirectSound drivers.

    Check your sound card to see if it is properly installed and 
    functioning. You can check your sound card by selecting "System" 
    from the Control Panel. Select the Device Manager tab at the System 
    Properties dialog box. Double-click on "Sound, Video and Game 
    Controllers." A description for your sound card should appear 
    underneath. Double-click on this text to bring up a dialog box 
    telling you if your sound card is functioning correctly or not.

    You can also try turning down the graphics acceleration by 
    selecting the System control panel and clicking on the Performance 
    tab. Then click on the "Graphics" button and slide the bar to the 
    left in order to adjust the degree of hardware acceleration. We 
    recommend lowering the graphics acceleration to "Most accelerator 
    functions." If that doesn't help, try "Basic accelerator functions."

Q:  The game is locking up when I try to load a saved game, or I am 
    experiencing "Page fault" errors.

A:  Magic: The Gathering requires at least 30MB of virtual memory. 
    Please check your virtual memory by selecting the System control 
    panel. Then click on the Performance tab and click the "Virtual 
    Memory" button. 

Q:  I am receiving an error message of "Error vids:iv41 decompression."

A:  This error is caused by the Indeo video drivers. Please run the 
    SETINDEO.EXE program by inserting your Original Magic CD-ROM, changing
    to your CD-ROM drive and typing "\autoplay\setindeo" [Enter] at the 
    DOS prompt. You need to restart your computer for this change to 
    take effect. If you continue to experience this error message, 
    please download the version of Video for Windows from Intel's Web 
    site at www.intel.com.

Q:  I am having problems with "Assertion failed" or "Add card" error 
    messages while playing in Shandalar.

A:  This problem has been mostly eliminated in this version of the game. 
    If your graphics resolution is set to something higher than 
    1024 x 768, please try lowering it to 1024 x 768. If that doesn't 
    work, please try lowering it to 800 x 600. 

Q:  When I try to run the game, the system crashes with an error message 
    about MSVFW32.DLL.

A:  You need to install Video for Windows. To do this, you must go to 
    the Control Panel and double-click the "Add/Remove Programs" icon. 
    Click on the Windows Setup tab at the top. Click to put a check mark 
    in the Multimedia component. Then click the "Apply" button to 
    install these components. You will probably need to have your 
    Windows 95 CD-ROM or disks handy.

Q:  The game crashes when I get to the Wizard Statistics screen (showing 
    the five major wizards) or when the coin flip animation starts.

A:  We recommend downloading the latest version of the Indeo video 
    drivers from Intel from their Web site at www.intel.com.

    You can also try turning down the graphics acceleration by selecting 
    the System control panel and clicking on the Performance tab. Then 
    click on the "Graphics" button and slide the bar to the left in 
    order to adjust the degree of hardware acceleration. We recommend 
    lowering the graphics acceleration to "Most accelerator functions." 
    If that doesn't help, try "Basic accelerator functions."

Q:  Sometimes the game quits back to Windows unexpectedly.

A:  Applications running in the background may cause the game to close 
    without notice. Please shut down all other applications before 
    running Magic: The Gathering.

Sound
=====
Q:  I don't have a Windows 95-compatible sound card, and the game 
    crashes on me.

A:  You can play Magic: The Gathering without a sound card if you have 
    version 1.1 or greater.  

Q:  I am having problems with the sound in Magic: The Gathering.

A:  Please note that the game requires DirectX version 5.0 or greater 
    working in order to hear the sound and music. If you are not hearing
    any sound, the problem is most likely caused by a problem with DirectX not 
    working with your sound card. Please contact your sound card 
    manufacturer for the latest drivers that are compatible with 
    DirectX. 

Q:  I am hearing white noise when I play a duel.

A:  You may have an older version of the drivers for your sound card. 
    Please contact your sound card manufacturer to obtain the latest 
    DirectX-compatible drivers.

Q:  I am experiencing glitches in the game due to the sound repeating.

A:  Please try first uninstalling the game and then reinstalling the 
    game using the full custom install. This will sometimes take care 
    of the problem.

Q:  I'm having problems with sound in duels.

A:  Try renaming the folder "DuelSounds" in the MAGIC\PROGRAM directory 
    to something else. You will no longer hear any sounds in the duel.

Q:  I am having some problems with sound in the game.

A:  If you have a Sound Blaster AWE32 card, please download the latest 
    drivers from the Creative Labs Web site at www.creaf.com or contact 
    them directly for the latest version of their Windows 95-compliant 
    sound card drivers.

    If you have an Ensoniq ViVo sound card, please download the latest 
    drivers that should fix this problem from Ensoniq's Web site at 
    www.ensoniq.com or contact them directly for the latest version of 
    their Windows 95-compliant sound card drivers. You can also try 
    downloading the Ensoniq Soundscape ViVo driver installer from 
    Gateway which is called "Soundscape InstallWizard for Windows 95 
    version 1.00.09" (SSIWIZ.EXE dated 2/97). This installer is 
    supposed to make sure your Ensoniq sound drivers are up-to-date 
    and correctly installed. 

    If you have a Packard Bell computer with an Aztech Sound Galaxy 
    Washington 16 sound card, please download the latest drivers that 
    should fix this problem from Aztech's Web site at www.aztechca.com 
    or contact them directly for the latest version of their Windows 
    95-compliant sound card drivers.

Q:  I have an IBM Aptiva with an MWave sound card. Every time I run into 
    a creature in Shandalar, the game crashes.

A:  Unfortunately, the MWave sound card is a proprietary sound card that 
    is not explicitly supported by Microsoft's DirectX technology. 
    Please contact the manufacturer of your sound card or Microsoft 
    for details.

Graphics
========
Q:  I am having video glitches or seeing graphic anomalies.

A:  Please try changing your display resolution to either a lower 
    resolution (such as 640 x 480) or higher resolution (such as 
    1024 x 768). 
    
    If at all possible, we recommend running the game in High Color 
    graphics (16-bit color).

    You can also try turning down the graphics acceleration by selecting 
    the System control panel and clicking on the Performance tab. Then 
    click on the "Graphics" button and slide the bar to the left in 
    order to adjust the degree of hardware acceleration. We recommend 
    lowering the graphics acceleration to "Most accelerator functions." 
    If that doesn't help, try "Basic accelerator functions."

    In addition, make sure that you do not have any other programs (such 
    as Norton Crash Protector) running in the background. 

Q:  When I run the game, the screen turns black but I can still move my 
    mouse.

A:  If you are running in 256-color mode, we recommend switching the 
    graphics to 16-bit or 24-bit mode.

Q:  Sometimes when I exit Magic: The Gathering, the colors on my Windows 
    desktop are messed up.

A:  Unfortunately, running Magic: The Gathering in only 256 colors will 
    sometimes cause this problem with the color palette. If at all 
    possible, please switch to High Color graphics (16-bit color).

Q:  I am having problems with the game returning to Windows 
    unexpectedly.

A:  This problem may be caused by graphics hardware acceleration. Please 
    try reducing the graphics acceleration hardware by selecting the 
    System control panel. Then click on the Performance tab and click 
    the "Graphics" button. Slide the bar to the left to lower the 
    graphics acceleration. We recommend lowering the graphics 
    acceleration to "Most accelerator functions." If that doesn't 
    help, try "Basic accelerator functions."

Q:  I am having some problems with my video card in this game.

A:  If you have an ATI Mach 64 video card, please download the latest 
    video drivers from the ATI Web site at www.atitech.ca or contact ATI 
    directly for the latest version of their Windows 95-compliant 
    drivers.
	
    If you have a Diamond Stealth 3D video card, please download the 
    latest video drivers from the Diamond Web site at www.diamondmm.com 
    or contact Diamond directly for the latest version of their Windows 
    95-compliant drivers.

    If you have a Chips and Technology video card, please download the 
    latest video drivers from the Chips and Technology Web site at 
    www.chips.com or contact Chips and Technology directly for the 
    latest version of their Windows 95-compliant drivers.

    If you have a Matrox Millennium video card, please download the 
    latest BIOS update from the Matrox Web site at www.matrox.com or 
    contact Matrox directly for the latest version of their Windows 
    95-compliant drivers. This BIOS update fixes many DirectX-related 
    issues.

Q:  When the coin flip animation starts, the colors become corrupted.

A:  Magic: The Gathering is designed to work in higher color depth video 
    modes. The problem is due to running the game in 256 colors. If at 
    all possible, we recommend running the game in 16-bit or 24-bit 
    color mode.

Q:  Sometimes I get the error message "Video not available, cannot find 
    decompressor."

A:  Go to the Control Panel and double-click the "Add/Remove Programs" 
    icon. Click on the Windows Setup tab at the top. Select the 
    Multimedia component and click the "Details" button. Scroll down 
    the list and click to put a check mark in the box next to "Video 
    Compression." Click the OK button. Finally, click the OK button 
    again. You will probably need to have your Windows 95 CD-ROM or 
    disks handy.

Q:  When I play a game in Shandalar, I am having various problems 
    including with graphics (particularly fonts). For example, text is 
    appearing outside their boxes and I am getting invalid page faults.

A:  The problem is caused by a font conflict. To check for font 
    conflicts, double-click the Fonts control panel. Delete any font 
    that shows a file size of 0K (zero kilobytes). You may also want to 
    copy all the fonts from your MAGIC\PROGRAM folder (TTxxxx.TTF) into 
    your WINDOWS\FONTS folder. 

Gameplay
========
Q:  The computer AI is making strange or seemingly stupid decisions in 
    how it plays its cards.

A:  The computer deliberately makes some mistakes at the easier levels 
    of the game in order to give beginning Magic players a fighting 
    chance. If you feel these AI decisions are annoying, please try 
    increasing the difficulty level.

Q:  When I hit the 'T' key to bring up the taunt window it doesn't do anything.

A:  Since the printing of the manual the Taunt feature was moved from the 'T' key
    and is now acessable by hitting the [F6], [insert], of the single quote [']key.
    Also note that you must have a registered opponent in order for the Taunt feature
    to function. 

The Duel
========
Q:  When I click on the phase bar in a Multi-player game I cannot
    create a green phase stopper.

    Green phase stoppers have ben removed from the multiplayer 
    portion of the game in order to better enable bluffing when 
    playing a human opponent. You now must hit the done button 
    when you want to move to the next phase.

Q:  The game isn't stopping at my upkeep phase. Therefore, I can't do 
    optional upkeep effects such as paying to untap my Brass Man or 
    paying to untap my Paralyzed creature.

A:  You must put a Stop marker on your upkeep phase for the program to 
    stop there. Otherwise, the game will only stop at your upkeep phase 
    for mandatory effects (such as a creature getting a counter for 
    Unstable Mutation or taking damage for Cursed Land). To put a Stop 
    marker on any phase, right-click on the Phase Bar at the phase that 
    you want the game to stop at and select "Mark this phase to always 
    stop" from the pop-up menu. This will mark the Phase Bar with a red 
    dot which means that the program will stop there if you have a 
    valid action. For example, if you want to use a Land Tax or pay 
    to untap your Brass Man, put a Stop marker on the upkeep phase. 
    Please refer to pages 116-118 of the manual for more information.

Q:  When I double-click a card to cast it, it's tapping some lands that 
    I don't want to tap.

A:  You can instead single-click on a card in your hand to cast it. If 
    you single-click, you'll be prompted to click on your mana sources 
    manually. Or you can right-click the lands that you don't want to 
    be automatically tapped and choose "Don't auto tap this card" from 
    the mini-menu.

Q:  I clicked on the title bar of the Spell Chain Window, and it 
    disappeared. How can I get it back?

A:  Just click on the staff icon that's between the two phase bars to 
    make the Spell Chain Window reappear.

Q:  I clicked on the title bar of the Combat Window, and it disappeared. 
    How can I get it back?

A:  Just click on the sword icon that's between the two phase bars to 
    make the Combat Window reappear (see page 126 of the manual).

Q:  I have more than four creatures in combat, but I can't see beyond 
    the first four in the Combat Window.

A:  To scroll the Combat Window, just click on the little white mouse 
    (that's between your creatures and your opponent's). Drag the mouse 
    icon to scroll the Combat Windows so you can see the creatures on 
    the right-hand side.

Q:  When I bring up the Duel, the program isn't listing any decks in 
    the opening dialog box.

A:  When you install the game, the directory name must be no longer 
    than eight (8) characters. Exit the game and rename the directory 
    in Windows 95 to fix this problem. This problem has been fixed in 
    version 1.1 of the game.

Shandalar
=========
Q:  I am just starting in Shandalar and am getting beaten up on a 
    regular basis by all those creatures that keep attacking me. What 
    can I do?

A:  One of your first steps should be to run to the nearest village or 
    city as fast as possible so you can edit your starting deck. You 
    want to edit your deck to the best cards possible while remembering 
    the minimum deck size depending on your difficulty level (30 cards 
    for Apprentice, 35 for Magician, 40 for both Sorcerer and Wizard). 
    If at all possible, reduce the number of colors in your deck to two 
    or at the most three.

    Next, pick your battles wisely. As the manual says on page 41, the 
    weakest creatures are those that walk. The next hardest are those 
    that are mounted, and the toughest are those look like dragons or 
    other flying creatures.

    Try to run away from the more powerful creatures until you build up 
    a stronger deck. You can run away by moving to a terrain type that 
    the creature can't follow you onto. Each creature sticks to the 
    terrain type corresponding to its color (for example, red creatures 
    stay near mountains). Of course, some creatures are multicolored 
    and can survive on two different terrain types. Once you cross a 
    terrain boundary, the creature will be unable to follow you.

    Stay on the roads if at all possible. You will move much faster if 
    you walk on roads and can many times outrun the other creatures. 

    Don't forget that you can bribe many creatures. Discretion is 
    sometimes the better part of valor, and if you can afford the gold, 
    it's probably better than losing a card from your deck in ante.

    As your deck becomes stronger and you beat more creatures, you'll 
    notice that the weaker ones will start running away from you! In 
    addition, you don't have to face as many creatures in version 1.1 
    of the game.

Q:  When I defeat one of the creatures, I see an animation of the 
    five major wizards. What do all those things represent?

A:  As noted on page 32 of the manual, each skull represents a victory 
    by you against one of that wizard's creatures. The wizards' staffs 
    represent their total life points. Each red dot represents one 
    fewer life point for that wizard. (You can only knock them down 
    by 10 points, though, to a minimum of 20 life.) The tiny floating 
    spheres represent mana taps (see page 46 of the manual). You will 
    lose the battle to save Shandalar if any one wizard accumulates 
    enough mana taps to be able to cast the Spell of Dominion.

Q:  Every time I receive a notice that a wizard is attacking a 
    particular town, I don't arrive in time to save it.

A:  If a wizard gains control of a city, it will be converted into 
    a mana dome. You can still battle the wizard there to free it. 
    You will not regain your mana link, however, if you had one 
    there before. 

Q:  My character keeps moving when I'm walking in the main map of 
    Shandalar even after I stop pressing a directional key.

A:  You can stop your character in two different ways: either press the 
    Spacebar or click on your character's head with the mouse.

Q:  Can I speed up movement while walking around on the main map?

A:  Try pressing Shift-U. This may improve performance on slower 
    computer systems.

Q:  At the Status screen, my Record vs. the Creatures of Shandalar isn't 
    keeping track of all the creatures I have won against in Shandalar.

A:  Only creatures you encounter in the main map count toward your 
    Win/Loss record. Creatures you vanquish in dungeons and castles 
    do not count.

Q:  Is there any way I can load a saved game from within Shandalar 
    without having to quit the game first?

A:  If you update to version 1.1 of the game, you can right-click to 
    bring up a mini-menu with options to save, load, quit, etc.


Decks in Shandalar
------------------
Q:  I'm building a deck in Shandalar but there seem to be limits on how 
    many duplicates of one card I can put into my deck.

A:  The number of duplicate cards 
    allowed in your Shandalar deck depends on the total size of your 
    deck and whether you have the Tome of Enlightenment (one of the 
    World Magics). If you don't have the Tome of Enlightenment, you can 
    only have one card if your deck has 19 or fewer cards, 2 duplicate 
    cards if your deck has 20 to 39 cards, 3 duplicates if your deck 
    has 40 to 59 cards, or 4 duplicates if your deck is over 60 cards.

Q:  Even though I've edited my deck and know exactly which cards are in 
    it, I keep getting random land.

A:  If your deck does not have the 
    minimum number of cards in it, random basic lands will be 
    temporarily added to your library (after the ante but before 
    the shuffle). Your deck must have 30 cards at Apprentice level, 
    35 at Magician level, or 40 at both Sorcerer and Wizard level.

Q:  Every time I try to look at my deck or edit my deck in Shandalar, 
    the game crashes.

A:  The game will now notify you when you approach the 500 card limit 
    and advise you to sell some cards. 

The Cards
=========
Q:  How can I use my Mishra's Factory to attack or block?

A:  You must turn your Mishra's Factory into an Assembly Worker before 
    you try to attack since no fast effects are possible between 
    declaring an attack and declaring attackers. If you wish to 
    attack with your Mishra's Factory, in your Main phase (precombat), 
    single-click the card and choose "Change to Assembly Worker." Pay 
    the 1 colorless mana and your Mishra's Factory will then turn 
    into an Assembly Worker (a 2/2 artifact creature). As long as 
    your Assembly Worker is not suffering from summoning sickness, 
    you can attack with it by clicking the Combat phase on the Phase 
    Bar. When prompted to "Choose attackers," click on the Assembly 
    Worker card. 

    You can also use your Assembly Worker to block the computer's 
    creatures. Just turn your Mishra's Factory into an Assembly Worker 
    in the "Attacker fast effects phase." 

Q:  Why does Naf's Asp continue to damage me turn after turn?

A:  According to the latest rulings by Wizards of the Coast for the 
    Naf's Asp, you must pay to get rid of the Naf's Asp legacy to avoid
    further damage. Otherwise, you will continue to take damage turn 
    after turn.

Multi Player Supplement
=======================

== THE NEW MAIN MENU ==

As soon as you fire up Duels of the Planeswalkers, those of you who played the original 
Magic: The Gathering will notice two subtle changes. First of all, there's an unusual icon
in one corner of the screen. That's the ManaLink icon, and it's described in detail in the
Additions section. The other thing is that the Main Menu has been replaced. In its place
are the five option spheres.

The spheres are more like categories than options. Each one gives you access to a
particular segment of the Magic: The Gathering experience. When you click on any sphere,
all the activities that fall within that category appear on the right side of the screen,
in the upper area. Note that even when you have a sphere selected, all of the other
spheres are still available. You can switch from sphere to sphere with just a click, or
you can deselect the current sphere by clicking on it again.

Duel   This sphere encompasses the Solo Duel, the Gauntlet, and the new Duel Opponent
       feature, which is described later, in Additions.
Tools  All of the utilities--useful things like the Deck Builder--are collected in this
       sphere.
World  The world of Shandalar and the introductory animation that accompanies it reside
       in this sphere.
Help   This sphere includes all of the on-line sources of information, such as the Help
       system, the Tutorial, the Readme file, and the game Credits.
Player The set-up for your dueling persona, all the statistics, and any other record-
       keeping functions are contained in this sphere.

To select a displayed activity as the current one, click on it. Any settings and options
associated with the currently selected activity appear in the larger area below the list
of activities. (The options for those activities with which you're already familiar from
the original game have not changed significantly. The options for the new activities are
explained in the relevant parts of the Additions section.)

== Making a Name for Yourself ==

Before you can start racking up a brilliant record of sequential wins online (more about
that a little later), you must create a player persona, what we call your Screen Name.
Why? Well, in order to keep a record, there must be a name with which to associate that
record. As a bonus, you can also determine what your character looks like and give
yourself a tag line like all the AI characters have.

To get started on your screen name, select the Screen Name activity in the Player sphere.

Screen Name   When you have selected a player number (see below), you see the corresponding
              Screen Name. (At first, they're all filled with default names.) If you want
              to change the displayed name, simply click anywhere in the text box and begin
              typing.
              Next to the Screen Name, there is a box you can use to associate a particular
              character image with the player number you have chosen. Select any one of the
              existing faces or, if you don't care for any of them, use the new Face
              Builder tool to create new ones.
Date          This simply notes when this particular screen name was created.
Real Name     Since a screen name is more often than not a pseudonym, you might want to
              associate your real name with a character. After all, you created it and
              you'll likely spend quite some time building a record.
E-Mail        If you'd like online opponents to be able to communicate with you when you're
              not connected, you can enter your e-mail address here.
Quote         All the built-in characters have quotes associated with them. It's only fair
              that you have the same opportunity to express yourself.
New Player    Each screen name is associated with a Player Name. Before you do anything
              else, you should choose a name to work with. If no screen names have been
              set up yet, you need to click the new player button. 
Delete Player Use the Delete Player button only if you wish to permanently erase the record
              associated with the current player name. If you want to change the info for an 
              existing screen name without erasing the record, simply enter new text in the 
              boxes -- do not use this button.

NOTE          When you sign on to the Total Entertainment Network to play Magic: The Gathering 
              Online your TEN login name is added to your player list and is used whenever you
              sign on. Even if this name is deleted from your computer using the delete player
              button, it will still be available when you are playing on TEN.

== Manalink: Multiplayer Dueling ==

Here's the game you've been waiting for. You've had plenty of time to practice on the
computer; now let's see how well you fare against the malicious cunning of other humans.

When you start up Magic, you also activate ManaLink, the multiplayer part of the game.
You'll know it's active because there's a jewel-like icon in the corner; that's the
ManaLink icon. This icon is a convenient way to tell your connection status at a glance.
Double-click on the icon to see the other half of ManaLink, the ManaLink Dialogue. What
this looks like depends on whether you're already connected or not. This dialogue is the 
central control point for all multiplayer functions except setting up the actual match.

You set up the match using Duel Opponent, the newest activity in the Duel Sphere. That's
getting ahead of ourselves, though. First, let's go over all the basic steps to starting
a multiplayer match:

* Set up your screen name and all the information that goes with it. (At this point, you
  might also want to build a few dueling decks.)
* Connect to another player (or a community of players) through your chosen communications
  medium (modem, network, direct cable, etc.).
* Invite another connected player to a match. Once he or she accepts the invitation, the two of
  you become registered opponents. 
* Choose your dueling deck for the match.
* One player proposes the parameters for the match. The other player can accept or decline. 
  When both players agree to the parameters, the match starts immediately.
* Do the duels.
* Afterwards, each player's record and ranking are recalculated.


NOTE - Depending on the duel parameters not all decks will show up in the deck list. For example,
       if you have chosen to play with a type 1 restricted deck then Wild and Unrestricted decks 
       will not appear in the deck list. A list of banned and restricted cards can be found at 
       the end of this file.


=== GETTING CONNECTED ===

    <Connecting via LAN>

A Local Area Network (LAN) can provide one of the most convenient environments for
multiplayer dueling; it's the best of both worlds. Networks give you the flexibility of
having multiple potential opponents (like Internet play), and network connections are
often more stable and communications faster than on the Internet. Then there's that mighty 
convenient Autoconnect feature...

    Auto-connect

If your computer is attached to an active LAN, Manalink attempts to connect you as soon
as you start up Magic: The Gathering.  

________________________________________________________________________

Manalink will always make the attempt to auto-connect to a LAN unless you give it other
orders. To disable the auto-connect feature:

* Right-click on the ManaLink icon.
* At the mini-menu that opens, select Preferences.
* At the second mini-menu, select Options.
* In the Options box, click the Use Autoconnect checkbox (the box should become empty).
* Click the Apply button.

As long as you leave this disabled, Manalink will not attempt to auto-connect you.

    Manually

It's also fairly easy to initiate a connection when you are not connected (that is, the
ManaLink icon is in the No Net state). To initiate a LAN connection manually:

* Double-click on the ManaLink icon to open the Manalink Dialogue (unless it's already
  open, of course).
* Select the type of connection you want to make--LAN. (Click on that type in the list.)
* Click the Connect button.

Now that you're in contact with other players, you're ready to set up a duel.

    <Connecting Modem to Modem>

Playing via modem is a good option for those of you who don't have access to a network,
aren't close enough to one another to use a cable, and don't enjoy the crowds on the
Internet. The one possible disadvantage is that you only have one potential opponent--the
person at the other end of the line.

Obviously, if you expect to play via modem, both computers must have working modems
attached to them (and to functional phone lines).

To begin, select the Modem Connection option and click the Connect button. One player must
select to Answer (await the other player's call), and the other must select Dial (initiate
communication). The two of you should decide who will do which ahead of time so as to
avoid confusion. Next, both players must choose the modem they intend to use.

The dialing player must enter the phone number to call. When that's done, you must
invite your opponent to a duel. Once he accepts you are registered opponents and are ready
to duel.

    <Connecting through the Internet>

Internet play promises to deliver a whole world of challengers and champion players--not
to mention rules experts you can consult and company representatives ready to help you
with problems. The possibilities are virtually boundless, so let's get started.

There are a couple of necessary preparation steps you must complete before you can
successfully start Internet play on the Total Entertainment Network (TEN). Chances are
some players have already taken care of these, but for new players, here's the list:

1) You must have access to the Internet; this game will not do that for you.
2) You should set up a screen name for yourself, and maybe build a few decks.

Once that's done, it's time to get online. To initiate an Internet connection:

* Double-click on the ManaLink icon to open the Manalink Dialogue (unless it's already
  open, of course).
* Select the type of connection you want to make--'Play on GatheringNet' . (Click on that type in the list.)
* Click the Connect button.

If you have never used TEN before, you must install the TEN software and set up your TEN
membership. 

* Open the folder on your Duels of the Planeswalkers CD named Ten. Inside that folder 
  is a file named SETUP.EXE - double click that file to install the Total Entertainment 
  Network software.
* The installation program begins automatically. 
* Create a TEN membership for yourself. (You must be a member to use the TEN online
  services, including Magic.) If you have questions about registration, please consult
  the TEN Help feature or call TEN Customer Service. Registration is a simple, three-step
  process:
      1) Read and accept the Terms of Service.
      2) Enter your name and address.
      3) Enter your Screen Name and create a Password. 

<Connecting with a Serial or Null Modem Cable>

Playing via cable is likely to be the fastest method of communication. There are no
middle-men, just the two computers with a wire between them. The disadvantage, of course,
is that you only have one potential opponent¾the person at the other end of the cable.

Obviously, if you expect to play via serial or null modem cable, you must have the cable
hooked up before you begin.

When you select the Serial Port/Null Modem Cable option and click the Connect button,
you're prompted to choose the settings for the communication. With one exception, Port,
both players must choose the exact same settings.

Port       This tells Manalink which of your computer's communications outlets (ports) you
           have the cable attached to. (Most computers have ports named COM1, COM2, COM3,
           and COM4.) This is the only setting that may be different for each player.
Baud Rate  Determines the speed of the communication between the two computers. Both
           players should set this at the highest setting allowed by the slower computer.
           (Generally, anything 9600 or above provides good, smooth play.) Both players must 
           choose the exact same settings.
Stop Bits  This is something technical that only people with nothing better to do
           understand. If you leave it on the default setting, nothing bad happens.
           Both players must choose the exact same settings.
Parity     Parity is another one of those technical things. Don't change the default
           setting and nobody gets hurt. Both players must choose the exact same settings.
Flow       More technical junk here. (These things always come in threes, don't they?) If
           you leave it alone, it works. Both players must choose the exact same settings.

When the settings are correct, click the OK button to make the connection. That's all
there is to it. Then you must invite your opponent to a duel. Once he accepts you are 
registered opponents and are ready to duel.

    <Disconnecting>

If you are connected and decide you would rather not be, it's easy to remedy the situation.
To disconnect from whatever connection you have established:

* Double-click on the ManaLink icon to open the Manalink Dialogue (unless it's already
  open, of course).
* Click the Disconnect button.

That's all there is to it.


Before we get into the details, some of you probably want to just jump right into a duel
and read about it later. (Those of you with some self-control should keep reading.) If
you've already got an opponent lined up and can't wait to get started, here's a quick
step-by-step:

* Both players should already have Magic: The Gathering installed and running. (That might
  sound obvious, but you know how some people are.)
* Whatever communication method you're using--modems, network, cable, or whatever--should
  be in place.
* Follow the connection procedure for the type of connection you're using. (Turn back a
  few pages to the appropriate Connecting section.)
* If you're using a LAN or Internet connection:
     >  Both of you must double-click on the ManaLink icon.
     >  One of you must select the other's name from the list and click the Invite button.
        (The other one should just wait.)
     >  The challenge goes to your opponent (the one who waited), who must double-click on
        the invitation in the Message window, then click the Accept button.
     > When you receive notice of the acceptance, you should select a deck to use in the
       duel (if you haven't already). The two of you are now registered as opponents.
* Now set the Match Parameters for the duel. (For details on the parameters, please refer
  to the Online Dueling section.) Click on Send Parameters to transmit the parameters to
  your opponent. (You can still change your deck, but not for long.)
* Your opponent either clicks Agree to accept your parameters or Disagree to reject them.
  If he disagrees you will have to keep sending new parameters until you come to an agreement.


As soon as the parameters are accepted, the match begins with the selected decks.

== The Manalink Icon ==

The more astute among you will have noticed that when you start up ManaLink, an unusual
icon appears on-screen and stays there. It's called the ManaLink icon, and it's absolutely
essential for multiplayer Magic. As noted in earlier sections, the ManaLink icon is your
gateway to all of the different methods of connecting with other players. Above and beyond
that, it serves while you are connected as both your access point to the ManaLink Dialogue
and your social secretary.

If you don't like the ManaLink icon sitting in the default placement, don't fret. You can
move it anywhere on the screen that you want it, and it stays there until you move it
again (or close it).

The ManaLink Dialogue is covered a little further on (in a section called, remarkably
enough, "The ManaLink Dialogue"). The social secretary part is next. The icon has several
different states, each of which gives you information as to what's going on online.

    States

The ManaLink icon sits wherever you placed it and monitors the online environment for you.
Depending on what's going on out there, it changes its look--goes into a different
"state"--so that you can tell the situation at a glance. These are the possible states and
what each tells you:

No Net    is pretty self-explanatory; you're not connected. If you get disconnected
          unexpectedly, there is a sound cue to warn you.
Net       tells you that Manalink has successfully connected to either a LAN or the
          Internet, but that there are no other players present at the moment. (If you're
          playing via modem or cable, you should never see this state.)
Users     is just like the Net state, except that in this case you are connected and there
          are other players present. (If you're playing via modem or cable, you should
          never see this state.)
Opponent  indicates that you are registered with an opponent and are preparing to start
          dueling. If you're playing via modem or cable, you'll enter this state as soon
          as you connect with the other player.
Drop      If your registered opponent becomes disconnected (for whatever reason), you go
          into the Drop state temporarily. This really just serves to let you know what's
          going on; there's nothing you can do about it. After a few seconds, you should
          return to some other state. A LAN or Internet connection switches to Net or
          Users; one-on-one connections change to No Net.
<         When the left facet of the ManaLink icon lights up yellow, that means that
          someone has invited you to a Chat session. (To answer the invitation, open the
          Manalink Dialogue and double-click on the chat request in the Message window.)
          A chat request persists until everyone involved in that particular chat session
          leaves it. Note that if you receive another invitation after the facet is
          already lit, there is a sound cue to notify you, but the icon does not show any
          change. For a little more detail, please see the "Talking to Other Players"
          section.
^         If the top facet of the ManaLink icon lights up red, that means that someone has
          challenged you to a duel. (To answer the invitation and register as that person's
          opponent, open the Manalink Dialogue and double-click on the challenge in the
          Message window.) A challenge persists until it is answered or withdrawn. As soon
          as you accept an invitation to duel, any other pending challenges are erased,
          and you cannot receive new ones until you are no longer registered with an
          opponent. Note also that if you receive another challenge after the facet is
          already lit, there is a sound cue to notify you, but the icon does not show any
          change.
>         At times, the right facet of the icon lights up blue. This is an indication that
          a player you have marked as a friend is online with you. For the scoop on
          friends, read "The ManaLink Dialogue".
_         If you're the popular type, the bottom facet of your icon lights up green. That
          means that someone has sent you a message. To read the message, open the
          ManaLink Dialogue and check the Message window (the lower portion of the
          dialogue). For the details (like how to erase or reply to a message once you've
          read it), please see the "Talking to Other Players" section.

    Preferences

Naturally, the ManaLink icon doesn't perform all these functions without some guidance
from you. You have some control over how it works (and you can always turn it off, too).
Right-click on the ManaLink icon, then select Preferences from the mini-menu. At the
second mini-menu, select Options. (You can also get to the Preferences window by clicking
the Options button in the ManaLink Dialogue.)

System         This option controls whether you get system messages sent to you.
Messages       These are the messages that appear at the top of the screen.
               To toggle this preference, click the check box labeled Show System Messages 
               or select Show Messages from the (second) mini-menu.
Sounds         Sometimes, the ManaLink icon makes noises. Generally, this is to notify you
               that something is happening--another player is inviting you to a duel or a
               chat, for instance. The sound cues can be useful, but there are times when
               you don't want to be bothered by them. This option allows you to turn them
               on and off. To toggle this preference, click the check box labeled Play
               Sounds or select Play Sounds from the (second) mini-menu.
Availability   Just because you're online doesn't mean you want to duel anybody who comes
               along. If you're busy with something and don't want to be disturbed, you
               can make yourself not available to other players. To set your availability
               status, click the appropriate radio button (I am available or I am
               unavailable), or you can use the Do Not Disturb option on the (second)
               mini-menu as a toggle.
Autoconnect    If you're connected to a LAN, Manalink attempts to connect you every time
               you start up the game. If this is not what you want, you can disable the
               auto-connect feature. To turn this one on or off, click the check box
               labeled Use Autoconnect; there is no option on the mini-menu for this.
Badges         There's a box in the middle of the Preferences window that provides an easy
               way to review and manage the badges you have assigned to other players.
               (If you don't know what badges are, don't worry; they're explained just a
               few pages from here, in Who's Who.) If you select a player--by clicking on
               a name in the box--the Delete button removes any badge you have given that
               player. The Delete All button does just what it says; it removes all the
               badges you have given every player in the list.

When you're finished at the Preferences window, click Apply to make the changes or Cancel
to ignore your changes and close the window. (Any changes you make on the mini-menu take
effect immediately.)

    <The ManaLink Dialogue>

Double-click on the ManaLink icon, and the real meat of the multiplayer game opens up.
The ManaLink Dialogue is your control center for all of the major features and functions
you'll need in the online community.

A couple of notes:

* If you're not already connected, double-clicking on the ManaLink icon won't bring up the
  ManaLink Dialogue, it'll open the Connect window.
* If you're connected using a one-to-one communications method--modem or cable--there is only
  one other person in the player list at any given time. That means that some of the features 
  of the ManaLink Dialogue--the ones designed for an environment with many players--won't be 
  available to you.

The largest portion of the dialogue window is taken up by the player listings. This tells
you who's online with you and gives you a bunch of information about each player. The
scoop is in "Who's Who". Below the listings and all the function buttons is the Message
window. This is where you'll receive notes from other players. The details are covered in
"Talking to Other Players".

Last, but not least, are the buttons spread around the dialogue. What these buttons offer
is not just the basic communications options (messaging, chatting, and so on) and the
necessary functions (invite someone to duel, disconnect), but also a few convenient
features that our online research has led us to believe you'll appreciate.

Available       When you feel you're ready to go up against another player in a match, you
                have two options: you can challenge someone (see Invite), or you can use
                this button to advertise that you want an opponent. This changes your
                status to Available. (The details about your status are in "Who's Who".)
                Keep in mind that as soon as you accept a challenge, any other invitations
                are automatically declined for you until you finish the match or
                Unregister.
Chat            Sending messages is fine, but if you want to have a conversation, you can
                request a private chat with someone. First, select a player from the list
                by clicking on that person's name listing. (Make sure you select someone
                who is not marked with Do Not Disturb status.) Next, click the Chat button.
                Your request goes out immediately.
Disconnect      This one is fairly obvious; click Disconnect to sever your connection with
                whatever sort of communications link you're using. This does not shut down
                ManaLink; it only cuts off the communications connection.
Filters         When you get into an online community, there can be an awful lot of people
                in the player listing. If the size of the list gets to be a problem, you
                can use the Player Filters to decide who is and is not listed on your
                screen. Click the Filters button to work with the filter options. (The
                details are in "Filtering the List".)
Friend          It's a fact of life (especially online) that some people are more
                interesting than others. When you find one of those people, you can mark
                that player as an online friend. First, select a player from the list by
                clicking on that person's name listing. Next, click the Friend button. The
                friend badge appears next to that player's name. (For the skinny on badges,
                please read "Who's Who".) Friends are always listed near the top of the
                player listings.
Don't Disturb   If (for whatever reason) you do not want to chat with or duel anybody, you
                can prevent unwanted invitations by changing your status to Do Not Disturb.
                (The details about your status are in "Who's Who".) Just click the Don't
                Disturb button. As long as you leave this on, you receive no chat requests
                or duel invitations. You do still get messages, however (which you should
                feel free to ignore).
Info            Use this button to get the lowdown on a particular player--real name,
                ranking, record, and all that good stuff. First, select a player from the
                list by clicking on that person's name listing. Next, click the Info
                button. The whole story on what you'll find out is in "Who's Who".
Invite          This is how you challenge another player to a match. First, select a player
                from the list by clicking on that person's name listing. (Make sure you
                select someone who is not already registered with an opponent, in the
                midst of a duel, or marked with Do Not Disturb status.) Next, click the
                Invite button. Your invitation has been sent.
Message         This is how you send a brief text message to another player. First, select
                a player from the list by clicking on that person's name listing. Next,
                click the Message button. When the text box appears, go ahead and type in
                whatever it is you want to say to that person. To finish and send the
                message, click Send (or click Cancel if you change you mind).
Mute            If for any reason you do not want to receive communications from a certain
                player, you can tape that player's mouth shut with the Mute button. First,
                select a player from the list by clicking on that person's name listing.
                (There are some players you cannot mute; read "Who's Who" for the details.)
                Next, click the Mute button. As long as you leave the mute on, you receive
                no messages, chat requests, or duel invitations from that player. Also,
                muted players are dropped to the bottom of the player listings.
Help            This changes your status to Needs Help. (For the whole scoop on status, 
                refer to "Who's Who".) Essentially, this alerts the online rules experts
                and service folks that you have a question or a problem.
Refresh         The player list is up to date when you connect, but it is not updated for
                you. Every once in a while, you'll want to see what's changed--who's new,
                whose status has changed, and so on. To update your information, click the
                Refresh button.
Unregister      If you are registered with an opponent, but you decide that you don't want
                to duel that person after all (or need to undo the registration for any
                other reason), this is the button for you. Clicking this immediately
                releases you (and the other player) from the registration.

=== Deck Types ===

The options on the right determine what sort of decks are allowed in the match. There are
five possibilities:

Unrestricted  This is a total free-for-all. Unrestricted decks may include any card in the
              game and as many of each card as you care to add.
Wild          The only difference between Wild and Unrestricted is that no deck may include
              more than 4 of any card. All cards, including restricted and banned cards,
              are still allowed.
Restricted    This deck type allows no more than 4 of any card, like Wild. The difference
              is that Restricted decks may include only 1 of each restricted card and no
              banned cards. (Those of you who are familiar with tournament play will
              recognize this as the definition of a Type 1 deck.)
Tournament    The Tournament type of deck may include no restricted or banned cards at all.
              Otherwise, like Restricted and Wild, it allows up to 4 of any other card.
              (Those of you who are familiar with tournament play will recognize this as
              the definition of a Type 1.5 deck.)
Highlander    In a Highlander deck, you are allowed only 1 of each card. Restricted and
              banned cards may be included.

________________________________________________________________________
                     Restricted and Banned Cards

Some of you are scratching your heads and asking, "What's a restricted card?
How do I know which cards are banned? Good questions.

* Wizards of the Coast decides which cards are banned and restricted;
  these decisions are generally based on the fact that the card is too
  powerful, contradicts the rules, or otherwise unbalances the game in some
  way.
* The banned and restricted list for ManaLink is in the Appendix. This list
  is based on Wizards' current list as of the time Duels was created.

== NAUGHTY CARDS ==

These lists include all of the cards that are restricted and banned for Restricted and
Tournament decks. In addition, the ante cards are considered banned for non-ante play but
unrestricted for ante games.

    [Restricted]

Ancestral Recall
Balance
Berserk
Black Lotus
Black Vise
Braingeyser
Demonic Tutor
Fastbond
Fork
Ivory Tower
Library of Alexandria
Mox Emerald
Mox Jet
Mox Pearl
Mox Ruby
Mox Sapphire
Regrowth
Sol Ring
Strip Mine
Time Walk
Timetwister
Wheel of Fortune

    [Banned]

Channel
Mind Twist
Time Vault

    [Ante Cards]

Bronze Tablet
Contract from Below
Darkpact
Demonic Attorney
Jeweled Bird
Rebirth
Tempest Efreet

== CREDITS ==

Producer
   Alessandro De Lucia

Game Design
   David Etheredge


Programming
   Robert Colbert
   Chris Taormino
   Jim Thomas

Additional Programming
   Kim Crouse
   Kevin Ray

Art
   Frank Frazier, Lead Artist
   Todd Bilger
   Betsy Kirk


Sound Recording and Engineering
   Mark Reis


Documentation
   John Possidente


Product Marketing Managers
   Steve Haney, US
   Lisa Humphries, Europe

Creative Services
   Kathryn Lynch
   Jerome Paterno
   Rick Rasay
   Reiko Yamamoto


Quality Assurance
   Tom Falzone, Supervisor
   Chrispy Bowling, Project Lead
   Kevin Bane, Assistant Lead

QA Staff
   Bob Abe
   Paul Ambrose
   Tim Beggs
   Matt Bittman   
   Brandi Boone
   Ellie Crawley
   Jim Crawley   
   Alan Denham   
   Mike Dubose
   Grant Frazier
   Michael Gibbons
   Mark Gutknecht
   Rosalie Kofsky
   Jason Lego   
   Tim McCracken
   Roscoe Possidente
   Steve Purdie
   John Ross
   Rick Saffery
   Dean Schwarzkopf
   Mike Seal
   Jeff Smith
   
Wizards of the Coast Liaisons
   René Flores
   Emily Arons
   Jim Butler

Very Special Thanks
   Daniel Berner
   Jan-Maree Bourgeois
   Shirley Carlson
   Doru Culiac
   Skaff Elias
   Chaz Elliott
   Karen Ffinch
   Karol Fuentes
   Karen Kapscady
   Mendy Lowe
   Johanna Mead
   Joel Mick
   Yasuyo Nohara
   Marisa Ong 
   Juliane Parsons
   Lenny Raymond
   Roland Rizzo
   Bill Rose
   Henry Stern
   Rob Stewart
   Pete Venters
   Rob Voce
   Tom Wylie



