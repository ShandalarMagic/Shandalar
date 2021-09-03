package gui;

import java.util.HashMap;
import java.util.Iterator;
import java.util.Map.Entry;
import javax.swing.JOptionPane;

/**
 * A class to contain all the cards in the Shandalar game.
 * This class extends HashMap and uses a cards name as the key.
 * The name is then associated with a codePair which stores the
 * code for the Duel deck as an int and the code for the Game
 * deck as a String.
 *@see codePair
 *@author Ryan Russell
 */
public class allCards extends HashMap<String, codePair>{

    static String nl = System.getProperty("line.separator");

/**
 * When an allCards object is created we associate all
 * the values.
 */
public allCards(){
    try{
this.put("Swamp", new codePair(239,"0000"));
this.put("Island", new codePair(126,"0100"));
this.put("Forest", new codePair(91,"0200"));
this.put("Mountain", new codePair(164,"0300"));
this.put("Plains", new codePair(188,"0400"));
this.put("Badlands", new codePair(9,"0500"));
this.put("Bayou", new codePair(12,"0600"));
this.put("Plateau", new codePair(189,"0700"));
this.put("Savannah", new codePair(212,"0800"));
this.put("Scubland", new codePair(216,"0900"));
this.put("Taiga", new codePair(241,"0A00"));
this.put("Trop.Island", new codePair(252,"0B00"));
this.put("Tundra", new codePair(254,"0C00"));
this.put("Undergd.Sea", new codePair(258,"0D00"));
this.put("Vol.Island", new codePair(266,"0E00"));
this.put("Mesa Pegasus", new codePair(161,"0F00"));
this.put("BenalishHero", new codePair(13,"1000"));
this.put("Sav.Lions", new codePair(213,"1100"));
this.put("N.Paladin", new codePair(175,"1200"));
this.put("Serra Angel", new codePair(221,"1300"));
this.put("V.Bodyguard", new codePair(264,"1400"));
this.put("Samite Healr", new codePair(211,"1500"));
this.put("PearledUnicn", new codePair(180,"1600"));
this.put("WallofSwords", new codePair(273,"1700"));
this.put("WhiteKnight", new codePair(283,"1800"));
this.put("Atog", new codePair(477,"1900"));
this.put("Orcish Atty.", new codePair(177,"1A00"));
this.put("Dwarven Wpn", new codePair(493,"1B00"));
this.put("Hill Giant", new codePair(110,"1C00"));
this.put("Earth Elem.", new codePair(73,"1D00"));
this.put("DragonWhelp", new codePair(67,"1E00"));
this.put("Fire Elem.", new codePair(83,"1F00"));
this.put("GoblinBalln", new codePair(101,"2000"));
this.put("GoblinRaidrs", new codePair(163,"2100"));
this.put("Goblin King", new codePair(102,"2200"));
this.put("GraniteGarg.", new codePair(103,"2300"));
this.put("Gray Ogre", new codePair(104,"2400"));
this.put("HurloonMintr", new codePair(115,"2500"));
this.put("KeldonWarlrd", new codePair(135,"2600"));
this.put("Kird Ape", new codePair(437,"2700"));
this.put("Roc of Kher", new codePair(206,"2800"));
this.put("Rock Hydra", new codePair(207,"2900"));
this.put("Sedge Troll", new codePair(219,"2A00"));
this.put("ShivanDragon", new codePair(224,"2B00"));
this.put("Stone Giant", new codePair(235,"2C00"));
this.put("Uthden Troll", new codePair(261,"2D00"));
this.put("Wall of Fire", new codePair(270,"2E00"));
this.put("WallofStone", new codePair(272,"2F00"));
this.put("IronclawOrcs", new codePair(124,"3000"));
this.put("Rukh Egg", new codePair(452,"3100"));
this.put("Merfolk", new codePair(160,"3200"));
this.put("LordAtlantis", new codePair(150,"3300"));
this.put("AirElemental", new codePair(0,"3400"));
this.put("Sea Serpent", new codePair(218,"3500"));
this.put("Clone", new codePair(39,"3600"));
this.put("Wall of Air", new codePair(267,"3700"));
this.put("Wall of H2O", new codePair(274,"3800"));
this.put("IslandFish", new codePair(427,"3900"));
this.put("Mahamoti Djn", new codePair(154,"3A00"));
this.put("PhantasmFrce", new codePair(183,"3B00"));
this.put("PhntomMonstr", new codePair(185,"3C00"));
this.put("PirateShip", new codePair(186,"3D00"));
this.put("ProdigalSorc", new codePair(193,"3E00"));
this.put("WaterElement", new codePair(279,"3F00"));
this.put("ZephyrFalcon", new codePair(859,"4000"));
this.put("GiantTortse", new codePair(422,"4100"));
this.put("GuardBeast", new codePair(423,"4200"));
this.put("V.Doppelgngr", new codePair(263,"4300"));
this.put("ShanodnDryad", new codePair(222,"4400"));
this.put("Wall of Wood", new codePair(275,"4500"));
this.put("Grizzly Bear", new codePair(106,"4600"));
this.put("Fungusaur", new codePair(94,"4700"));
this.put("War Mammoth", new codePair(277,"4800"));
this.put("Giant Spider", new codePair(98,"4900"));
this.put("Craw Wurm", new codePair(49,"4A00"));
this.put("ElvishArchrs", new codePair(76,"4B00"));
this.put("ForceONature", new codePair(89,"4C00"));
this.put("IronrootTflk", new codePair(125,"4D00"));
this.put("LlanowarElvs", new codePair(149,"4E00"));
this.put("ScrybSprites", new codePair(217,"4F00"));
this.put("TimberWolves", new codePair(247,"5000"));
this.put("Wall/Bramble", new codePair(269,"5100"));
this.put("Enchantress", new codePair(262,"5200"));
this.put("DurkwdBoars", new codePair(618,"5300"));
this.put("Drudge Skel.", new codePair(70,"5400"));
this.put("ScatheZombie", new codePair(214,"5500"));
this.put("ZombieMaster", new codePair(291,"5600"));
this.put("Erg Raiders", new codePair(415,"5700"));
this.put("Carrion Ants", new codePair(589,"5800"));
this.put("Bog Wraith", new codePair(24,"5900"));
this.put("Frozen Shade", new codePair(93,"5A00"));
this.put("Nightmare", new codePair(174,"5B00"));
this.put("RoyalAssasin", new codePair(209,"5C00"));
this.put("SorceressQn", new codePair(460,"5D00"));
this.put("WallofBone", new codePair(268,"5E00"));
this.put("WillOtheWisp", new codePair(286,"5F00"));
this.put("HypnoticSpec", new codePair(117,"6000"));
this.put("Vampire Bats", new codePair(835,"6100"));
this.put("Bad Moon", new codePair(8,"6200"));
this.put("Cursed Land", new codePair(53,"6300"));
this.put("EvilPresence", new codePair(77,"6400"));
this.put("UnholyStrnth", new codePair(259,"6500"));
this.put("Weakness", new codePair(280,"6600"));
this.put("WarpArtifact", new codePair(278,"6700"));
this.put("Unstable Mn.", new codePair(462,"6800"));
this.put("Phantasmal", new codePair(184,"6900"));
this.put("CopyArtifact", new codePair(47,"6A00"));
this.put("Flight", new codePair(87,"6B00"));
this.put("PsychicVenom", new codePair(195,"6C00"));
this.put("CreatureBond", new codePair(50,"6D00"));
this.put("Animate Art.", new codePair(2,"6E00"));
this.put("Feedback", new codePair(82,"6F00"));
this.put("Lifetap", new codePair(144,"7000"));
this.put("Wild Growth", new codePair(285,"7100"));
this.put("Regeneration", new codePair(201,"7200"));
this.put("Web", new codePair(281,"7300"));
this.put("Wanderlust", new codePair(276,"7400"));
this.put("InstillEnrgy", new codePair(121,"7500"));
this.put("Lifeforce", new codePair(142,"7600"));
this.put("AspectofWolf", new codePair(7,"7700"));
this.put("LivingArtfct", new codePair(146,"7800"));
this.put("Earthbind", new codePair(74,"7900"));
this.put("Burrowing", new codePair(26,"7A00"));
this.put("Firebreath", new codePair(85,"7B00"));
this.put("Manabarbs", new codePair(158,"7C00"));
this.put("Oriflamme", new codePair(178,"7D00"));
this.put("ManaFlare", new codePair(155,"7E00"));
this.put("Holy Armor", new codePair(111,"7F00"));
this.put("Castle", new codePair(28,"8000"));
this.put("HolyStrength", new codePair(112,"8100"));
this.put("Black Ward", new codePair(19,"8200"));
this.put("Green Ward", new codePair(105,"8300"));
this.put("Blue Ward", new codePair(23,"8400"));
this.put("Red Ward", new codePair(200,"8500"));
this.put("White Ward", new codePair(284,"8600"));
this.put("Art Ward", new codePair(473,"8700"));
this.put("Karma", new codePair(134,"8800"));
this.put("Farmstead", new codePair(79,"8900"));
this.put("COP White", new codePair(37,"8A00"));
this.put("COP Black", new codePair(33,"8B00"));
this.put("COP Blue", new codePair(34,"8C00"));
this.put("COP Red", new codePair(36,"8D00"));
this.put("COP Green", new codePair(35,"8E00"));
this.put("Crusade", new codePair(51,"8F00"));
this.put("BlazeOfGlory", new codePair(20,"9000"));
this.put("Blessing", new codePair(21,"9100"));
this.put("Conversion", new codePair(45,"9200"));
this.put("SpiritLink", new codePair(794,"9300"));
this.put("DivineTrans", new codePair(616,"9400"));
this.put("Wrath of God", new codePair(290,"9500"));
this.put("Armageddon", new codePair(6,"9600"));
this.put("Resurrection", new codePair(203,"9700"));
this.put("Raise Dead", new codePair(198,"9800"));
this.put("Drain Life", new codePair(68,"9900"));
this.put("Reconstruct", new codePair(521,"9A00"));
this.put("Braingeyser", new codePair(25,"9B00"));
this.put("Disintegrate", new codePair(65,"9C00"));
this.put("Stone Rain", new codePair(236,"9D00"));
this.put("EarthQuake", new codePair(75,"9E00"));
this.put("Fireball", new codePair(84,"9F00"));
this.put("Flashfires", new codePair(86,"A000"));
this.put("Shatterstorm", new codePair(526,"A100"));
this.put("DesertTwistr", new codePair(409,"A200"));
this.put("Hurricane", new codePair(116,"A300"));
this.put("Tranquility", new codePair(251,"A400"));
this.put("StreamofLife", new codePair(237,"A500"));
this.put("Ice Storm", new codePair(118,"A600"));
this.put("Regrowth", new codePair(202,"A700"));
this.put("Tsunami", new codePair(253,"A800"));
this.put("Basalt Mono", new codePair(11,"A900"));
this.put("Conservator", new codePair(42,"AA00"));
this.put("GntletOMight", new codePair(96,"AB00"));
this.put("Iron Star", new codePair(123,"AC00"));
this.put("Ivory Cup", new codePair(128,"AD00"));
this.put("Crystal Rod", new codePair(52,"AE00"));
this.put("AnkhofMishra", new codePair(5,"AF00"));
this.put("ArmageddnClk", new codePair(471,"B000"));
this.put("Dingus Egg", new codePair(63,"B100"));
this.put("Ebony Horse", new codePair(412,"B200"));
this.put("JadeMonolith", new codePair(129,"B300"));
this.put("JandorsSddle", new codePair(430,"B400"));
this.put("JayemdaeTome", new codePair(131,"B500"));
this.put("Mana Vault", new codePair(157,"B600"));
this.put("Meekstone", new codePair(159,"B700"));
this.put("Onulet", new codePair(512,"B800"));
this.put("RocketLaunch", new codePair(523,"B900"));
this.put("RodofRuin", new codePair(208,"BA00"));
this.put("Sol Ring", new codePair(230,"BB00"));
this.put("Soul Net", new codePair(231,"BC00"));
this.put("ThroneofBone", new codePair(246,"BD00"));
this.put("WoodenSphere", new codePair(288,"BE00"));
this.put("Winter Orb", new codePair(287,"BF00"));
this.put("FlyingCarpet", new codePair(419,"C000"));
this.put("HelmofChatz", new codePair(109,"C100"));
this.put("HowlingMine", new codePair(114,"C200"));
this.put("Mightstone", new codePair(506,"C300"));
this.put("SunglassUrza", new codePair(238,"C400"));
this.put("Brass Man", new codePair(930,"C500"));
this.put("D.Scimitar", new codePair(931,"C600"));
this.put("DragonEngine", new codePair(492,"C700"));
this.put("ClockworkBst", new codePair(38,"C800"));
this.put("Living Wall", new codePair(148,"C900"));
this.put("ObsianusGolm", new codePair(176,"CA00"));
this.put("Ornithopter", new codePair(514,"CB00"));
this.put("Disenchant", new codePair(64,"CC00"));
this.put("Guard.Angel", new codePair(107,"CD00"));
this.put("Death Ward", new codePair(57,"CE00"));
this.put("Righteousnss", new codePair(205,"CF00"));
this.put("SwordToPlow", new codePair(240,"D000"));
this.put("HealingSalve", new codePair(108,"D100"));
this.put("AlabasterPot", new codePair(560,"D200"));
this.put("PsionicBlast", new codePair(194,"D300"));
this.put("Hurkyl's Rcl", new codePair(502,"D400"));
this.put("Jump", new codePair(133,"D500"));
this.put("Unsummon", new codePair(260,"D600"));
this.put("AncestralRcl", new codePair(1,"D700"));
this.put("Mana Short", new codePair(156,"D800"));
this.put("Howl/Beyond", new codePair(113,"D900"));
this.put("Terror", new codePair(242,"DA00"));
this.put("Shatter", new codePair(223,"DB00"));
this.put("Tunnel", new codePair(255,"DC00"));
this.put("Lightning", new codePair(145,"DD00"));
this.put("Inferno", new codePair(337,"DE00"));
this.put("Berserk", new codePair(14,"DF00"));
this.put("Giant Growth", new codePair(97,"E000"));
this.put("Simulacrum", new codePair(225,"E100"));
this.put("Purelace", new codePair(196,"E200"));
this.put("RagingRiver", new codePair(197,"E300"));
this.put("BlueBlast", new codePair(22,"E400"));
this.put("Counterspell", new codePair(48,"E500"));
this.put("Spell Blast", new codePair(232,"E600"));
this.put("Deathlace", new codePair(59,"E700"));
this.put("Sacrifice", new codePair(210,"E800"));
this.put("Dark Ritual", new codePair(55,"E900"));
this.put("Chaoslace", new codePair(32,"EA00"));
this.put("RedBlast", new codePair(199,"EB00"));
this.put("Lifelace", new codePair(143,"EC00"));
this.put("Thoughtlace", new codePair(245,"ED00"));
this.put("Mox Emerald", new codePair(165,"EE00"));
this.put("Mox Jet", new codePair(166,"EF00"));
this.put("Mox Pearl", new codePair(167,"F000"));
this.put("Mox Ruby", new codePair(168,"F100"));
this.put("Mox Sapphire", new codePair(169,"F200"));
this.put("Black Lotus", new codePair(17,"F300"));
this.put("AladdinsRing", new codePair(396,"F400"));
this.put("Celest.Prism", new codePair(29,"F500"));
this.put("CopperTablet", new codePair(46,"F600"));
this.put("DisruptScept", new codePair(66,"F700"));
this.put("IcyManipultr", new codePair(119,"F800"));
this.put("Consecrate", new codePair(41,"F900"));
this.put("Lance", new codePair(138,"FA00"));
this.put("ReverseDamge", new codePair(204,"FB00"));
this.put("JandorsRing", new codePair(429,"FC00"));
this.put("Jeweled Bird", new codePair(431,"FD00"));
this.put("Jade Statue", new codePair(130,"FE00"));
this.put("RingOfMaruf", new codePair(451,"FF00"));
this.put("ErhnamDjn", new codePair(416,"0001"));
this.put("CityofBrass", new codePair(403,"0101"));
this.put("Desert", new codePair(407,"0201"));
this.put("ArmyofAllah", new codePair(397,"0301"));
this.put("Jihad", new codePair(432,"0401"));
this.put("KingSuleiman", new codePair(436,"0501"));
this.put("Moorish Cav.", new codePair(443,"0601"));
this.put("Piety", new codePair(448,"0701"));
this.put("Pyramids", new codePair(449,"0801"));
this.put("Blacksmith", new codePair(450,"0901"));
this.put("WarElephant", new codePair(463,"0A01"));
this.put("Dandan", new codePair(406,"0B01"));
this.put("FishliverOil", new codePair(418,"0C01"));
this.put("FlyingMen", new codePair(420,"0D01"));
this.put("MerchantShip", new codePair(440,"0E01"));
this.put("SerendibDjn.", new codePair(455,"0F01"));
this.put("SerendibEfr.", new codePair(456,"1001"));
this.put("El-Hajjaj", new codePair(413,"1101"));
this.put("HasranOgress", new codePair(424,"1201"));
this.put("Junun Efreet", new codePair(433,"1301"));
this.put("Juzam Djinn", new codePair(434,"1401"));
this.put("Khabal Ghoul", new codePair(435,"1501"));
this.put("Stone Devils", new codePair(461,"1601"));
this.put("Ali Baba", new codePair(394,"1701"));
this.put("AliFromCairo", new codePair(395,"1801"));
this.put("Bird Maiden", new codePair(399,"1901"));
this.put("Mijae Djinn", new codePair(442,"1A01"));
this.put("Ghazban Ogre", new codePair(421,"1B01"));
this.put("Sandstorm", new codePair(454,"1C01"));
this.put("SandalsAbdal", new codePair(453,"1D01"));
this.put("Plague Rats", new codePair(187,"1E01"));
this.put("CuombajWitch", new codePair(404,"1F01"));
this.put("BlackKnight", new codePair(16,"2001"));
this.put("Camel", new codePair(401,"2101"));
this.put("Juggernaut", new codePair(132,"2201"));
this.put("Naf's Asp", new codePair(444,"2301"));
this.put("Wyluli Wolf", new codePair(464,"2401"));
this.put("Hurr Jackal", new codePair(425,"2501"));
this.put("Invisibility", new codePair(122,"2601"));
this.put("Fear", new codePair(81,"2701"));
this.put("DwarvenDTeam", new codePair(71,"2801"));
this.put("DwrvnWarrior", new codePair(72,"2901"));
this.put("BirdOParadse", new codePair(15,"2A01"));
this.put("GiantBadger", new codePair(898,"2B01"));
this.put("Nettling Imp", new codePair(172,"2C01"));
this.put("SengirVampre", new codePair(220,"2D01"));
this.put("LordofthePit", new codePair(151,"2E01"));
this.put("NetherShadow", new codePair(171,"2F01"));
this.put("NevinyrrlDsk", new codePair(173,"3001"));
this.put("Paralyze", new codePair(179,"3101"));
this.put("AnimateDead", new codePair(3,"3201"));
this.put("DemonicTutor", new codePair(62,"3301"));
this.put("MindTwist", new codePair(162,"3401"));
this.put("DiamondVally", new codePair(410,"3501"));
this.put("IsleOfWakWak", new codePair(428,"3601"));
this.put("Pestilence", new codePair(182,"3701"));
this.put("The Hive", new codePair(243,"3801"));
this.put("Forcefield", new codePair(90,"3901"));
this.put("Power Leak", new codePair(190,"3A01"));
this.put("DrainPower", new codePair(69,"3B01"));
this.put("Twiddle", new codePair(256,"3C01"));
this.put("2HeadedGiant", new codePair(257,"3D01"));
this.put("Stasis", new codePair(233,"3E01"));
this.put("Scav.Ghoul", new codePair(215,"3F01"));
this.put("Sinkhole", new codePair(226,"4001"));
this.put("MagneticMtn", new codePair(439,"4101"));
this.put("Power Surge", new codePair(192,"4201"));
this.put("Smoke", new codePair(229,"4301"));
this.put("WheelFortune", new codePair(282,"4401"));
this.put("Channel", new codePair(30,"4501"));
this.put("Fastbond", new codePair(80,"4601"));
this.put("Ley Druid", new codePair(139,"4701"));
this.put("Lure", new codePair(152,"4801"));
this.put("ThicketBslsk", new codePair(244,"4901"));
this.put("Cockatrice", new codePair(40,"4A01"));
this.put("WallofIce", new codePair(271,"4B01"));
this.put("Magical Hack", new codePair(153,"4C01"));
this.put("SleightOMind", new codePair(228,"4D01"));
this.put("Black Vise", new codePair(18,"4E01"));
this.put("Ivory Tower", new codePair(503,"4F01"));
this.put("The Rack", new codePair(535,"5001"));
this.put("ContractBelo", new codePair(43,"5101"));
this.put("TimeTwister", new codePair(250,"5201"));
this.put("AladdinsLamp", new codePair(393,"5301"));
this.put("Millstone", new codePair(507,"5401"));
this.put("BazaarOBaghd", new codePair(398,"5501"));
this.put("LibraryOAlex", new codePair(438,"5601"));
this.put("Sindbad", new codePair(458,"5701"));
this.put("SingingTree", new codePair(459,"5801"));
this.put("GoblnArtisns", new codePair(498,"5901"));
this.put("MishraFactry", new codePair(508,"5A01"));
this.put("MishraWrkshp", new codePair(510,"5B01"));
this.put("Strip Mine", new codePair(528,"5C01"));
this.put("Urza's Mine", new codePair(541,"5D01"));
this.put("Urza'sPlant", new codePair(543,"5E01"));
this.put("Urza'sTower", new codePair(544,"5F01"));
this.put("ArgivianArch", new codePair(467,"6001"));
this.put("ArgivianSmth", new codePair(468,"6101"));
this.put("COPArtifacts", new codePair(481,"6201"));
this.put("DampingField", new codePair(489,"6301"));
this.put("MartyrOKorls", new codePair(505,"6401"));
this.put("RvrsPolarity", new codePair(522,"6501"));
this.put("DrafnaRstore", new codePair(491,"6601"));
this.put("EnergyFlux", new codePair(494,"6701"));
this.put("SageOfLatNam", new codePair(524,"6801"));
this.put("TransmuteArt", new codePair(537,"6901"));
this.put("Afct.Possess", new codePair(516,"6A01"));
this.put("GatePhyrexia", new codePair(497,"6B01"));
this.put("HauntingWind", new codePair(501,"6C01"));
this.put("PhyrxGremlin", new codePair(515,"6D01"));
this.put("PriestYwgmth", new codePair(549,"6E01"));
this.put("XenicPolterg", new codePair(547,"6F01"));
this.put("YwgmthDemon", new codePair(548,"7001"));
this.put("ArtifactBlst", new codePair(472,"7101"));
this.put("Detonate", new codePair(490,"7201"));
this.put("OrcMechanic", new codePair(513,"7301"));
this.put("ArgothnPixie", new codePair(469,"7401"));
this.put("ArgothnTrflk", new codePair(470,"7501"));
this.put("CitanulDruid", new codePair(482,"7601"));
this.put("Crumble", new codePair(487,"7701"));
this.put("GaeasAvenger", new codePair(496,"7801"));
this.put("PowerLeech", new codePair(518,"7901"));
this.put("AmuletOKroog", new codePair(466,"7A01"));
this.put("AshnodsAltar", new codePair(474,"7B01"));
this.put("AshnodsBtlgr", new codePair(475,"7C01"));
this.put("AshnodsTrans", new codePair(476,"7D01"));
this.put("BatteringRam", new codePair(478,"7E01"));
this.put("CandlOTawnos", new codePair(480,"7F01"));
this.put("ClayStatue", new codePair(483,"8001"));
this.put("ClockworkAvn", new codePair(484,"8101"));
this.put("ColosusOSard", new codePair(485,"8201"));
this.put("Coral Helm", new codePair(486,"8301"));
this.put("Cursed Rack", new codePair(488,"8401"));
this.put("Feldons Cane", new codePair(495,"8501"));
this.put("GrapeshotCpt", new codePair(500,"8601"));
this.put("Jalum Tome", new codePair(504,"8701"));
this.put("MishraWarMch", new codePair(509,"8801"));
this.put("ObeliskOUndo", new codePair(511,"8901"));
this.put("Primal Clay", new codePair(519,"8A01"));
this.put("Rakalite", new codePair(520,"8B01"));
this.put("StaffOfZegon", new codePair(527,"8C01"));
this.put("Su-Chi", new codePair(529,"8D01"));
this.put("TabltOEpityr", new codePair(530,"8E01"));
this.put("TawnosCoffin", new codePair(531,"8F01"));
this.put("TawnosWeapon", new codePair(533,"9001"));
this.put("Triskelion", new codePair(538,"9101"));
this.put("UrzasAvenger", new codePair(539,"9201"));
this.put("UrzasChalice", new codePair(540,"9301"));
this.put("UrzasMiter", new codePair(542,"9401"));
this.put("WallofSpears", new codePair(545,"9501"));
this.put("Weakstone", new codePair(546,"9601"));
this.put("YotnSoldier", new codePair(550,"9701"));
this.put("Time Walk", new codePair(249,"9801"));
this.put("Time Vault", new codePair(248,"9901"));
this.put("Balance", new codePair(10,"9A01"));
this.put("CyclopeanTmb", new codePair(54,"9B01"));
this.put("EyeForAnEye", new codePair(417,"9C01"));
this.put("IslandSanct.", new codePair(127,"9D01"));
this.put("PersonalInc.", new codePair(181,"9E01"));
this.put("ControlMagic", new codePair(44,"9F01"));
this.put("StealArtifct", new codePair(234,"A001"));
this.put("Siren's Call", new codePair(227,"A101"));
this.put("VolcanicErpt", new codePair(265,"A201"));
this.put("Darkpact", new codePair(56,"A301"));
this.put("DemonicHorde", new codePair(61,"A401"));
this.put("DemonicAttny", new codePair(60,"A501"));
this.put("Fork", new codePair(92,"A601"));
this.put("GaeasLiege", new codePair(95,"A701"));
this.put("Kudzu", new codePair(137,"A801"));
this.put("LivingLands", new codePair(147,"A901"));
this.put("Kormus Bell", new codePair(136,"AA01"));
this.put("NaturalSelec", new codePair(170,"AB01"));
this.put("BottleOSulmn", new codePair(400,"AC01"));
this.put("GlassesOUrza", new codePair(99,"AD01"));
this.put("LibraryOLeng", new codePair(140,"AE01"));
this.put("Lich", new codePair(141,"AF01"));
this.put("ElephGrvyard", new codePair(414,"B001"));
this.put("OldManoftheC", new codePair(446,"B101"));
this.put("Oubliette", new codePair(447,"B201"));
this.put("DesertNomads", new codePair(408,"B301"));
this.put("YdwenEfreet", new codePair(465,"B401"));
this.put("Cyclone", new codePair(405,"B501"));
this.put("DropofHoney", new codePair(411,"B601"));
this.put("IfhBiffEfrt", new codePair(426,"B701"));
this.put("Abu Jafar", new codePair(391,"B801"));
this.put("Aladdin", new codePair(392,"B901"));
this.put("AnimateWall", new codePair(4,"BA01"));
this.put("CallfmGrave", new codePair(860,"BB01"));
this.put("PrismaticDrg", new codePair(861,"BC01"));
this.put("RainbowKnght", new codePair(862,"BD01"));
this.put("PandorasBox", new codePair(863,"BE01"));
this.put("Whimsy", new codePair(864,"BF01"));
this.put("FaerieDragon", new codePair(865,"C001"));
this.put("GoblinPolka", new codePair(866,"C101"));
this.put("PowerStruggl", new codePair(867,"C201"));
this.put("Aswan Jaguar", new codePair(868,"C301"));
this.put("OrcCatapult", new codePair(869,"C401"));
this.put("Gem Bazaar", new codePair(870,"C501"));
this.put("NecropofAzar", new codePair(871,"C601"));
this.put("Arena", new codePair(899,"C701"));
this.put("Mana Crypt", new codePair(894,"C801"));
this.put("NalathniDrgn", new codePair(895,"C901"));
this.put("SewerOEstark", new codePair(896,"CA01"));
this.put("WindseekCntr", new codePair(897,"CB01"));
this.put("Bog Imp", new codePair(336,"CC01"));
this.put("Carniv.Plant", new codePair(305,"CD01"));
this.put("DiabolicMach", new codePair(376,"CE01"));
this.put("Ghost Ship", new codePair(326,"CF01"));
this.put("GiantStrngth", new codePair(649,"D001"));
this.put("Immolation", new codePair(679,"D101"));
this.put("Killer Bees", new codePair(698,"D201"));
this.put("Land Leeches", new codePair(339,"D301"));
this.put("Lost Soul", new codePair(719,"D401"));
this.put("Pikeman", new codePair(942,"D501"));
this.put("Seeker", new codePair(781,"D601"));
this.put("SegovianLev", new codePair(782,"D701"));
this.put("SistersFlame", new codePair(360,"D801"));
this.put("TundraWolves", new codePair(826,"D901"));
this.put("UncleIstvan", new codePair(382,"DA01"));
this.put("SunkenCity", new codePair(364,"DB01"));
this.put("BlueBattery", new codePair(584,"DC01"));
this.put("WhiteBattery", new codePair(852,"DD01"));
this.put("BlackBattery", new codePair(580,"DE01"));
this.put("RedBattery", new codePair(765,"DF01"));
this.put("GreenBattery", new codePair(662,"E001"));
this.put("ApprenticeWz", new codePair(300,"E101"));
this.put("Abomination", new codePair(551,"E201"));
this.put("Venom", new codePair(383,"E301"));
this.put("CyclopMummy", new codePair(606,"E401"));
this.put("CosmicHorror", new codePair(600,"E501"));
this.put("AngryMob", new codePair(293,"E601"));
this.put("Blood Lust", new codePair(583,"E701"));
this.put("FortArea", new codePair(642,"E801"));
this.put("AmrouKithkin", new codePair(563,"E901"));
this.put("ElvenRiders", new codePair(622,"EA01"));
this.put("Visions", new codePair(837,"EB01"));
this.put("BallLightng", new codePair(295,"EC01"));
this.put("PsionicEnt.", new codePair(749,"ED01"));
this.put("Morale", new codePair(347,"EE01"));
this.put("CavePeople", new codePair(306,"EF01"));
this.put("Blight", new codePair(582,"F001"));
this.put("TheBrute", new codePair(813,"F101"));
this.put("EternalWar", new codePair(628,"F201"));
this.put("PradeshGyp", new codePair(745,"F301"));
this.put("Ashes2Ashes", new codePair(294,"F401"));
this.put("Fissure", new codePair(321,"F501"));
this.put("WindsOChange", new codePair(854,"F601"));
this.put("WordOBinding", new codePair(388,"F701"));
this.put("FellwarStone", new codePair(318,"F801"));
this.put("ManaClash", new codePair(343,"F901"));
this.put("MarshViper", new codePair(939,"FA01"));
this.put("MindBomb", new codePair(346,"FB01"));
this.put("TimeElementl", new codePair(817,"FC01"));
this.put("UntamedWilds", new codePair(831,"FD01"));
this.put("Backfire", new codePair(575,"FE01"));
this.put("ElderLandWrm", new codePair(620,"FF01"));
this.put("Erosion", new codePair(314,"0002"));
this.put("Flood", new codePair(935,"0102"));
this.put("Gaseous Form", new codePair(645,"0202"));
this.put("GoblnRockSld", new codePair(937,"0302"));
this.put("Greed", new codePair(661,"0402"));
this.put("Kismet", new codePair(699,"0502"));
this.put("Leviathan", new codePair(340,"0602"));
this.put("Murk Dweller", new codePair(371,"0702"));
this.put("Osai Vulture", new codePair(736,"0802"));
this.put("Pyrotechnics", new codePair(752,"0902"));
this.put("Relic Bind", new codePair(768,"0A02"));
this.put("WhirlingDerv", new codePair(851,"0B02"));
this.put("Winter Blast", new codePair(855,"0C02"));
this.put("BrotherOFire", new codePair(304,"0D02"));
this.put("Energy Tap", new codePair(626,"0E02"));
this.put("Marsh Gas", new codePair(365,"0F02"));
this.put("RadjanSpirit", new codePair(756,"1002"));
this.put("SpiritShckle", new codePair(795,"1102"));
this.put("Pit Scorpion", new codePair(742,"1202"));
this.put("Brainwash", new codePair(303,"1302"));
this.put("CrimsonMant", new codePair(604,"1402"));
this.put("SylvnLibrary", new codePair(803,"1502"));
this.put("Land Tax", new codePair(710,"1602"));
this.put("Rag Man", new codePair(943,"1702"));
this.put("Rebirth", new codePair(763,"1802"));
this.put("TempestEfrt", new codePair(810,"1902"));
this.put("Shapeshifter", new codePair(525,"1A02"));
this.put("Fog", new codePair(88,"1B02"));
this.put("Deathgrip", new codePair(58,"1C02"));
this.put("Tawnos'sWand", new codePair(532,"1D02"));
this.put("Wall of Dust", new codePair(841,"1E02"));
this.put("BronzeTablet", new codePair(479,"1F02"));
this.put("Tetravus", new codePair(534,"2002"));
this.put("TitaniasSong", new codePair(536,"2102"));
this.put("Power Sink", new codePair(191,"2202"));
this.put("Gloom", new codePair(100,"2302"));
this.put("Oasis", new codePair(445,"2402"));
this.put("CrimsonKobolds", new codePair(603,"2502"));
this.put("CrookKobolds", new codePair(605,"2602"));
this.put("KoboldsOKher", new codePair(704,"2702"));
this.put("Mountain Yeti", new codePair(730,"2802"));
this.put("Raging Bull", new codePair(757,"2902"));
this.put("Wall Of Earth", new codePair(842,"2A02"));
this.put("Wall Of Heat", new codePair(843,"2B02"));
this.put("Goblin Hero", new codePair(330,"2C02"));
this.put("Azure Drake", new codePair(573,"2D02"));
this.put("Devouring Deep", new codePair(612,"2E02"));
this.put("Barbary Apes", new codePair(576,"2F02"));
this.put("Cat Warriors", new codePair(590,"3002"));
this.put("Hornet Cobra", new codePair(674,"3102"));
this.put("Moss Monster", new codePair(728,"3202"));
this.put("HeadlessHMan", new codePair(667,"3302"));
this.put("KeepersOFaith", new codePair(696,"3402"));
this.put("RighteousAvers", new codePair(774,"3502"));
this.put("Thunder Spirit", new codePair(816,"3602"));
this.put("Wall Of Light", new codePair(844,"3702"));
this.put("KnightsOThorn", new codePair(338,"3802"));
this.put("Squire", new codePair(948,"3902"));
this.put("Acid Rain", new codePair(552,"3A02"));
this.put("Darkness", new codePair(609,"3B02"));
this.put("Holy Day", new codePair(672,"3C02"));
this.put("InfernalMedusa", new codePair(683,"3D02"));
this.put("Lifeblood", new codePair(715,"3E02"));
this.put("Walking Dead", new codePair(839,"3F02"));
this.put("Boomerang", new codePair(585,"4002"));
this.put("Cleanse", new codePair(596,"4102"));
this.put("DAvenantArcher", new codePair(607,"4202"));
this.put("DivineOffering", new codePair(615,"4302"));
this.put("Exorcist", new codePair(316,"4402"));
this.put("Fallen Angel", new codePair(631,"4502"));
this.put("FountainOYouth", new codePair(324,"4602"));
this.put("GhostsODamned", new codePair(647,"4702"));
this.put("GoblinDigTeam", new codePair(936,"4802"));
this.put("Great Defender", new codePair(658,"4902"));
this.put("Hell Swarm", new codePair(669,"4A02"));
this.put("Inquisition", new codePair(374,"4B02"));
this.put("Jovial Evil", new codePair(692,"4C02"));
this.put("MerfolkAssassn", new codePair(940,"4D02"));
this.put("PeopleOWoods", new codePair(323,"4E02"));
this.put("Relic Barrier", new codePair(767,"4F02"));
this.put("Remove Soul", new codePair(770,"5002"));
this.put("Riptide", new codePair(355,"5102"));
this.put("Scavenger Folk", new codePair(947,"5202"));
this.put("Shield Wall", new codePair(786,"5302"));
this.put("Spinal Villain", new codePair(793,"5402"));
this.put("Storm Seeker", new codePair(798,"5502"));
this.put("The Drowned", new codePair(370,"5602"));
this.put("Typhoon", new codePair(827,"5702"));
this.put("WallOOppositn", new codePair(845,"5802"));
this.put("Water Wurm", new codePair(951,"5902"));
this.put("Alchors Tomb", new codePair(561,"5A02"));
this.put("Angelic Voices", new codePair(564,"5B02"));
this.put("Banshee", new codePair(296,"5C02"));
this.put("BeastOBogardan", new codePair(579,"5D02"));
this.put("Bog Rats", new codePair(350,"5E02"));
this.put("Bone Flute", new codePair(302,"5F02"));
this.put("ElvesODeepShad", new codePair(313,"6002"));
this.put("Emerald Dfly", new codePair(623,"6102"));
this.put("Eternal Flame", new codePair(315,"6202"));
this.put("Fire Drake", new codePair(320,"6302"));
this.put("Giant Turtle", new codePair(650,"6402"));
this.put("GrterRealmOPrs", new codePair(660,"6502"));
this.put("Hidden Path", new codePair(335,"6602"));
this.put("Holy Light", new codePair(375,"6702"));
this.put("IvoryGuardians", new codePair(686,"6802"));
this.put("KoboldDSerge", new codePair(701,"6902"));
this.put("KoboldOverlord", new codePair(702,"6A02"));
this.put("KoboldTmaster", new codePair(703,"6B02"));
this.put("Life Chisel", new codePair(713,"6C02"));
this.put("Miracle Worker", new codePair(941,"6D02"));
this.put("Mold Demon", new codePair(727,"6E02"));
this.put("Pixie Queen", new codePair(743,"6F02"));
this.put("Reset", new codePair(771,"7002"));
this.put("Savaen Elves", new codePair(944,"7102"));
this.put("SerpentGener", new codePair(784,"7202"));
this.put("SpiritualSanct", new codePair(796,"7302"));
this.put("Syphon Soul", new codePair(805,"7402"));
this.put("WallOTombstone", new codePair(848,"7502"));
this.put("War Barge", new codePair(385,"7602"));
this.put("Witch Hunter", new codePair(387,"7702"));
this.put("AkronLegnnaire", new codePair(558,"7802"));
this.put("Amnesia", new codePair(292,"7902"));
this.put("Barl's Cage", new codePair(297,"7A02"));
this.put("Blood Moon", new codePair(299,"7B02"));
this.put("Book Of Rass", new codePair(353,"7C02"));
this.put("Coal Golem", new codePair(308,"7D02"));
this.put("Elder Spawn", new codePair(621,"7E02"));
this.put("Fire Sprites", new codePair(635,"7F02"));
this.put("Force Spike", new codePair(640,"8002"));
this.put("GoblinsOFlarg", new codePair(332,"8102"));
this.put("HyperionBsmith", new codePair(677,"8202"));
this.put("Martyr's Cry", new codePair(345,"8302"));
this.put("Moat", new codePair(726,"8402"));
this.put("Rabid Wombat", new codePair(755,"8502"));
this.put("Tracker", new codePair(380,"8602"));
this.put("WallOfWonder", new codePair(850,"8702"));
this.put("Wormwood Tfolk", new codePair(389,"8802"));
this.put("AssemblyWrkr", new codePair(910,"8902"));
this.put("GiantWasp", new codePair(885,"8A02"));
this.put("BottleDjinn", new codePair(890,"8B02"));
this.put("SpawnofAzar", new codePair(900,"8C02"));
this.put("XCyclopean", new codePair(902,"8D02"));
this.put("Tetravite", new codePair(891,"8E02"));
this.put("Rukh", new codePair(892,"8F02"));
this.put("PoisonSnake", new codePair(887,"9002"));
this.put("Dummy", new codePair(916,"9102"));
this.put("Dummy", new codePair(916,"9202"));
this.put("Dummy", new codePair(916,"9302"));
this.put("DataCard", new codePair(909,"9402"));
this.put("Damage", new codePair(901,"9502"));
this.put("PowerUp", new codePair(903,"9602"));
this.put("Unblockable", new codePair(903,"9702"));
this.put("AddAbility", new codePair(903,"9802"));
this.put("Sleighted", new codePair(902,"9902"));
this.put("Hacked", new codePair(902,"9A02"));
this.put("Stoning", new codePair(903,"9B02"));
this.put("TakeAbility", new codePair(903,"9C02"));
this.put("NoAttack", new codePair(903,"9D02"));
this.put("DrawCard", new codePair(904,"9E02"));
this.put("Hunting", new codePair(905,"9F02"));
this.put("Polka", new codePair(903,"A002"));
this.put("MarshGas", new codePair(903,"A102"));
this.put("FogEffect", new codePair(903,"A202"));
//this.put("Channel", new codePair(902,"A302")); //TODO - Why does this have 2 entries?
this.put("Generic", new codePair(903,"A402"));
this.put("Asp Sting", new codePair(902,"A502"));
this.put("NetherLink", new codePair(902,"A602"));
this.put("Graveyard", new codePair(902,"A702"));
this.put("Activation", new codePair(906,"A802"));
this.put("PoltergeistFX", new codePair(903,"A902"));
this.put("EbonyHorseFX", new codePair(903,"AA02"));
this.put("TitaniasLeg", new codePair(902,"AB02"));
this.put("DisintegrtFX", new codePair(903,"AC02"));
this.put("SirensCallFX", new codePair(903,"AD02"));
this.put("Damage Legacy", new codePair(902,"AE02"));
this.put("AsteriskFX", new codePair(903,"AF02"));
this.put("PiggyFX", new codePair(903,"B002"));
this.put("TElementalFX", new codePair(903,"B102"));
this.put("RukhEggFX", new codePair(903,"B202"));
this.put("NettlingImpFX", new codePair(903,"B302"));
this.put("ErhnamDjinnFX", new codePair(903,"B402"));
this.put("DesertFX", new codePair(903,"B502"));
this.put("PGremlinFX", new codePair(903,"B602"));
this.put("XmorgrantFX", new codePair(903,"B702"));
this.put("CTombFX", new codePair(903,"B802"));
this.put("GuardianFX", new codePair(903,"B902"));
this.put("IfhBiffFX", new codePair(902,"BA02"));
this.put("SewerFX", new codePair(903,"BB02"));
this.put("ControlFX", new codePair(903,"BC02"));
this.put("LivingLandFX", new codePair(903,"BD02"));
this.put("BlazeFX", new codePair(903,"BE02"));
this.put("RiverFX", new codePair(903,"BF02"));
this.put("BeastFX", new codePair(903,"C002"));
this.put("TransmuteFX", new codePair(903,"C102"));
this.put("Dummy", new codePair(65535,"C202"));
this.put("Dummy", new codePair(65535,"C302"));
this.put("Dummy", new codePair(65535,"C402"));
this.put("Dummy", new codePair(65535,"C502"));
this.put("Dummy", new codePair(65535,"C602"));
this.put("Dummy", new codePair(65535,"C702"));
this.put("Dummy", new codePair(65535,"C802"));
    }catch(Exception e){
        JOptionPane.showMessageDialog(null, "Error whilst populating card list in allCards class." + nl + "e.getMessage():"
                + e.getMessage(), null, JOptionPane.ERROR_MESSAGE);
    }
    }

/**
 * Takes a String representing the code of a card in a game deck
 * and returns that card's name as a String. Strips the gameCode
 * down to just the first 2 bytes and also takes care of some funny
 * business with cards in no decks or just deck 2.
 *
 * The funny business is that if a card isn't in any decks or is only in deck 2 then
 * the 2nd byte in its code is set to a different value from the rest. This means if we
 * have a card like Flood that is normally 0102 it is changed to 0142 which stops it
 * being found. Actually this happens in other cases as well so rather than find them all
 * to take care of it we just set it back to 0 if it is 4. We should really check
 * what cases cause this and look for those cases but it looks like that would be more
 * trouble that it is worth. There is no needed to write in the 4 when saving as it
 * works fine without it however the game will add it back in when saving normally.
 *
 * A new way of reading the cards (basically replacing the 4 with a 0 when we read)
 * has been added so this shouldn't be a problem at this stage anymore.
 *
 * Returns null if the code is not in the map.
 * @param gameCode A String representation of the hexadecimal game card code
 * @return String
 */
public String gameToName(String gameCode) {
        try {
            if (gameCode.length() > 4) {
                if (gameCode.substring(2, 3).equals("4")){ //deal with funny business
                    gameCode = gameCode.substring(0, 2) + "0" + gameCode.substring(3, 4);
                }else{
                gameCode = gameCode.substring(0, 4);
                }
            }
            Iterator<Entry<String, codePair>> i = this.entrySet().iterator();
            while (i.hasNext()) {
                Entry<String, codePair> currentEntry = i.next();
                String currentCode = currentEntry.getValue().getgameCode();
                if (currentCode.equals(gameCode)) {
                    return currentEntry.getKey();
                }
            }
            return null;
        } catch (Exception e) {
            JOptionPane.showMessageDialog(null, "Error whilst converting from game card code to name in allCards class." + nl + "e.getMessage():"
                    + e.getMessage(), null, JOptionPane.ERROR_MESSAGE);
            return null;
        }
    }

/**
 * Takes an int representing the code of a card in a duel deck
 * and returns that card's name as a String.
 *
 * Returns null if the code is not in the map.
 * @param duelCode An int representation of the decimal duel card code
 * @return String
 */
public String duelToName(int duelCode) {
        try {
            Iterator<Entry<String, codePair>> i = this.entrySet().iterator();
            while (i.hasNext()) {
                Entry<String, codePair> currentEntry = i.next();
                int currentCode = currentEntry.getValue().getduelCode();
                if (currentCode == duelCode) {
                    return currentEntry.getKey();
                }
            }
            return null;
        } catch (Exception e) {
            JOptionPane.showMessageDialog(null, "Error whilst converting from duel card code to name in allCards class." + nl + "e.getMessage():"
                    + e.getMessage(), null, JOptionPane.ERROR_MESSAGE);
            return null;
        }
    }

/**
 * Takes an int representing the code of a card in a duel deck
 * and returns that card's game code as a String.
 *
 * Returns null if the code is not in the map.
 * @param duelCode An int representation of the decimal duel card code
 * @return String
 */
public String duelToGame(int duelCode) {
        try {
            Iterator<Entry<String, codePair>> i = this.entrySet().iterator();
            while (i.hasNext()) {
                Entry<String, codePair> currentEntry = i.next();
                int currentCode = currentEntry.getValue().getduelCode();
                if (currentCode == duelCode) {
                    return currentEntry.getValue().getgameCode();
                }
            }
            return null;
        } catch (Exception e) {
            JOptionPane.showMessageDialog(null, "Error whilst converting from duel card code to game card code in allCards class." + nl + "e.getMessage():"
                    + e.getMessage(), null, JOptionPane.ERROR_MESSAGE);
            return null;
        }
}

/**
 * Takes a String representing the code of a card stored in a game deck and returns
 * an int representation of the same card in a duel deck.
 *
 * Returns -1 if the card is not in the map.
 * @param gameCode The code of the card to convert
 * @return
 */
public int gameToDuel(String gameCode){
        try {
            Iterator<Entry<String, codePair>> i = this.entrySet().iterator();
            while (i.hasNext()) {
                Entry<String, codePair> currentEntry = i.next();
                String currentCode = currentEntry.getValue().getgameCode();
                if (currentCode.equals(gameCode)){
                    return currentEntry.getValue().getduelCode();
                }
            }
            return -1;
        } catch (Exception e) {
            JOptionPane.showMessageDialog(null, "Error whilst converting from game card code to duel card code in allCards class." + nl + "e.getMessage():"
                    + e.getMessage(), null, JOptionPane.ERROR_MESSAGE);
            return -1;
        }
}
}
