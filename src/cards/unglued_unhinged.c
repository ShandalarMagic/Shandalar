#include "manalink.h"

// Unglued
static const char* target_has_reminder_text(int who_chooses, int player, int card)
{
  card_ptr_t* cp = cards_ptr[get_id(player, card)];
  return cp->rules_text && strchr(cp->rules_text, '(') ? NULL : "no reminder text";
}

int card_jack_in_the_mox(int player, int card, event_t event){
	if( event == EVENT_ACTIVATE ){
		tap_card(player, card);
		int roll = internal_rand(6);
		if( roll == 0 ){
			lose_life(player, 5);
			kill_card(player, card, KILL_SACRIFICE);
			spell_fizzled = 1;
			return 0;
		}
		else if( roll == 1 ){
			produce_mana(player, COLOR_WHITE, 1);
		}
		else if( roll == 2 ){
			produce_mana(player, COLOR_BLUE, 1);
		}
		else if( roll == 3 ){
			produce_mana(player, COLOR_BLACK, 1);
		}
		else if( roll == 4 ){
			produce_mana(player, COLOR_RED, 1);
		}
		else if( roll == 5 ){
			produce_mana(player, COLOR_GREEN, 1);
		}
		spell_fizzled = 1;
		return 0;
	}
	int result = mana_producer(player, card, event);
	return result;
}

/*
B.F.M. (Big Furry Monster) 
Creature — The-Biggest-Baddest-Nastiest-Scariest-Creature-You'll-Ever-See 99/99, BBBBBBBBBBBBBBB (15)
You must play both B.F.M. cards to put B.F.M. into play. If either B.F.M. card leaves play, sacrifice the other.
B.F.M. can be blocked only by three or more creatures.

Blacker Lotus 
Artifact, 0
{T}: Tear Blacker Lotus into pieces. Add four mana of any one color to your mana pool. 
Play this ability as a mana source. Remove the pieces from the game afterwards.

Burning Cinder Fury of Crimson Chaos Fire 
Enchantment, 3R (4)
Whenever any player taps a card, that player gives control of that card to an opponent at end of turn.
If a player does not tap any nonland cards during his or her turn, 
Burning Cinder Fury of Crimson Chaos Fire deals 3 damage to that player at end of turn.

Chicken à la King 
Creature — Chicken 2/2, 1UU (3)
Whenever a 6 is rolled on a six-sided die, put a +1/+1 counter on each Chicken in play. (You may roll dice only when a card instructs you to.)
Tap a Chicken you control: Roll a six-sided die.

Chicken Egg 
Creature — Egg 0/1, 1R (2)
During your upkeep, roll a six-sided die. On a 6, sacrifice Chicken Egg and put a Giant Chicken token into play. 
Treat this token as a 4/4 red creature that counts as a Chicken.

Clam-I-Am 
Creature — Clamfolk 2/2, 2U (3)
Whenever you roll a 3 on a six-sided die, you may reroll that die.

Clambassadors 
Creature — Clamfolk 4/4, 3U (4)
If Clambassadors damages any player, choose an artifact, creature, or land you control. That player gains control of that artifact, creature, or land.

Denied! 
Instant, U (1)
Play Denied only as any opponent casts target spell. 
Name a card, then look at all cards in that player's hand. If the named card is in the player's hand, counter target spell.

Elvish Impersonators 
Creature — Elves 100/100, 3G (4)
When you play Elvish Impersonators, roll two six-sided dice one after the other. 
Elvish Impersonators comes into play with power equal to the first die roll and toughness equal to the second.

Flock of Rabid Sheep 
Sorcery, XGG (2)
Flip X coins; an opponent calls heads or tails. For each flip you win, put a Rabid Sheep token into play. 
Treat these tokens as 2/2 green creatures that count as Sheep.

Fowl Play 
Enchantment — Aura, 2U (3)
Enchanted creature loses all abilities and is a 1/1 creature that counts as a Chicken.

Free-for-All 
Enchantment, 3U (4)
When Free-for-All comes into play, set aside all creatures in play, face down.
During each player's upkeep, that player chooses a creature card at random from those set aside in this way and puts that creature into play under his or her control.
If Free-for-All leaves play, put each creature still set aside this way into its owner's graveyard.

Free-Range Chicken 
Creature — Chicken 3/3, 3G (4)
{1}{G}: Roll two six-sided dice. If both die rolls are the same, Free-Range Chicken gets +X/+X until end of turn, 
where X is the number rolled on each die. Otherwise, if the total rolled is equal to any other total you have rolled this turn for 
Free-Range Chicken, sacrifice it. (For example, if you roll two 3s, Free-Range Chicken gets +3/+3. If you roll a total of 6 
for Free-Range Chicken later in that turn, sacrifice it.)

Gerrymandering 
Sorcery, 2G (3)
Remove all lands from play and shuffle them together. Randomly deal to each player one land card for each land he or she had before. 
Each player puts those lands into play under his or her control, untapped.

Giant Fan 
Artifact, 4 (4)
{2}, {T}: Move target counter from one card to another. 
If the second card's rules text refers to any type of counters, the moved counter becomes one of those counters. Otherwise, it becomes a +1/+1 counter.

Goblin Bookie 
Creature — Goblin 1/1, R (1)
{R}, {T}: Reflip any coin or reroll any die.

Goblin Bowling Team 
Creature — Goblins 1/1, 3R (4)
Whenever Goblin Bowling Team damages any creature or player, roll a six-sided die. Goblin Bowling Team deals to 
that creature or player additional damage equal to the die roll.

Growth Spurt 
Instant, 1G (2)
Roll a six-sided die. Target creature gets +X/+X until end of turn, where X is equal to the die roll.

Hungry Hungry Heifer 
Creature — Cow 3/3, 2G (3)
During your upkeep, remove a counter from any card you control or sacrifice Hungry Hungry Heifer.

Incoming! 
Sorcery, 4GGGG (8)
Each player searches his or her library for any number of artifacts, creatures, enchantments, and lands and puts those cards into play. 
Each player shuffles his or her library afterwards.

Infernal Spawn of Evil 
Creature — Demon Beast 7/7, 6BBB (9)
Flying, first strike
{1}{B}, Reveal Infernal Spawn of Evil from your hand, Say "It's coming": 
Infernal Spawn of Evil deals 1 damage to target opponent. Use this ability only during your upkeep and only once each upkeep.

 Jack-in-the-Mox 

Artifact, 0
{T}: Roll a six-sided die for Jack-in-the-Mox. On a 1, sacrifice Jack-in-the-Mox and lose 5 life. 
Otherwise, Jack-in-the-Mox has one of the following effects. Treat this ability as a mana source.
2 Add {W} to your mana pool.
3 Add {U} to your mana pool.
4 Add {B} to your mana pool.
5 Add {R} to your mana pool.
6 Add {G} to your mana pool.

Jalum Grifter 
Creature — Legend 3/5, 3RR (5)
{1}{R}, {T}: Put Jalum Grifter and two lands you control face down in front of target opponent after revealing each card to him or her. 
Then, rearrange the order of the three cards as often as you wish, keeping them on the table at all times. 
That opponent then chooses one of those cards. If a land is chosen, destroy target card in play. Otherwise, sacrifice Jalum Grifter.

Jumbo Imp 
Creature — Imp 0/0, 2B (3)
Flying
When you play Jumbo Imp, roll a six-sided die. Jumbo Imp comes into play with a number of +1/+1 counters on it equal to the die roll.
During your upkeep, roll a six-sided die and put on Jumbo Imp a number of +1/+1 counters equal to the die roll.
At the end of your turn, roll a six-sided die and remove from Jumbo Imp a number of +1/+1 counters equal to the die roll.

Krazy Kow 
Creature — Cow 3/3, 3R (4)
During your upkeep, roll a six-sided die. On a 1, sacrifice Krazy Kow and it deals 3 damage to each creature and player.

Mine, Mine, Mine! 
Enchantment, 4GG (6)
When Mine, Mine, Mine comes into play, each player puts his or her library into his or her hand.
Each player skips his or her discard phase and does not lose as a result of being unable to draw a card.
Each player cannot play more than one spell each turn.
If Mine, Mine, Mine leaves play, each player shuffles his or her hand and graveyard into his or her library.

Once More with Feeling 
Sorcery, WWWW (4)
Remove Once More with Feeling from the game as well as all cards in play and in all graveyards. Each player shuffles his or her hand into her or his library, then draws seven cards. Each player's life total is set to 10.
DCI ruling: This card is restricted. (You cannot play with more than one in a deck.)

Organ Harvest 
Sorcery, B (1)
You and your teammates may sacrifice any number of creatures. For each creature sacrificed in this way, add {B}{B} to your mana pool.

Paper Tiger 
Artifact Creature 4/3, 4 (4)
Rock Lobsters cannot attack or block.

Poultrygeist 
Creature — Chicken 1/1, 2B (3)
Flying
Whenever a creature is put into any graveyard from play, you may roll a six-sided die. 
On a 1, sacrifice Poultrygeist. Otherwise, put a +1/+1 counter on Poultrygeist.

Rock Lobster 
Artifact Creature 4/3, 4 (4)
Scissors Lizards cannot attack or block.

Scissors Lizard 
Artifact Creature 4/3, 4 (4)
Paper Tigers cannot attack or block.

Spark Fiend 
Creature — Beast 5/6, 4R (5)
When Spark Fiend comes into play, roll two six-sided dice. On a total of 2, 3, or 12, sacrifice Spark Fiend. On a total of 7 or 11, do not roll dice for Spark Fiend during any of your following upkeep phases. If you roll any other total, note it.
During your upkeep, roll two six-sided dice. On a total of 7, sacrifice Spark Fiend. If you roll the noted total, do not roll dice for Spark Fiend during any of your following upkeep phases. On any other roll, there is no effect.

Strategy, Schmategy 
Sorcery, 1R (2)
Roll a six-sided die for Strategy, Schmategy. On a 1, Strategy, Schmategy has no effect. Otherwise, it has one of the following effects.
2 Destroy all artifacts.
3 Destroy all lands.
4 Strategy, Schmategy deals 3 damage to each creature and player.
5 Each player discards his or her hand and draws seven cards.
6 Roll the die two more times.

Temp of the Damned 
Creature — Zombie 3/3, 2B (3)
When you play Temp of the Damned, roll a six-sided die. Temp of the Damned comes into play with a number of funk counters on it equal to the die roll.
During your upkeep, remove a funk counter from Temp of the Damned or sacrifice Temp of the Damned.

The Cheese Stands Alone 
Enchantment, 4WW (6)
If you control no cards in play other than The Cheese Stands Alone and have no cards in your hand, you win the game.

Timmy, Power Gamer 
Creature — Legend 1/1, 2GG (4)
{4}: Put a creature into play from your hand.

Urza's Science Fair Project 
Artifact Creature 4/4, 6 (6)
{2}: Roll a six-sided die for Urza's Science Fair Project.
1 It gets -2/-2 until end of turn.
2 It deals no combat damage this turn.
3 Attacking does not cause it to tap this turn.
4 It gains first strike until end of turn.
5 It gains flying until end of turn.
6 It gets +2/+2 until end of turn.
*/

//Unhinged

/*
B-I-N-G-O 
Creature — Hound 1/1, 1G (2)
Trample
Whenever a player plays a spell, put a chip counter on its converted mana cost.
B-I-N-G-O gets +9/+9 for each set of three numbers in a row with chip counters on them.

Blast from the Past 
Instant, 2R (3)
Madness {R}, cycling {1}{R}, kicker {2}{R}, flashback {3}{R}, buyback {4}{R}
Blast from the Past deals 2 damage to target creature or player.
If the kicker cost was paid, put a 1/1 red Goblin creature token into play.

Booster Tutor 
Instant, B (1)
Open a sealed Magic booster pack, reveal the cards, and put one of those cards into your hand. 
(Remove that card from your deck before beginning a new game.)

Curse of the Fire Penguin 
Enchant Creature, 4RR (6)
Curse of the Fire Penquin consumes and confuses enchanted creature.
-----
Creature Penguin
Trample
When this creature is put into a graveyard from play, return Curse of the Fire Penguin from your graveyard to play.
6/5
*/

int card_duh(int player, int card, event_t event)
{
  /* Duh |B
   * Instant
   * Destroy target creature with reminder text. (Reminder text is any italicized text in parentheses that explains rules you already know.) */

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_CREATURE);
  td.special = TARGET_SPECIAL_EXTRA_FUNCTION;
  td.extra = (int32_t)target_has_reminder_text;

  if (event == EVENT_RESOLVE_SPELL)
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  if (valid_target(&td))
		kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);

	  kill_card(player, card, KILL_DESTROY);
	}

  return generic_spell(player, card, event, GS_CAN_TARGET|GS_LITERAL_PROMPT, &td, "Select target creature with reminder text.", 1, NULL);
}

/*
Form of the Squirrel 
Enchantment, G (1)
As Form of the Squirrel comes into play, put a 1/1 green Squirrel creature token into play. You lose the game when it leaves play.
Creatures can't attack you.
You can't be the target of spells or abilities.
You can't play spells.

Goblin Secret Agent 
Creature — Goblin Rogue 2/2, 2R (3)
First strike
At the beginning of your upkeep, reveal a card from your hand at random.

Greater Morphling 
Creature — Shapeshifter 5/5, 6UU (8)
{2}: Greater Morphling gains your choice of banding, bushido 1, double strike, fear, flying, first strike, haste, landwalk of your choice, 
protection from a color of your choice, provoke, rampage 1, shadow, or trample until end of turn.
{2}: Greater Morphling becomes the colors of your choice until end of turn.
{2}: Greater Morphling's type becomes the creature type of your choice until end of turn.
{2}: Greater Morphling gets +2/-2 or -2/+2 until end of turn.
{2}: Untap Greater Morphling.

Infernal Spawn of Infernal Spawn of Evil 
Creature — Demon Child 8/8, 8BB (10)
Flying, first strike, trample
If you say "I'm coming, too" as you search your library, you may pay {1}{B} and reveal Infernal Spawn of Infernal Spawn of Evil 
from your library to have it deal 2 damage to a player of your choice. Do this no more than once each turn.

Johnny, Combo Player 
Legendary Creature — Human Gamer 1/1, 2UU (4)
{4}: Search your library for a card and put that card into your hand. Then shuffle your library.

Look at Me, I'm R&D 
Enchantment, 2W (3)
As Look at Me, I'm R&D comes into play, choose a number and a second number one higher or one lower than that number.
All instances of the first chosen number on permanents, spells, and cards in any zone are the second chosen number.

Magical Hacker 
Creature — Human Gamer 1/2, 1U (2)
{U}: Change the text of target spell or permanent by replacing all instances of + with -, and vice versa, until end of turn.

Mana Screw 
Artifact, 1 (1)
{1}: Flip a coin. If you win the flip, add {2} to your mana pool. Play this ability only any time you could play an instant.

Mise 
Instant, U (1)
Name a nonland card, then reveal the top card of your library. If that card is the named card, draw three cards.

Mox Lotus 
Artifact, 15 (15)
{T}: Add {8} to your mana pool.
{100}: Add one mana of any color to your mana pool.
You don't lose life due to mana burn.

Necro-Impotence 
Enchantment, BBB (3)
Skip your untap step.
At the beginning of your upkeep, you may pay X life. If you do, untap X permanents.
Pay ½ life: Remove the top card of your library from the game face down. Put that card into your hand at end of turn.

Old Fogey 
Summon Dinosaur 7/7, GG (2)
Phasing, cumulative upkeep {1}, echo, fading 3, bands with other Dinosaurs, protection from Homarids, snow-covered plainswalk, 
flanking, rampage 2

Rare-B-Gone 
Sorcery, 2BR (4)
Each player sacrifices all rare permanents, then reveals his or her hand and discards all rare cards.

Rocket-Powered Turbo Slug 
Creature — Slug 3/1, 3R (4)
Super haste (This may attack the turn before you play it. (You may put this card into play from your hand, tapped and attacking, 
during your declare attackers step. If you do, you lose the game at the end of your next turn unless you pay this card's mana cost 
during that turn.))

S.N.O.T. 
Creature — Ooze X^2/X^2, G (1)
As S.N.O.T. comes into play, you may stick it onto another creature named S.N.O.T. in play. 
If you do, all those creatures form a single creature.
S.N.O.T.'s power and toughness are equal to the square of the number of S.N.O.T.s stuck together. 
(One is a 1/1, two are a 4/4, three are a 9/9, and four are a 16/16.)

Six-y Beast 
Creature — Beast 0/0, 3R (4)
As Six-y Beast comes into play, you secretly put six or fewer +1/+1 counters on it, then an opponent guesses the number of counters. 
If that player guesses right, sacrifice Six-y Beast.

Togglodyte 
Artifact Creature — Golem 4/4, 3 (3)
Togglodyte comes into play turned on.
Whenever a player plays a spell, toggle Togglodyte's ON/OFF switch.
As long as Togglodyte is turned off, it can't attack or block, and all damage it would deal is prevented.

Uktabi Kong 
Creature — Ape 8/8, 5GGG (8)
Trample
When Uktabi Kong comes into play, destroy all artifacts.
Tap two untapped Apes you control: Put a 1/1 green Ape creature token into play.

Water Gun Balloon Game 
Artifact, 2 (2)
As Water Gun Balloon Game comes into play, each player puts a pop counter on a 0.
Whenever a player plays a spell, move that player's pop counter up 1.
Whenever a player's pop counter hits 5, that player puts a 5/5 pink Giant Teddy Bear creature token into play and resets all pop counters to 0.

What (Who/What/When/Where/Why) 
Instant, 2R (3)
Destroy target artifact.

When (Who/What/When/Where/Why) 
Instant, 2U (3)
Counter target creature spell.

Where (Who/What/When/Where/Why) 
Instant, 3B (4)
Destroy target land.

Who (Who/What/When/Where/Why) 
Instant, XW (1)
Target player gains X life.

Why (Who/What/When/Where/Why) 
Instant, 1G (2)
Destroy target enchantment.

“Ach! Hans, Run!” 
Enchantment, 2RRGG (6)
At the beginning of your upkeep, you may say "Ach Hans, run It's the . . ." and name a creature card. 
If you do, search your library for the named card, put it into play, then shuffle your library. 
That creature has haste. Remove it from the game at end of turn.
*/


