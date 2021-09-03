The Manalink PlayDeck Analyser v0.5

Written for the CCGHQ Manalink Forums @ slightlymagic.net
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

INTRODUCTION
------------
The Manalink PlayDeck Analyser is a small app that corrects PlayDeck.dck files.
It does this by comparing them against a specific Cards.dat\Manalink.csv file.
If the CardName in the PlayDeck doesn't match the CardID# in the DAT\CSV,
then the entire DAT\CSV is searched for a matching CardName and the CardID#
in the PlayDeck is corrected if a proper match can be found.
It also performs a small host of other Manalink-related functions.

Besides correcting CardNames and CardID#s, PDAnalyser v0.1 is able to:
   Identify and use both Manalink 2.0 and 3.0 Manalink.csv files
   Identify PlayDecks that violate Banned\Restricted Lists
   Identify PlayDecks that violate a Minimum Deck Size
   Swap cards into and out of PlayDecks using Card Substitution Lists
   Sort cards in PlayDecks by CardColor, CardQuantity, CardName or CardID#
   Give PlayDecks a correct, accurate and consistent Color Description
   Choose the % of Non-Land cards needed to describe a deck as "Artifact"
   Check and correct PlayDeck Headers to match Manalink's DeckBuilder Format
   
Now, Manalink PlayDeck Analyser v0.2 can also:
   Import PlayDecks from MagicWorkStation and Apprentice
   Import PlayDecks from OCTaGoN and MtG: Interactive Encyclopedia
   Import PlayDecks from plain text (NetDeck): <CardQty> <SPACE> <CardName>
   Export Manalink.csv as PlayDeck (with or without Non-Coded cards)
   Export Manalink.csv as Expansions: PlayDecks and Info, Coded\NonCoded
   Export Manalink.csv as Vertically Oriented DataFile
   Search PlayDeck Headers: Move\Copy files, Match In\Exact\Any\All
   Verify CardImages against DAT\CSV to find Missing or UnMatched files
   Copy CardImages for all CardNames listed in a PlayDeck
   Seperate rebuilt PlayDecks containing NonCoded, UnMatched, etc. CardNames
   Auto-Save and Auto-Reload user settings and the last used Manalink.csv
   Reload the Manalink.csv file 5-6 times faster than before
   
Starting in v0.3, the Manalink PlayDeck Analyser now can:
   Replace individual lines in the PlayDeck Headers with static values
   Read Cards.dat or Manalink.csv for use with most operations
   Automatically load the Cards.dat file if found
   Convert Manalink 2.0 Cards.dat file into Manalink.csv file
   Rebuild Manalink 2.0 CardImages from Manalink 3.0 CardImages
   Operate by Commandline to convert NetDecks without using the GUI
   Boast of many improvements in the Verify and Import functions
   Do all of this 3-5 times faster as a single ~225 Kb executable
   
The new features of Manalink PlayDeck Analyser v0.4 include:
   New CardList format; much more flexible, intuitive and easier to use
   Now Banned, Restricted and Substitute Lists are simple and convenient
   Identifying, Displaying, and Scanning of Banned and Locked Challenge Cards
   External Spelling and CardName corrections CardList for UnMatched cards
   Search PlayDeck contents for a specific CardName and CardQuantity
   Substitute Cards by CardName using the Replace PlayDeck Contents DialogBox
   Select, Create and Manage multiple Target PlayDeck Folders
   Turn On\Off Seperate Target Folders, and select which ones (if any) to use
   NetDeck now uses all settings. CardLists, Sorting, Replace Content, etc.
   NetDeck commandline conversion now supports Wildcards and Relative Paths

Here are a few more additions to Manalink PlayDeck Analyser v0.5:
   Now supports the use of recursive folders in the target PlayDeck folder
   CSV-based Shandalar Import\Export function to customize exe\sve files
   NetDeck will save a copy of new PlayDecks to a chosen default folder
   Rebuild now warns about duplicate FileNames instead of just clobbering them
   MetaPad.exe is now the default embedded editor for most text operations
   Greatly improved LZW decompression routines for all embedded files
   User may now select additional invalid characters for CardImage FileNames
   Improved sorting functions, now they seem to work in all circumstances
   Added ALT-Key shortcuts to practically every available button and widget
   When updating, old settings are saved to .old.ini instead of being clobbered
   Updated defaults to allow for longer DeckNames, new illegal characters, etc.
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

OVERVIEW
--------
Note: This program should always operate only on copies of the source files,
and therefore it will automatically import both PlayDeck.dck files and the
Manalink.csv. Please use the import functions when loading new files, as this
allows the program an attempt to verify their proper format, which helps to
assure more accurate output and prevents the program from choking on
improperly formatted data. If the user chooses to place PlayDeck.dck files
directly into the target PlayDeck folder, it is their responsibility to
assure the proper format of said files. Otherwise, the program may crash.
The PDAnalyser will automatically clobber any same-named files already
present in the import folders. 

Always retain copies of your original source files!

Installation is simple, just extract and run. Uninstallation is equally simple,
just delete the .\PlayDeckAnalyser\ folder, and it's gone.

Using the PDAnalyser is as easy as 1-2-3:

Step 1: Select the Cards.dat or Manalink.csv File
   PDAnalyser attempts to find a .DAT file by recursively searching the current
   folder, and then searching the parent folder. If a valid CARDS.DAT is found,
   it will be automatically loaded. Otherwise, PDAnalyser will attempt to load
   the last-loaded Manalink.csv file. If neither is found, then you must
   select either a Cards.dat file or a Manalink.csv to compare the PlayDecks
   against. If a CSV file is selected from anywhere other than
   .\PlayDeckAnalyser\CSV\ then the program will request a unique filename
   for the CSV. This helps to identify the specific Manalink.csv and allows
   keeping multiple copies of the CSV in the default folder (I would recommend
   using the date for the update... e.g. "20140218v2"). After selecting, the CSV
   file will be checked, verified and saved into the default folder.
   Manalink.csv files are named and copied, but Cards.dat files are not.

Step 2: Select the Target Folder and Import PlayDecks
   The default Target Folder is .\PlayDeckAnalyser\PlayDeck\. If desired,
   the Folder Dialog may be used to either select or create a new Target Folder.
   If any PlayDeck.dck files exist in the new Target Folder, PDAnalyser will
   attempt to import them. Otherwise, another folder must be selected.
   To import new PlayDecks, click "Import PlayDeck" on the Program Menu and
   select "from Manalink", then choose the PlayDeck.dck files you'd like to
   rebuild. They will be verified for format and copied into the Target Folder.

Step 3: Check settings and Rebuild
   Make any additional settings you'd like to use and hit "Rebuild PlayDecks".
   The program will now begin examining and correcting the PlayDeck.dck files.
   
...and that's it. It's compatible with virtually all versions of Manalink
(from v1.3.2 upwards), so as long as the cards in the PlayDecks are available
in the CSV or DAT being used, this program should be able to translate any
PlayDecks created with a wide variety of software to compatible PlayDecks
for the chosen specific version of Manalink.

A LogFile is created each time PlayDecks are Rebuilt. It details all of the
corrections\substitutions made as well as any matches found in the
Banned\Restricted lists. The logfile will be named Date-Time.log, and will be
located in the .\PlayDeckAnalyser\ folder after the rebuild is complete.

The PDAnalyser will auto-save settings after each operation, and reload
those settings when the program is first executed. There are a few
settings available in the PDAnalyser.ini file that are unavailable through the
UserInterface, please view the PDAnalyser.ini file in a TextEditor for details.
(or click "View" on the Program Menu and select "Settings.ini")


PROGRAM ITEMS
-------------
 Buttons:
   \Select DAT or CSV File
      Allows the selection of a Cards.dat or Manalink.csv for use with scanning
      and correcting PlayDecks. Cards.dat files are read from their current
      location, but any Manalink.csv file selected from an exterior location
      will be named, verified and copied to the .\PlayDeckAnalyser\CSV\ folder.
   
   \Select PlayDeck Folder
      Allows the selection or creation of a new Target Folder for the
      Rebuilt PlayDecks. If any PlayDeck.dck files are found in the new
      Target Folder, then the option to import them is given. If refused,
      a different Target Folder must be selected.
      Recursive import of PlayDeck files is currently unsupported.
   
   \Rebuild PlayDecks
      Starts the process of rebuilding the PlayDeck.dck files. This button is
      only selectable after a DAT or CSV file has been loaded and there are
      PlayDeck.dck files present in the PlayDeck Target Folder.
      
   \CardList
      Opens a CardList.txt file for easy alteration or review.
      CardLists are located in the .\PlayDeckAnalyser\Lists\ folder.
      (see appendix "How To Use CardLists")
   
   \Refresh
      Checks\Reloads altered settings and re-initializes the program.
      
   \Abort
      Aborts the currently-running operation. Lengthy operations will
      offer a Confirmation Dialog just to be sure.
   
   \View ReadMe
      Displays the ReadMe.txt file.
      
   \View LogFile
      Displays the last-created LogFile.txt file.
      This option is only available if a last-created LogFile is present.
   
 RadioButtons:
   \Sort PlayDeck By
      Select the sorting method for the rebuilt PlayDecks. "Card Name" and
      "Card ID#" do a straight sort from top to bottom. "Card Color" and
      "Card Quantity" first sort by catagory, and then alphabetically.
      "Don't Sort" will leave the PlayDeck in the original order.

 TextBoxes:
   \Minimum PlayDeck Size
      The minimum number of cards allowed in the scanned PlayDecks.
      If there are fewer cards present, the PlayDeck will be marked as
      under-sized in the LogFile and rebuilt to the appropriate folder.
   
   \Maximum # of Unique Cards
      The maximum number of unique cards allowed in the PlayDecks. Reading
      of PlayDecks will stop at this number, truncating the contents of
      PlayDecks having more unique cards. 200 is the limit imposed by
      Manalink and DeckBuilder so it is the default. Be sure to change this
      value if you are scanning PlayDecks with a larger number of cards.
   
 CheckBox:
   \Use Recursive PlayDeck Folders
      If checked, the current Target PlayDeck folder will be recursively
      searched for PlayDeck.dck files and they will be rebuilt in place.
      This option is incompatible with "Use Seperate Target Folders",
      and that option will be automatically deactivated.
      Recursive import of PlayDeck files is currently unsupported.

   \Scan Restricted CardLists
      If checked, all of the PlayDecks will be scanned for matches in the
      Restricted CardLists found in the .\PlayDeckAnalyser\Lists\
      folder (see appendix "How To Use CardLists"). If any matches are found,
      the PlayDeck will be marked as Banned\Restricted in the LogFile and
      rebuilt to the appropriate folder.
   
   \Scan Substitute CardLists
      If checked, all of the PlayDecks will be scanned for matches in the
      Substitute CardLists found in the .\PlayDeckAnalyser\Lists\ folder
      (see appendix "How To Use CardLists"). If any matches are found, the
      card substitution will be detailed in the LogFile and the PlayDeck
      will be rebuilt to the appropriate folder.
      
   \Scan Locked Challenge Cards
      When checked, a DialogBox appears displaying the location and Locked 
      Challenge Card status of the selected Manalink Program Folder. You may
      change the current Manalink folder by clicking "Select" and choosing a
      new "MAGIC.EXE" file. If accepted, all cards marked as "Locked!" will
      be treated as Banned during the Rebuild process.
      If all Challenge Cards are UnLocked, then "Accept" is unavailable.  
      
   \Rebuild Color Descriptions
      If checked, the Description (Header Line #2) of all the PlayDecks
      will be rebuilt to reflect the CardColors present in each deck.
      Since PlayDeck Descriptions are limited to 18 characters, they will
      be shortened if necessary (or truncated if this option is UnChecked).
      Color Descriptions include:
         Blue/Black/Red/Green/White = Contains MonoColored Cards of this Color.
         Blu/Bk/Rd/Grn/Wht = Shorted versions to fit within the 18 char limit.
         Artifact/Art = Contains more than threshhold % of Artifact + Colorless.
         5 Color  = PlayDeck contains MonoColored Cards of all 5 Colors
         Golden   = All colored cards in PlayDeck are MultiColored
         All Land = PlayDeck contains only Land
         
   \Use Seperate Target Folders
      If active, PlayDecks will be rebuilt to one of the following folders
      based on the severity of the error corrected:
      
      .\PlayDeck\              There were no corrections made.
      .\PlayDeck\FixHeader\    Only the Header was fixed, the Contents were OK.
      .\PlayDeck\FixDeck\      One or more corrections to the PlayDeck Contents.
      .\PlayDeck\Substitute\   One or more cards matched a Substitute List.
      .\PlayDeck\MinDeckSize\  Deck is smaller than the minimum deck size.
      .\PlayDeck\Restricted\   One or more cards matched a Restricted List.
      .\PlayDeck\Banned\       One or more cards matched a Banned List.
      .\PlayDeck\NonCoded\     One or more cards are listed as NonCoded.
      .\PlayDeck\UnMatched\    One or more cards were not found in the CSV.
      
      The option of which Target Folders to use (if any) is given when the
      CheckBox is first selected. This is an ascending hierarchy, meaning that
      the PlayDecks will be rebuilt to the matching folder that is furthest
      down the list. So, if the PlayDeck had an incorrect color description
      (FixHeader), wrong CardID#s (FixDeck), matched a Restricted List
      (Restricted) and contained cards listed as NonCoded in the Manalink.csv
      (NonCoded), then the PlayDeck would be rebuilt to the
      .\PlayDeckAnalyser\PlayDeck\NonCoded\ folder.
      This option is incompatible with "Use Recursive PlayDeck Folders",
      and that option will be automatically deactivated.
         
   \Replace PlayDeck Contents
      If active, then all scanned PlayDecks will have the appropriate lines
      in their Headers replaced with the chosen static values, as well as
      any matching CardNames which are Substituted based on the criteria chosen.
      The Replace Contents Dialog will appear when the CheckBox is selected.
      (see "Replace PlayDeck Contents" for more information)

   
PROGRAM MENU
------------
 View:
   \ReadMe.txt     - View this text file.
   \LogFile.log    - View the most recent LogFile.
   \CardList.txt   - Open a CardList for alteration or review.
   \Settings.ini   - Opens the PDAnalyser.ini file for settings management.
   \SourceCode.bas - Extract the Source Code for PlayDeckAnalyser v0.5
   
 Delete:
   \LogFiles.log  - Delete all LogFiles in the .\PlayDeckAnalyser\ folder.
   \Settings.ini  - Delete the PDAnalyser.ini file and reload default settings.
   \Fixed Imports - Delete all Fixed Import Decks in the .\Import\ folder.

 Action:
   \Verify CardImages - Verify a folder of CardImages against the currently
      loaded DAT\CSV. (see "Verify CardImages" for more information)
      
   \Search PlayDecks  - Open the Search PlayDecks DialogBox.
      (see "Search PlayDecks" for more information)
      
   \Restore Defaults  - Delete PDAnalyser.ini file and reload default settings.
   
   \Decrease fontSize - Decrease the font pointsize by 1.
   \Increase fontSize - Increase the font pointsize by 1.

 Export:
   \as PlayDeck - Build a PlayDeck.dck containing all of the cards in the
      currently loaded Cards.dat\Manalink.csv. If you choose to include
      NonCoded cards, they will be written to the PlayDeck with "0" quantity.
      PlayDeck will be saved in the .\PlayDeckAnalyser\CSV\ folder.
      
   \as Old CardImages - Rebuilds CardImages for the currently loaded DAT\CSV
      in the old Manalink 2.0 naming convention (e.g. "0000A.jpg") using
      current Manalink 3.0 CardImages. If you choose to include cards listed
      as NonCoded, the search will include all cards listed as having images,
      whether they are Coded or not. Rebuilt CardImages will be saved under
      the DAT\CSV name in the .\PlayDeckAnalyser\CSV\ folder.
   
   \CSV as VOD - Save the Manalink.csv as a Vertically Oriented DataFile.
      File will be saved in the .\PlayDeckAnalyser\CSV\ folder.
      
   \CSV as Expansions - Build PlayDecks containing all of the cards from each
      expansion, as well as a CSV and human-readable text file containing
      expansion-rarity info. You may choose to include\not include NonCoded
      cards. Files will be saved in the .\PlayDeckAnalyser\Expansions\ folder.
      
   \DAT as CSV - Converts a selected Manalink 2.0 Cards.dat file to a
      Manalink.csv file, which is then verified and imported into the
      .\PlayDeckAnalyser\CSV\ folder. All Cards.dat files are selectable,
      but only Manalink 2.0 Cards.dat are fully supported.
      Because the location of the CardCoded data for pre-Manalink 2.0
      Cards.dat is still unknown, the CardCoded values from Manalink 1.3.2
      are used during those conversions.
      
 Import PlayDeck:
   \from Manalink - Allows the selection of a folder containing PlayDeck.dck
      files. If the selected folder is within the current Target Folder,
      then the option to Move or Copy is given. Otherwise, any PlayDeck.dck
      files selected from an exterior location will be checked for Unicode,
      verified for format, and then copied to the current Target Folder.
      If any PlayDecks with unrecognized formats are found, they are instead
      copied to the .\Invalid\ PlayDeck folder.
      
   \from Apprentice
   \from MagicWorkStation
   \from MtG: Interactive Encyclopedia
   \from OCTaGoN - Self-explanatory. Import decks from the chosen application.
                   Verified\Converted decks will be saved to the Target Folder.
                   Temporary files will be saved to the .\Import\ folder.
                   Invalid decks will be saved to the .\Import\Invalid\ folder.
                   You must Rebuild the PlayDecks after importing them.
   
   \from NetDeck - Opens the Import NetDeck DialogBox. This allows you to
                   Copy & Paste PlayDecks from the internet and convert quickly
                   and directly to Manalink.dck format. It also keeps all
                   previous NetDecks and allows you to peruse them at will.
                   (see "Import NetDeck" for more information)

 Shandalar:
   \Export Settings - Exports settings from Shandalar.exe and all MAGIC*.SVE
                      files in CSV format. These may be edited and imported.
                      (see "Shandalar Settings" for more information)
   \Import Settings - Imports settings to Shandalar.exe and all MAGIC*.SVE
                      files using the previously exported CSV files.
                      (see "Shandalar Settings" for more information)
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

PROGRAM FUNCTIONS
-----------------


Verify CardImages
-----------------
This function allows you to choose any file in any folder. Once selected,
all files in the same folder with a matching file-extension will be included
(e.g. choose a .JPG file and all *.jpg files will be included, or choose a 
.BMP file and all *.bmp files will be included).

PDAnalyser will parse the "Num Pics" info for each card listed in the DAT\CSV
and build a table of required CardImages, which is then compared to the
contents of the chosen folder. The names of any missing images, unmatched
files, or CodedCards with zero images are recorded to the LogFile.

The option is given to include cards listed as NonCoded. If selected,
the search is expanded to include all cards listed as having images,
whether they are Coded or not.

CardImage files with incorrect capitalization are automatically renamed
to match the DAT\CSV.

Both Manalink 2.0 and 3.0 CardImages are supported. If the first character in
the selected filename is a numeral, then Manalink 2.0 images will be assumed.


Search PlayDecks
----------------
This is a DialogBox that allows you to search the entire contents of all the
PlayDecks in the current Target Folder. Type your search text into the
appropriate TextBox and click "Search". Any PlayDecks matching the search
criteria will be sent to the .\PlayDeckAnalyser\Search\ folder, along with
a LogFile that details the search results.

The Search PlayDecks DialogBox has the following options:

   Search       - This button starts the PlayDeck Search.
   
   Log          - This button displays the last-created Search LogFile.

   Ignore Case  - Ignore capitalization of search text.
   Match Case   - Match the capitalization of search text.
   
   Log Only     - Don't Move or Copy, just create a Log File.
   Copy 'Em     - Copy matched PlayDecks to .\PlayDeckAnalyser\Search\.
   Move 'Em     - Move matched PlayDecks to .\PlayDeckAnalyser\Search\.
   
   Match Any    - Trigger if search text from any TextBox is found.
   Match All    - Trigger only if search text from all TextBoxes is found.
   
   Exact Match  - If checked, PlayDecks must match the search text
                  in the selected TextBox exactly.
                  Otherwise, an inexact match is used.
                 
   Qty Operator - Allows selection of the Boolean Operator to use when
                  examining the Card Quantity of matched CardNames. A blank
                  entry is equivilent to zero (so the default is "Qty > 0").
                  If the CardName is absent and the Quantity is non-zero,
                  then a search of the PlayDeck's total Card Count is used.
   
Note: One of the TextBoxes is disabled because currently all PlayDecks must
have the value "4th Edition" on this line or Manalink will reject it.


Replace PlayDeck Contents
-------------------------
This is a DialogBox that allows you to set static values for lines in the
PlayDeck Headers (The leading semi-colon should not be included). The maximum
number of characters allowed for each entry is displayed to the left of the
TextBox, all entries will be automatically truncated to this length.
These values are selectable using the PDAnalyser.ini file, and will display
as "off" if set to zero.

If there is a value entered for Line#2 (Description), then the
"Rebuild Color Descriptions" CheckBox will be automatically reset.

This Dialog may also be used to make single-card substitutions within the
rebuilt PlayDecks. Both "CardName in PlayDeck" and "CardName to Substitute"
are verified against the currently loaded DAT\CSV, and will be substituted
within the rebuilt PlayDecks up to the quantity selected.

Note: Two of the TextBoxes are disabled because their content is specifically
required by Manalink, and is therefore unchangeable.
   
   
Import NetDeck
--------------
This is a DialogBox that will convert copy\pasted or hand-typed text directly
into Manalink's PlayDeck.dck format. Simply copy\paste\type the PlayDeck
info into the TextEditor in the format <CardQuantity> <SPACE> <CardName>, give
the PlayDeck a name (please don't use the boring default) and click "Convert".

Your text will be saved as "NetDeck_*.txt" and then converted to Apprentice.dec
format (any lines not matching the CardInfo format will be written to the
Apprentice.dec as comments). The deck will be rejected if no usable CardInfo
is found. Trailing letters or symbols on the first word of each line are
allowed (e.g. 20x Forest), but the second word must be equal to the Full
CardName. Next, the Apprentice.dec is converted to PlayDeck.dck format, saved,
and then ran through PlayDeck Analyser to make all corrections. It is then
presented to you for alteration and review.

Manalink PlayDeck.dck format is allowable as input, but this is achieved by
striping the first word from any line beginning with a period (i.e. ".") as
well as all other non-CardInfo from the input text.

The NetDeck DialogBox uses all of the selections made from the Main Program
Interface; so Sorting, CardLists, Color Descriptions, etc, will carry-over to
the NetDeck conversions. Please make all of these selections before opening
the NetDeck Dialog.

A LogFile is created with details of the NetDeck conversion, which can be viewed
by clicking "View Log". If the PlayDeck contains either UnMatched or NonCoded
cards this will be reflected in the LogFile as well as on the StatusLine at the
bottom of the DialogBox. You may now choose to "ReScan" the PlayDeck, "Save" it
directly, go "Back" to the last NetDeck_*.txt, or move to the "Next"
NetDeck_*.txt file. All files are located in .\PlayDeckAnalyser\NetDeck\ under
seperate folders.
   .\PlayDeckAnalyser\NetDeck\txt - NetDeck_*.txt Text files
   .\PlayDeckAnalyser\NetDeck\dec - Apprentice.dec PlayDeck files
   .\PlayDeckAnalyser\NetDeck\dck - Manalink.dck PlayDeck files
    
The Import NetDeck DialogBox has the following options:

   Buttons:
   
   OpenTxt - Open a TextFile and display it in the TextEditor.
   AddQty1 - Add a "1" to all lines not beginning with a number or a period.
   Back    - Go back to the previous NetDeck_*.txt file.
   Next    - Move forward to the next NetDeck_*.txt file.
   Convert - Read the current text and convert to PlayDeck.dck format.
   Clear   - Clear all text from the TextEditor.
   ReScan  - Strip all non-CardInfo from the current text and Convert
   Save    - Save the PlayDeck.dck file to a folder of your choosing.
             This folder is selected each time the NetDeck dialog is opened.
             To choose another folder, you must exit and re-open the dialog.
   
   Program Menu
   
   Edit:
      \Generic (but full-featured) Edit Menu that JustBASIC provides
         with the TextEditor object. It includes Undo, Cut, Copy, Paste, Clear,
         Select All, Print Selection, Find\Replace and Find Again.
         
   Action:
      \Open CardList - Allows the selection of a CardList.txt file, which
         is then opened in a TextEditor for alteration or review.
         
      \Import TextFile - Imports a TextFile into the NetDeck DialogBox and
         displays it for quick alteration and conversion.
      
      \Export as Images - Allows you to choose any files in any folder. Once
         selected, all files in the same folder with a matching file-extension
         will be included. Any files matching CardName info in the TextEditor
         will be copied to the .\PlayDeckAnalyser\NetDeck\jpg\ folder.
         Currently, only Manalink 3.0 CardImage names are supported.
         
   Delete:
      \LogFiles.log
      \Apprentice.dec
      \Manalink.dck
      \NetDeck.txt
      \CardImages - Delete all files in the appropriate folder.
      \All Files  - Delete all files in all folders in the NetDeck folder.
      
      
CommandLine Mode
----------------
If PlayDeckAnalyser.exe is executed with any additional arguments, then no GUI
will be shown and the PDAnalyser operates in complete silence.
PlayDeckAnalyser.exe will accept the following CommandLine switches:

   /c or -c: Convert NetDeck.txt to PlayDeck.dck. Full path may be provided,
             otherwise current folder is assumed. Use this format...
             PlayDeckAnalyser.exe /c [drive:\Full\Path\to\] FileName.txt
             
   /help
   /h or -h
   /? or -?: Display the ReadMe.txt file in a TextWindow.
   
Both recursive pathnames and wildcards in file names are supported:

e.g. PlayDeckAnalyser.exe -c .\NetDeck\txt\NetDeck_*.txt
e.g. PlayDeckAnalyser.exe /c ..\..\SomeFolder\BunchaFiles\*.*

The converted PlayDeck.dck files will be written to the same folder as the
source NetDeck.txt files using the same filenames. A LogFile detailing the
conversion is saved to the .\PlayDeckAnalyser\NetDeck\ folder.
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
| APPENDIX |
------------

Shandalar Settings
------------------

The "Shandalar\Export Settings" function will create CSV files containing info
pulled from the Shandalar.exe and MAGIC*.SVE files. These may then be edited
and the changes incorporated by using the "Shandalar\Import Settings" function.
All of the settings known to this author are included in the generated CSV
files. An exhaustive list of extracted values and their placement within the
source files can be viewed by selecting "Shandalar\Documents" from the menu.

Shandalar.exe yeilds these files:
   .Cards.csv       - All Cards. Name;ID;Color;Cost;Type;Abilities;P/T;etc
   .Enemies.csv     - 55 Enemies. Name;Life;Color;Deck;Speed;Ability;Reward;etc

each MAGIC*.SVE yeilds these files:
   .Dungeons.csv    - 15 Dungeons\Castles. Cards;MapX/Y;NearTown;Clues;Rules;etc
   .Journal.csv     - 2000 Log Entries. LogType;LogContent;MapX;MapY
   .OwnedCards.csv  - 500 Owned Cards. CardName;NotInDeck;InDeck#
   .Towns.csv       - 128 Towns. Name;MapX/Y;Status;Cards;Timers;Visits;etc
   .Victories.csv   - 1000 Win Entries. Color;MonsterType
   .Wandering.csv   - 8 Enemies. WanderType;MapX/Y;Color;Movement;SpawnTimer
   .WorldMagics.csv - 12 Magics. WM Name;Unknown;Price;City#;Duration
   .AllElse.txt     - 53 Settings. Food;Gold;Amulets;Quests;Magics;Gender;etc

Export only requires Shandalar.exe, but the MAGIC*.SVE files cannot be
exported if the file advstrings.txt is not found.
Any of the CSV files may be removed without affecting the import function.


How to Use CardLists
---------------------

CardLists are just plain TextFiles containing Banned, Restricted, and
Substitute card information, and are located in the .\PlayDeckAnalyser\Lists\
folder. They use the following simple format:

   Activation Key
   --------------
   AnyWordYou'dLike = Key$
   
   e.g. ActivateList = true  | Capitalization of Key$ is ignored. Any of these
        List is Active = Yes | are acceptable; only the "= Key$" is required.
        Use this list? = 1   | If Key$ is not found, the CardList is ignored.
        
   Restricted Lists
   ----------------
   MaxQty Allowed;CardName in PlayDeck
   
   e.g. 0;Demonic Attorney     | finds any quantity of this card
        1;Black Lotus          | finds more than one of this card
        10;Forest              | finds more than ten of this card
   
   Substitute Lists
   ----------------
   MaxQty to Sub;CardName in PlayDeck;CardName to Substitute
   
   e.g. 1;Forest;Mox Emerald   | trade up to 1 of the 1st card for the 2nd
        4;Shock;Lightning Bolt | trade up to 4 of the 1st card for the 2nd
        20;Mox Ruby;Mountain   | trade up to 20 of the 1st card for the 2nd
        
All files in .\PlayDeckAnalyser\Lists\ are searched for usable data (except
for the Spelling.txt file). Any lines appearing before a proper Activation Key
is found are ignored, as are all blank lines, any lines starting with
a semi-colon (";"), any lines not starting with a numeral, and any lines
containing an empty "CardName in PlayDeck" field. All CardNames are verified
against the currently loaded DAT\CSV before rebuild begins, and an alert will
be given for any mis-matches found before they are excluded.

Seperate CardLists can be managed by turning-off the Activation Key for those
CardLists you'd like to skip (commenting the line with a semi-colon works
well). Banned, Restricted, and Substitute Lists are managed through the Main
Program Interface and are seperated as follows:

   if the line is...                then PDAnalyser considers it...
   -----------------                -------------------------------
   0;Contract from Below          | Banned List (max of 0 allowed)
   1;Ancestral Recall             | Restricted List (max of 1 allowed)
   2;Force of Nature              | Restricted List (max of 2 allowed)
   4;Wall of Wood;Wall of Vines   | Substitute List (up to 4 will be traded)
   50;Plague Rats;Relentless Rats | Substitute List (up to 50 will be traded)
   
PlayDecks matching Banned or Restricted Lists will be marked as such in the
LogFile and rebuilt to the appropriate Target Folder.

PDAnalyser will only allow a maximum of 4 copies of any card matching a
Substitute List to be present in a PlayDeck. The exceptions are Basic Lands,
Snow-Covered Lands, and cards with the name Relentless Rats.

So if a rebuilt PlayDeck contained:   4 Wall of Wood & 2 Wall of Vines
the above example would change it to: 2 Wall of Wood & 4 Wall of Vines
                                      --------------------------------   
If instead the PlayDeck contained:    3 Wall of Wood & 0 Wall of Vines
the above example would change it to: 0 Wall of Wood & 3 Wall of Vines
                                      --------------------------------
Again, if a PlayDeck contained:       40 Plague Rats & 20 Relentless Rats
the above example would change it to: 0 Plague Rats & 60 Relentless Rats (fun!)
                                      -----------------------------------
Here's an example Substitute List from the default CardList.txt automatically
created by PlayDeck Analyser (to activate select "Scan Substitute CardLists):

--------------------------
Sample Substitute CardList
--------------------------
ListIsActive = True

; Eraticate all Moxen!
;---------------------
50;Mox Emerald;Forest
50;Mox Sapphire;Island
50;Mox Ruby;Mountain
50;Mox Pearl;Plains
50;Mox Jet;Swamp
;---------------------

That CardList will remove all Moxen from all PlayDecks and replace them with
an equal number of the appropriate Basic Land... nice for leveling the playing
field on a whole folder full of PlayDecks.
Next the Substitute CardList gives us:

; Everyone gets a Mox!
;---------------------
1;Forest;Mox Emerald
1;Island;Mox Sapphire
1;Mountain;Mox Ruby
1;Plains;Mox Pearl
1;Swamp;Mox Jet
;---------------------

This will then exchange a single copy of each Basic Land for a Mox of the
appropriate color. Useful for evenly boosting a folder full of PlayDecks.
The default CardList.txt file that is automatically created contains the full
Vintage Banned and Restricted lists, as well as the above SubList sample and
some other useful information. You may open it either in a TextEditor or use
the CardList Button on the Main Program Interface.


Spelling Correction CardList
----------------------------

The Spelling Correction CardList is a TextFile named Spelling.txt located in
the .\PlayDeckAnalyser\Lists\ folder that uses the following simple format:

   Incorrect CardName Spelling;Correct CardName Spelling
   
This CardList is only parsed if the CardName is not found in the DAT\CSV, and
is included in the large battery of checks that the PDAnalyser performs
when confronted with an UnMatched CardName. Any lines not adhering to the
above format are ignored, as are all blank lines, any lines starting with
a semi-colon (";"), and any lines where the two spellings match. It may be
opened in any TextEditor, or you may use the CardList Button on the Main
Program Interface to open it.

This CardList is not checked against the DAT\CSV, so please spell carefully.
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

The Manalink PlayDeck Format
----------------------------

This program will reject as "Invalid" any PlayDecks that do not strictly
adhere to the format produced by Manalink's DeckBuilder. Here is an
example of a valid PlayDeck:

;White Death
;White
;CirothUngol
;CirothUngol@slightlymagic.net
;February 12, 2007
;1
;4th Edition
;

.64     2       Disenchant
.616    4       Divine Transformation
.417    4       Eye for an Eye
.161    2       Mesa Pegasus
.862    4       Rainbow Knights
.213    4       Savannah Lions
.221    4       Serra Angel
.240    4       Swords to Plowshares
.816    4       Thunder Spirit
.283    4       White Knight
.17     1       Black Lotus
.167    1       Mox Pearl
.188    22      Plains

The 8-line PlayDeck header:

;Header Item               |MaxCharacters - Description
--------------------------------------------------------------------------------
;Name of PlayDeck          |28 - Required - Must be equal to FileName
;PlayDeck Description      |18 - Optional - Description seen in Gauntlet\Duel
;Player's Name             |78 - Optional - Name of Player creating the Deck
;Player's email address    |78 - Optional
;Date of PlayDeck Creation |19 - Optional
;PlayFace Value            | 2 - Required - Integer > 0 and < 56 (1-55)
;MtG Edition               |11 - Required - Must be equal to "4th Edition"
;PlayDeck Comments         |398- Optional - Comments seen on Deck Info screen
;Blank Line                | 0 - Required - Separates Header from Contents
--------------------------------------------------------------------------------
The PlayFace Value was used in Microprose's MtG ver1.0 to directly associate
PlayDecks with 1 of 55 different villains, whose face would show whenever
the PlayDeck was used in Gauntlet\Duel. Newly created PlayDecks from
Manalink's DeckBuilder always receive a PlayFace Value of "1".

The Header is followed directly by the Contents of the PlayDeck
listed one card per line in the following format:

.CardID# <TAB> CardQty <TAB> FullCardName

Header items are prefaced with a semi-colon (i.e. ";") and Content items are
prefaced with a period (i.e. "."). The blank line separating them is required.
Although Manalink will allow slight deviations from this format and still
have the PlayDeck be playable, if you load said PlayDeck into DeckBuilder
and re-save, this is the format it creates, so PlayDeck Analyser will attempt
to correct imported decks into this format.
If it can't recognize the source file, it will copy that PlayDeck into the
.\PlayDeck\Invalid\ folder, instead of the PlayDeck Target Folder, for you to
review and correct.

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

Manalink PlayDeck Analyser Folder Structure
-------------------------------------------

The PDAnalyser is designed to contain all Imported and Created files within
its own folder structure, allowing the user to decide when and where to move
or remove files and which files to move or remove. All essential files or
folders will be created as needed. The parent folder is .\PlayDeckAnalyser\,
all other files and folders will be found inside this folder.

.\CSV\................... Contains imported\corrected Manalink.csv files
                          and products of DAT and CSV functions
                          
.\Expansions\............ Contains PlayDecks from "Export CSV as Expansions"
.\Expansions\Text\....... Contains text and csv from "Export CSV as Expansions"

.\Imports\............... Contains imported decks that were in Unix\Mac\Unicode
                          format and had to be corrected\rewritten before import
.\Imports\Invalid\....... Contains invalid decks from "Import PlayDeck" function

.\Lists\................. Contains Banned\Restricted\Substitute CardLists.
                          see "How To Use CardLists" for more info
                          
.\NetDeck\............... Contains LogFile.log files from "Import NetDeck"
.\NetDeck\dck\........... Contains PlayDeck.dck files from "Import NetDeck"
.\NetDeck\dec\........... Contains Apprentice.dec files from "Import NetDeck"
.\NetDeck\jpg\........... Contains CardImages from the "Export as Images"
                          menu option in the "Import NetDeck" DialogBox
.\NetDeck\txt\........... Contains NetDeck.txt files from "Import NetDeck"

.\PlayDeck\.............. This is the Main Target Folder for all PlayDecks
                          imported into PDAnalyser. After Rebuild, all PlayDecks
                          listed as "OK" in the LogFile will be found here
.\PlayDeck\Banned\....... PlayDecks containing cards matching a Banned List
.\PlayDeck\FixDeck\...... PlayDecks containing cards with corrected CardID#s 
.\PlayDeck\FixHeader\.... PlayDecks with corrected Header contents
.\PlayDeck\Invalid\...... PlayDecks found with non-correctable\invalid formats
.\PlayDeck\MinDeckSize\.. PlayDecks containing fewer cards than minimum limit
.\PlayDeck\NonCoded\..... PlayDecks w\cards not listed as "Coded" in the DAT\CSV
.\PlayDeck\Restricted\... PlayDecks containing cards matching a Restricted List
.\PlayDeck\Substitute\... PlayDecks containing cards matching a Substitute List
.\PlayDeck\UnMatched\.... PlayDecks containing cards not found in the CSV file

.\Search\................ PlayDecks matched during the "Search PlayDecks" dialog

.\Shandalar\............. Contains all of the CSV files exported from Shandalar


--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
I would like to thank all the folks at the CCGHQ Manalink Forum for helping
to keep this great old PC game alive and for offering their suggestions and
bug-reports that help me to try and make the Manalink PlayDeck Analyser into
a better and more useful application.

Written in Liberty Basic using these free software tools:
LB Booster  - LB4-to-BB4W translator +compiler +debugger by Richard T. Russell
LB Workshop - Complete and inclusive IDE +Gui maker for LB4 by Alyce Watson
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
| Updated by CirothUngol |
|     March 13, 2014     |
 ------------------------