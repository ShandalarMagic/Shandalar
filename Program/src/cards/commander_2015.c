#include "manalink.h"

/***** Functions *****/

/***** Cards *****/

/* Bastion Protector	|2|W	0x000000
 * Creature - Human Soldier 3/3
 * Commander creatures you control get +2/+2 and have indestructible. */

/* Dawnbreak Reclaimer	|4|W|W	0x000000
 * Creature - Angel 5/5
 * Flying
 * At the beginning of your end step, choose a creature card in an opponent's graveyard, then that player chooses a creature card in your graveyard. You may return those cards to the battlefield under their owners' control. */

/* Grasp of Fate	|1|W|W	0x000000
 * Enchantment
 * When ~ enters the battlefield, for each opponent, exile up to one target nonland permanent that player controls until ~ leaves the battlefield. */

/* Herald of the Host	|3|W|W	0x000000
 * Creature - Angel 4/4
 * Flying, vigilance
 * Myriad */

/* Kalemne's Captain	|3|W|W	0x000000
 * Creature - Giant Soldier 5/5
 * Vigilance
 * |5|W|W: Monstrosity 3.
 * When ~ becomes monstrous, exile all artifacts and enchantments. */

/* Oreskos Explorer	|1|W	0x000000
 * Creature - Cat Scout 2/2
 * When ~ enters the battlefield, search your library for up to X |H1Plains cards, where X is the number of players who control more lands than you. Reveal those cards, put them into your hand, then shuffle your library. */

/* Righteous Confluence	|3|W|W	0x000000
 * Sorcery
 * Choose three. You may choose the same mode more than once.
 * * Put a 2/2 |Swhite Knight creature token with vigilance onto the battlefield.
 * * Exile target enchantment.
 * * You gain 5 life. */

/* Shielded by Faith	|1|W|W	0x000000
 * Enchantment - Aura
 * Enchant creature
 * Enchanted creature has indestructible.
 * Whenever a creature enters the battlefield, you may attach ~ to that creature. */

/* AEthersnatch	|4|U|U	0x000000
 * Instant
 * Gain control of target spell. You may choose new targets for it. */

/* Broodbirth Viper	|4|U	0x000000
 * Creature - Snake 3/3
 * Myriad
 * Whenever ~ deals combat damage to a player, you may draw a card. */

/* Gigantoplasm	|3|U	0x000000
 * Creature - Shapeshifter 0/0
 * You may have ~ enter the battlefield as a copy of any creature on the battlefield except it gains "|X: This creature has base power and toughness X/X." */

/* Illusory Ambusher	|4|U	0x000000
 * Creature - Cat Illusion 4/1
 * Flash
 * Whenever ~ is dealt damage, draw that many cards. */

/* Mirror Match	|4|U|U	0x000000
 * Instant
 * Cast ~ only during the declare blockers step.
 * For each creature attacking you or a planeswalker you control, put a token that's a copy of that creature onto the battlefield blocking that creature. Exile those tokens at end of combat. */

/* Mystic Confluence	|3|U|U	0x000000
 * Instant
 * Choose three. You may choose the same mode more than once.
 * * Counter target spell unless its controller pays |3.
 * * Return target creature to its owner's hand.
 * * Draw a card. */

/* Synthetic Destiny	|4|U|U	0x000000
 * Instant
 * Exile all creatures you control. At the beginning of the next end step, reveal cards from the top of your library until you reveal that many creature cards, put all creature cards revealed this way onto the battlefield, then shuffle the rest of the revealed cards into your library. */

/* Banshee of the Dread Choir	|3|B|B	0x000000
 * Creature - Spirit 4/4
 * Myriad
 * Whenever ~ deals combat damage to a player, that player discards a card. */

/* Corpse Augur	|3|B	0x000000
 * Creature - Zombie Wizard 4/2
 * When ~ dies, you draw X cards and you lose X life, where X is the number of creature cards in target player's graveyard. */

/* Daxos's Torment	|3|B	0x000000
 * Enchantment
 * Constellation - Whenever ~ or another enchantment enters the battlefield under your control, ~ becomes a 5/5 Demon creature with flying and haste in addition to its other types until end of turn. */

/* Deadly Tempest	|4|B|B	0x000000
 * Sorcery
 * Destroy all creatures. Each player loses life equal to the number of creatures he or she controlled that were destroyed this way. */

/* Dread Summons	|X|B|B	0x000000
 * Sorcery
 * Each player puts the top X cards of his or her library into his or her graveyard. For each creature card put into a graveyard this way, you put a 2/2 |Sblack Zombie creature token onto the battlefield tapped. */

/* Scourge of Nel Toth	|5|B|B	0x000000
 * Creature - Zombie Dragon 6/6
 * Flying
 * You may cast ~ from your graveyard by paying |B|B and sacrificing two creatures rather than paying its mana cost. */

/* Thief of Blood	|4|B|B	0x000000
 * Creature - Vampire 1/1
 * Flying
 * As ~ enters the battlefield, remove all counters from all permanents. ~ enters the battlefield with a +1/+1 counter on it for each counter removed this way. */

/* Wretched Confluence	|3|B|B	0x000000
 * Instant
 * Choose three. You may choose the same mode more than once.
 * * Target player draws a card and loses 1 life.
 * * Target creature gets -2/-2 until end of turn.
 * * Return target creature card from your graveyard to your hand. */

/* Awaken the Sky Tyrant	|3|R	0x000000
 * Enchantment
 * When a source an opponent controls deals damage to you, sacrifice ~. If you do, put a 5/5 |Sred Dragon creature token with flying onto the battlefield. */

/* Dream Pillager	|5|R|R	0x000000
 * Creature - Dragon 4/4
 * Flying
 * Whenever ~ deals combat damage to a player, exile that many cards from the top of your library. Until end of turn, you may cast nonland cards exiled this way. */

/* Fiery Confluence	|2|R|R	0x000000
 * Sorcery
 * Choose three. You may choose the same mode more than once.
 * * ~ deals 1 damage to each creature.
 * * ~ deals 2 damage to each opponent.
 * * Destroy target artifact. */

/* Magus of the Wheel	|2|R	0x000000
 * Creature - Human Wizard 3/3
 * |1|R, |T, Sacrifice ~: Each player discards his or her hand, then draws seven cards. */

/* Meteor Blast	|X|R|R|R	0x000000
 * Sorcery
 * ~ deals 4 damage to each of X target creatures and/or players. */

/* Mizzix's Mastery	|3|R	0x000000
 * Sorcery
 * Exile target card that's an instant or sorcery from your graveyard. For each card exiled this way, copy it, and you may cast the copy without paying its mana cost. Exile ~.
 * Overload |5|R|R|R */

/* Rite of the Raging Storm	|3|R|R	0x000000
 * Enchantment
 * Creatures named Lightning Rager can't attack you or planeswalkers you control.
 * At the beginning of each player's upkeep, that player puts a 5/1 |Sred Elemental creature token named Lightning Rager onto the battlefield. It has trample, haste, and "At the beginning of the end step, sacrifice this creature." */

/* Warchief Giant	|3|R|R	0x000000
 * Creature - Giant Warrior 5/3
 * Haste
 * Myriad */

/* Arachnogenesis	|2|G	0x000000
 * Instant
 * Put X 1/2 |Sgreen Spider creature tokens with reach onto the battlefield, where X is the number of creatures attacking you. Prevent all combat damage that would be dealt this turn by non-Spider creatures. */

/* Bloodspore Thrinax	|2|G|G	0x000000
 * Creature - Lizard 2/2
 * Devour 1
 * Each other creature you control enters the battlefield with an additional X +1/+1 counters on it, where X is the number of +1/+1 counters on ~. */

/* Caller of the Pack	|5|G|G	0x000000
 * Creature - Beast 8/6
 * Trample
 * Myriad */

/* Centaur Vinecrasher	|3|G	0x000000
 * Creature - Plant Centaur 1/1
 * Trample
 * ~ enters the battlefield with a number of +1/+1 counters on it equal to the number of land cards in all graveyards.
 * Whenever a land card is put into a graveyard from anywhere, you may pay |G|G. If you do, return ~ from your graveyard to your hand. */

/* Ezuri's Predation	|5|G|G|G	0x000000
 * Sorcery
 * For each creature your opponents control, put a 4/4 |Sgreen Beast creature token onto the battlefield. Each of those Beasts fights a different one of those creatures. */

/* Great Oak Guardian	|5|G	0x000000
 * Creature - Treefolk 4/5
 * Flash
 * Reach
 * When ~ enters the battlefield, creatures target player controls get +2/+2 until end of turn. Untap them. */

/* Pathbreaker Ibex	|4|G|G	0x000000
 * Creature - Goat 3/3
 * Whenever ~ attacks, creatures you control gain trample and get +X/+X until end of turn, where X is the greatest power among creatures you control. */

/* Skullwinder	|2|G	0x000000
 * Creature - Snake 1/3
 * Deathtouch
 * When ~ enters the battlefield, return target card from your graveyard to your hand, then choose an opponent. That player returns a card from his or her graveyard to his or her hand. */

/* Verdant Confluence	|4|G|G	0x000000
 * Sorcery
 * Choose three. You may choose the same mode more than once.
 * * Put two +1/+1 counters on target creature.
 * * Return target permanent card from your graveyard to your hand.
 * * Search your library for a basic land card, put it onto the battlefield tapped, then shuffle your library. */

/* Anya, Merciless Angel	|3|R|W	0x000000
 * Legendary Creature - Angel 4/4
 * Flying
 * ~ gets +3/+3 for each opponent whose life total is less than half his or her starting life total.
 * As long as an opponent's life total is less than half his or her starting life total, ~ has indestructible. */

/* Arjun, the Shifting Flame	|4|U|R	0x000000
 * Legendary Creature - Sphinx Wizard 5/5
 * Flying
 * Whenever you cast a spell, put the cards in your hand on the bottom of your library in any order, then draw that many cards. */

/* Daxos the Returned	|1|W|B	0x000000
 * Legendary Creature - Zombie Soldier 2/2
 * Whenever you cast an enchantment spell, you get an experience counter.
 * |1|W|B: Put a |Swhite and |Sblack Spirit enchantment creature token onto the battlefield. It has "This creature's power and toughness are each equal to the number of experience counters you have." */

/* Ezuri, Claw of Progress	|2|G|U	0x000000
 * Legendary Creature - Elf Warrior 3/3
 * Whenever a creature with power 2 or less enters the battlefield under your control, you get an experience counter.
 * At the beginning of combat on your turn, put X +1/+1 counters on another target creature you control, where X is the number of experience counters you have. */

/* Kalemne, Disciple of Iroas	|2|R|W	0x000000
 * Legendary Creature - Giant Soldier 3/3
 * Double strike, vigilance
 * Whenever you cast a creature spell with converted mana cost 5 or greater, you get an experience counter.
 * ~ gets +1/+1 for each experience counter you have. */

/* Karlov of the Ghost Council	|W|B	0x000000
 * Legendary Creature - Spirit Advisor 2/2
 * Whenever you gain life, put two +1/+1 counters on ~.
 * |W|B, Remove six +1/+1 counters from ~: Exile target creature. */

/* Kaseto, Orochi Archmage	|1|G|U	0x000000
 * Legendary Creature - Snake Wizard 2/2
 * |G|U: Target creature can't be blocked this turn. If that creature is a Snake, it gets +2/+2 until end of turn. */

/* Mazirek, Kraul Death Priest	|3|B|G	0x000000
 * Legendary Creature - Insect Shaman 2/2
 * Flying
 * Whenever a player sacrifices another permanent, put a +1/+1 counter on each creature you control. */

/* Meren of Clan Nel Toth	|2|B|G	0x000000
 * Legendary Creature - Human Shaman 3/4
 * Whenever another creature you control dies, you get an experience counter.
 * At the beginning of your end step, choose target creature card in your graveyard. If that card's converted mana cost is less than or equal to the number of experience counters you have, return it to the battlefield. Otherwise, put it into your hand. */

/* Mizzix of the Izmagnus	|2|U|R	0x000000
 * Legendary Creature - Goblin Wizard 2/2
 * Whenever you cast an instant or sorcery spell with converted mana cost greater than the number of experience counters you have, you get an experience counter.
 * Instant and sorcery spells you cast cost |1 less to cast for each experience counter you have. */

/* Blade of Selves	|2	0x000000
 * Artifact - Equipment
 * Equipped creature has myriad.
 * Equip |4 */

/* Sandstone Oracle	|7	0x000000
 * Artifact Creature - Sphinx 4/4
 * Flying
 * When ~ enters the battlefield, choose an opponent. If that player has more cards in hand than you, draw cards equal to the difference. */

/* Scytheclaw	|5	0x000000
 * Artifact - Equipment
 * Living weapon
 * Equipped creature gets +1/+1.
 * Whenever equipped creature deals combat damage to a player, that player loses half his or her life, rounded up.
 * Equip |3 */

/* Seal of the Guildpact	|5	0x000000
 * Artifact
 * As ~ enters the battlefield, choose two colors.
 * Each spell you cast costs |1 less to cast for each of the chosen colors it is. */

/* Thought Vessel	|2	0x000000
 * Artifact
 * You have no maximum hand size.
 * |T: Add |1 to your mana pool. */

/* Command Beacon	""	0x000000
 * Land
 * |T: Add |1 to your mana pool.
 * |T, Sacrifice ~: Put your commander into your hand from the command zone. */

// And a bunch of reprints.
