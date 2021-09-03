//----- (401000) --------------------------------------------------------
rfalse_1()
//----- (401020) --------------------------------------------------------
sub_401020()
//----- (4010A0) --------------------------------------------------------
sub_4010A0()
//----- (4010F0) --------------------------------------------------------
sub_4010F0()
//----- (401120) --------------------------------------------------------
sub_401120()
//----- (401180) --------------------------------------------------------
sub_401180(int a1, int a2)
//----- (4012B0) --------------------------------------------------------
sub_4012B0(int a1, int a2)
//----- (4013F0) --------------------------------------------------------
sub_4013F0(int a1)
//----- (401490) --------------------------------------------------------
sub_401490()
//----- (4014B0) --------------------------------------------------------
sub_4014B0(char a1, char a2)
//----- (4014D0) --------------------------------------------------------
sub_4014D0(int a1)
//----- (401500) --------------------------------------------------------
sub_401500(int a1)
//----- (401520) --------------------------------------------------------
sub_401520(int a1)
//----- (401540) --------------------------------------------------------
sub_401540()
//----- (401550) --------------------------------------------------------
sub_401550()
//----- (401570) --------------------------------------------------------
sub_401570()
//----- (401590) --------------------------------------------------------
sub_401590()
//----- (4015A0) --------------------------------------------------------
sub_4015A0()
//----- (4015B0) --------------------------------------------------------
sub_4015B0()
//----- (4015C0) --------------------------------------------------------
sub_4015C0()
//----- (4015D0) --------------------------------------------------------
sub_4015D0()
//----- (4015F0) --------------------------------------------------------
sub_4015F0(int a1, int a2)
//----- (401680) --------------------------------------------------------
j_malloc(size_t size)
//----- (401690) --------------------------------------------------------
j_free(void *memory)
//----- (4016A0) --------------------------------------------------------
fx_perm_color_903(int player, int card, event_t event)
//----- (401710) --------------------------------------------------------
get_color<eax>(int player<eax>, int card<ecx>)
//----- (401730) --------------------------------------------------------
change_color_and_kill_other_change_color_effects(int tar_player, int tar_card, int32_t TENTATIVE_new_color, int player, int card)
//----- (4017F0) --------------------------------------------------------
change_color(int tar_player, int tar_card, int32_t TENTATIVE_new_color, int player, int card)
//----- (401840) --------------------------------------------------------
messagebox_eax(int a1<eax>, int a2<ecx>)
//----- (401880) --------------------------------------------------------
finalize_graveyard_to_hand(int player<eax>, int card<ecx>)
//----- (4018C0) --------------------------------------------------------
call_cards_function<eax>(card_data_t *cd<eax>, int player, int card, event_t event)
//----- (401970) --------------------------------------------------------
sub_401970()
//----- (4019E0) --------------------------------------------------------
wrong_parameters_popup(int a1<eax>, int a3<ecx>)
//----- (401A40) --------------------------------------------------------
in_play_and_card_instance_in_esi<zf>(int player<eax>, int card<ecx>)
//----- (401A80) --------------------------------------------------------
get_card_instance<esi>(int player<eax>, int card<ecx>)
//----- (401AB0) --------------------------------------------------------
get_displayed_card_instance<esi>(int player<eax>, int card<ecx>)
//----- (401AD0) --------------------------------------------------------
get_card_instance_into_edi_no_error_for_150<edi>(int player<eax>, int card<ecx>)
//----- (401AF0) --------------------------------------------------------
in_play_and_card_instance_in_edi<zf>(int player<eax>, int card<ecx>)
//----- (401B20) --------------------------------------------------------
converted_mana_cost<eax>(card_instance_t *instance<esi>)
//----- (401B50) --------------------------------------------------------
TENTATIVE_64bit_multiply(__int64 a1, unsigned int a2)
//----- (401B70) --------------------------------------------------------
sub_401B70(int a1, unsigned int a2, unsigned int a3)
//----- (401B90) --------------------------------------------------------
j_SelectObject(HDC hdc, HGDIOBJ h)
//----- (401BB0) --------------------------------------------------------
comes_into_play_trigger(int player<eax>, int card<ecx>)
//----- (401C10) --------------------------------------------------------
sub_401C10<eax>(CHAR *a1<eax>)
//----- (401C40) --------------------------------------------------------
checked_malloc<eax>(size_t size<eax>)
//----- (401C50) --------------------------------------------------------
checked_free(void *memory<eax>)
//----- (401C60) --------------------------------------------------------
sub_401C60<eax>(int a1<eax>, int a2<ebp>)
//----- (401CA0) --------------------------------------------------------
get_random_creature_type(int a1)
//----- (401DC0) --------------------------------------------------------
hide_window_for_GetDlgItem<eax>(HWND hwnd<esi>, int nIDDlgItem)
//----- (401DE0) --------------------------------------------------------
show_window_for_GetDlgItem<eax>(HWND a1<esi>, int nIDDlgItem)
//----- (401E00) --------------------------------------------------------
UpdateWindow_for_GetDlgItem<eax>(HWND a1<esi>, int nIDDlgItem)
//----- (401E20) --------------------------------------------------------
C_get_protections_from(int player, int card)
//----- (401ECC) --------------------------------------------------------
random_24_bits()
//----- (401F08) --------------------------------------------------------
seed_rng()
//----- (401F2C) --------------------------------------------------------
load_text(int a4, char *a5)
//----- (402133) --------------------------------------------------------
read_digits_to_eol<eax>(int a1<esi>)
//----- (402154) --------------------------------------------------------
UNKNOWN_load_text_helper_function(unsigned int a1<ebp>, unsigned int a2<esi>)
//----- (402170) --------------------------------------------------------
read_cards_dat()
//----- (402440) --------------------------------------------------------
cheat_mana_for_spell(int iid<eax>, int player<ecx>)
//----- (4024F0) --------------------------------------------------------
add_to_total_power_of_creatures_by_color_and_total_toughness_of_creatures_by_color<eax>(int result<eax>, int toughness<edx>, int power<ecx>, int player<ebx>)
//----- (402530) --------------------------------------------------------
return_cards_txt<edi>()
//----- (402538) --------------------------------------------------------
hidword_is_cards_txt_lodword_is_offset_by_csvid<edx:eax>(int csvid<eax>)
//----- (402593) --------------------------------------------------------
sub_402593<edx>(card_data_t *a1<edi>)
//----- (4025E0) --------------------------------------------------------
TENTATIVE_modify_mana_prod__returns_eax_and_ecx<eax>(int player, int card)
//----- (402680) --------------------------------------------------------
charge_spell_cost<eax>(card_data_t *a1<edi>, int player, int card)
//----- (402930) --------------------------------------------------------
C_compute_and_check_casting_cost(int player, int a2, int card)
//----- (402B34) --------------------------------------------------------
get_card_ptr_data<esi>()
//----- (402B3C) --------------------------------------------------------
UNKNOWN_TENTATIVE_read_card_dat_helper2<eax>(int a1<ebp>)
//----- (402B48) --------------------------------------------------------
alloc_cards_txt()
//----- (402B60) --------------------------------------------------------
TENTATIVE_something_to_do_with_paying_mana<eax>(card_instance_t *instance<eax>, int player<ecx>)
//----- (402BB0) --------------------------------------------------------
sub_402BB0<eax>(int player<eax>, int card<ecx>)
//----- (402BE0) --------------------------------------------------------
check_mana_multi(int player)
//----- (402F80) --------------------------------------------------------
sub_402F80(int a1, int a2)
//----- (403010) --------------------------------------------------------
no_other_card_in_play_with_same_csvid(int player, int card)
//----- (403090) --------------------------------------------------------
C_card_generic_legend(int player, int card, event_t event)
//----- (403130) --------------------------------------------------------
C_card_angel_of_mercy(int player, int card, event_t event)
//----- (4031A0) --------------------------------------------------------
card_angelic_page(int player, int card, event_t event)
//----- (4032F0) --------------------------------------------------------
card_aven_flock(int player, int card, event_t event)
//----- (403540) --------------------------------------------------------
C_card_adun_oakenshield(int player, int card, event_t event)
//----- (403840) --------------------------------------------------------
C_card_dakkon_blackblade(int player, int card, event_t event)
//----- (4038D0) --------------------------------------------------------
C_card_gwendlyn_di_corci(int player, int card, event_t event)
//----- (4039E0) --------------------------------------------------------
C_card_jacques_le_vert(int player, int card, event_t event)
//----- (403A80) --------------------------------------------------------
C_card_lady_caleria(int player, int card, event_t event)
//----- (403B90) --------------------------------------------------------
mana_producer_tapped(int player, int card, event_t event)
//----- (403C00) --------------------------------------------------------
card_beast_of_burden(int player, int card, event_t event)
//----- (403C60) --------------------------------------------------------
card_ensnaring_bridge(int player, int card, event_t event)
//----- (403CD0) --------------------------------------------------------
card_glorious_anthem(int player, int card, event_t event)
//----- (403D20) --------------------------------------------------------
C_card_noble_purpose(int player, int card, event_t event)
//----- (403E70) --------------------------------------------------------
C_card_pacifism(int player, int card, event_t event)
//----- (404020) --------------------------------------------------------
C_card_rolling_stones(int player, int card, event_t event)
//----- (4040D0) --------------------------------------------------------
C_remove_wall_can_attack_if_no_subtype(int a1, int a2, int player, int card, int a5)
//----- (404100) --------------------------------------------------------
card_sanctimony(int player, int card, event_t event)
//----- (4041C0) --------------------------------------------------------
C_card_rubinia_soulsinger(int player, int card, event_t event)
//----- (4046B0) --------------------------------------------------------
card_ambitions_cost(int player, int card, event_t event)
//----- (404780) --------------------------------------------------------
card_anaba_shaman(int player, int card, event_t event)
//----- (4048A0) --------------------------------------------------------
card_archivist(int player, int card, event_t event)
//----- (404900) --------------------------------------------------------
card_aven_cloudchaser(int player, int card, event_t event)
//----- (4049B0) --------------------------------------------------------
C_card_volcanic_geyser(int player, int card, event_t event)
//----- (404AB0) --------------------------------------------------------
C_card_blanchwood_armor(int player, int card, event_t event)
//----- (404BE0) --------------------------------------------------------
card_aven_fisher(int player, int card, event_t event)
//----- (404C70) --------------------------------------------------------
card_balance_of_power(int player, int card, event_t event)
//----- (404D40) --------------------------------------------------------
card_bloodshot_cyclops(int player, int card, event_t event)
//----- (404F50) --------------------------------------------------------
card_boil(int player, int card, event_t event)
//----- (404FC0) --------------------------------------------------------
C_card_call_of_the_wild(int player, int card, event_t event)
//----- (405100) --------------------------------------------------------
card_carrion_wall(int player, int card, event_t event)
//----- (405230) --------------------------------------------------------
card_catalog(int player, int card, event_t event)
//----- (405360) --------------------------------------------------------
C_card_chastise(int player, int card, event_t event)
//----- (405430) --------------------------------------------------------
card_coastal_hornclaw(int player, int card, event_t event)
//----- (405570) --------------------------------------------------------
card_choke(int player, int card, event_t event)
//----- (405610) --------------------------------------------------------
card_cinder_wall(int player, int card, event_t event)
//----- (405650) --------------------------------------------------------
C_card_coastal_piracy(int player, int card, event_t event)
//----- (4057C0) --------------------------------------------------------
C_card_coat_of_arms(int player, int card, event_t event)
//----- (405930) --------------------------------------------------------
card_confiscate(int player, int card, event_t event)
//----- (405980) --------------------------------------------------------
card_coercion(int player, int card, event_t event)
//----- (405AD0) --------------------------------------------------------
card_collective_unconscious(int player, int card, event_t event)
//----- (405B60) --------------------------------------------------------
card_concentrate(int player, int card, event_t event)
//----- (405BA0) --------------------------------------------------------
card_creeping_mold(int player, int card, event_t event)
//----- (405C70) --------------------------------------------------------
card_davenant_archer(int player, int card, event_t event)
//----- (405D70) --------------------------------------------------------
C_card_curiosity(int player, int card, event_t event)
//----- (405F70) --------------------------------------------------------
C_card_horn_of_deafening(int player, int card, event_t event)
//----- (406100) --------------------------------------------------------
C_card_maze_of_ith(int player, int card, event_t event)
//----- (406250) --------------------------------------------------------
C_card_angus_mackenzie(int player, int card, event_t event)
//----- (406360) --------------------------------------------------------
C_card_bartel_runeaxe(int player, int card, event_t event)
//----- (406380) --------------------------------------------------------
C_card_axelrod_gunnarson(int player, int card, event_t event)
//----- (4065A0) --------------------------------------------------------
C_card_boris_devilboon(int player, int card, event_t event)
//----- (406700) --------------------------------------------------------
C_card_tor_wauki(int player, int card, event_t event)
//----- (406810) --------------------------------------------------------
C_card_tetsuo_umezawa(int player, int card, event_t event)
//----- (4069B0) --------------------------------------------------------
C_card_sunastian_falconer(int player, int card, event_t event)
//----- (406A80) --------------------------------------------------------
C_card_princess_lucrezia(int player, int card, event_t event)
//----- (406B50) --------------------------------------------------------
C_card_riven_turnbull(int player, int card, event_t event)
//----- (406C20) --------------------------------------------------------
C_card_ramses_overdark(int player, int card, event_t event)
//----- (406D60) --------------------------------------------------------
card_niall_silvain(int player, int card, event_t event)
//----- (406FB0) --------------------------------------------------------
C_card_ragnar(int player, int card, event_t event)
//----- (407250) --------------------------------------------------------
C_card_pavel_maliki(int player, int card, event_t event)
//----- (4074A0) --------------------------------------------------------
C_card_palladia_mors(int player, int card, event_t event)
//----- (407530) --------------------------------------------------------
C_card_xira_arien(int player, int card, event_t event)
//----- (407680) --------------------------------------------------------
C_card_willow_satyr(int player, int card, event_t event)
//----- (407B70) --------------------------------------------------------
C_card_arcades_sabboth(int player, int card, event_t event)
//----- (407E40) --------------------------------------------------------
C_card_tuknir_deathlock(int player, int card, event_t event)
//----- (407FD0) --------------------------------------------------------
C_card_vaevictus_asmadi(int player, int card, event_t event)
//----- (408330) --------------------------------------------------------
card_daring_apprentice(int player, int card, event_t event)
//----- (408460) --------------------------------------------------------
card_dark_banishing(int player, int card, event_t event)
//----- (408520) --------------------------------------------------------
card_death_pit_offering(int player, int card, event_t event)
//----- (40860A) --------------------------------------------------------
card_death_pits_of_rath(int player, int card, event_t event)
//----- (408670) --------------------------------------------------------
C_card_deathgazer(int player, int card, event_t event)
//----- (408820) --------------------------------------------------------
card_deepwood_ghoul(int player, int card, event_t event)
//----- (4088D0) --------------------------------------------------------
C_card_dehydration(int player, int card, event_t event)
//----- (408AB0) --------------------------------------------------------
card_demolish(int player, int card, event_t event)
//----- (408B70) --------------------------------------------------------
card_demystify(int player, int card, event_t event)
//----- (408C20) --------------------------------------------------------
card_distorting_lens(int player, int card, event_t event)
//----- (408D80) --------------------------------------------------------
card_eastern_paladin(int player, int card, event_t event)
//----- (408EA0) --------------------------------------------------------
card_elite_archers(int player, int card, event_t event)
//----- (408F90) --------------------------------------------------------
card_elite_javelineer(int player, int card, event_t event)
//----- (409040) --------------------------------------------------------
C_card_elvish_champion(int player, int card, event_t event)
//----- (4090F0) --------------------------------------------------------
card_elvish_lyrist(int player, int card, event_t event)
//----- (409230) --------------------------------------------------------
card_elvish_pioneer(int player, int card, event_t event)
//----- (409410) --------------------------------------------------------
C_card_elvish_piper(int player, int card, event_t event)
//----- (4094E0) --------------------------------------------------------
TENTATIVE_put_creature_from_hand_into_play(int player)
//----- (409620) --------------------------------------------------------
card_elvish_scrapper(int player, int card, event_t event)
//----- (409758) --------------------------------------------------------
card_emperor_crocodile(int player, int card, event_t event)
//----- (4097F0) --------------------------------------------------------
C_card_abyssal_specter(int player, int card, event_t event)
//----- (4098B0) --------------------------------------------------------
card_execute(int player, int card, event_t event)
//----- (409970) --------------------------------------------------------
card_flash_counter(int player, int card, event_t event)
//----- (409B20) --------------------------------------------------------
card_evacuation(int player, int card, event_t event)
//----- (409C20) --------------------------------------------------------
C_card_soul_net(int player, int card, event_t event)
//----- (409DB0) --------------------------------------------------------
card_gloom(int player, int card, event_t event)
//----- (409EC0) --------------------------------------------------------
card_fecundity(int player, int card, event_t event)
//----- (40A080) --------------------------------------------------------
card_defense_grid(int player, int card, event_t event)
//----- (40A0B0) --------------------------------------------------------
C_card_fertile_ground(int player, int card, event_t event)
//----- (40A340) --------------------------------------------------------
C_card_dark_heart_of_the_wood(int player, int card, event_t event)
//----- (40A3F0) --------------------------------------------------------
C_card_goblin_caves(int player, int card, event_t event)
//----- (40A510) --------------------------------------------------------
card_stone_calendar(int player, int card, event_t event)
//----- (40A530) --------------------------------------------------------
card_fleeting_image(int player, int card, event_t event)
//----- (40A5C0) --------------------------------------------------------
card_tivadars_crusade(int player, int card, event_t event)
//----- (40A650) --------------------------------------------------------
card_planar_gate(int player, int card, event_t event)
//----- (40A690) --------------------------------------------------------
card_mana_matrix(int player, int card, event_t event)
//----- (40A6D0) --------------------------------------------------------
card_underworld_dreams(int player, int card, event_t event)
//----- (40A740) --------------------------------------------------------
card_spell_blast(int player, int card, event_t event)
//----- (40A980) --------------------------------------------------------
card_fireball(int player, int card, event_t event)
//----- (40AFF0) --------------------------------------------------------
C_card_fungusaur(int player, int card, event_t event)
//----- (40B0F0) --------------------------------------------------------
card_fodder_cannon(int player, int card, event_t event)
//----- (40B260) --------------------------------------------------------
card_foratog(int player, int card, event_t event)
//----- (40B410) --------------------------------------------------------
C_card_fyndhorn_elder(int player, int card, event_t event)
//----- (40B540) --------------------------------------------------------
C_card_severed_legion(int player, int card, event_t event)
//----- (40B5A0) --------------------------------------------------------
haste(int player, int card, event_t event)
//----- (40B5E0) --------------------------------------------------------
card_generic_cant_block(int player, int card, event_t event)
//----- (40B610) --------------------------------------------------------
fx_graveyard_902(int player, int card, event_t event)
//----- (40B920) --------------------------------------------------------
copy_to_display()
//----- (40BCA3) --------------------------------------------------------
sub_40BCA3<eax>(int a1<eax>, int a2<edx>, int a3<ecx>, int a4<esi>)
//----- (40BDF0) --------------------------------------------------------
sub_40BDF0<eax>(int a1<eax>, int a2<ebp>)
//----- (40BE30) --------------------------------------------------------
sub_40BE30<eax>(int player<eax>, int a2<ebp>)
//----- (40BE80) --------------------------------------------------------
C_card_library_of_leng(int player, int card, event_t event)
//----- (40C080) --------------------------------------------------------
C_card_tormods_crypt(int player, int card, event_t event)
//----- (40C1E0) --------------------------------------------------------
card_wand_of_ith(int player, int card, event_t event)
//----- (40C4C0) --------------------------------------------------------
card_active_volcano(int player, int card, event_t event)
//----- (40C760) --------------------------------------------------------
card_flash_flood(int player, int card, event_t event)
//----- (40CA00) --------------------------------------------------------
C_card_kei_takahashi(int player, int card, event_t event)
//----- (40CD00) --------------------------------------------------------
any_creatures_damaged()
//----- (40CD80) --------------------------------------------------------
C_card_lady_evangela(int player, int card, event_t event)
//----- (40CF70) --------------------------------------------------------
card_land_equilibrium(int player, int card, event_t event)
//----- (40D060) --------------------------------------------------------
card_arena_of_the_ancients(int player, int card, event_t event)
//----- (40D220) --------------------------------------------------------
C_card_karakas(int player, int card, event_t event)
//----- (40D470) --------------------------------------------------------
C_card_blessed_reversal(int player, int card, event_t event)
//----- (40D4E0) --------------------------------------------------------
card_hibernation(int player, int card, event_t event)
//----- (40D650) --------------------------------------------------------
card_horned_troll(int player, int card, event_t event)
//----- (40D6A0) --------------------------------------------------------
card_hunted_wumpus(int player, int card, event_t event)
//----- (40D710) --------------------------------------------------------
card_index(int player, int card, event_t event)
//----- (40D840) --------------------------------------------------------
card_inspiration(int player, int card, event_t event)
//----- (40D8E0) --------------------------------------------------------
card_intrepid_hero(int player, int card, event_t event)
//----- (40DA00) --------------------------------------------------------
C_card_intruder_alarm(int player, int card, event_t event)
//----- (40DC50) --------------------------------------------------------
C_card_larceny(int player, int card, event_t event)
//----- (40DD90) --------------------------------------------------------
card_lava_axe(int player, int card, event_t event)
//----- (40DE10) --------------------------------------------------------
card_lava_hounds(int player, int card, event_t event)
//----- (40DEB0) --------------------------------------------------------
card_lesser_gargadon(int player, int card, event_t event)
//----- (40DF80) --------------------------------------------------------
card_lhurgoyf(int player, int card, event_t event)
//----- (40DFF0) --------------------------------------------------------
card_lightning_blast(int player, int card, event_t event)
//----- (40E080) --------------------------------------------------------
card_llanowar_behemoth(int player, int card, event_t event)
//----- (40E210) --------------------------------------------------------
card_maggot_carrier(int player, int card, event_t event)
//----- (40E290) --------------------------------------------------------
card_mana_leak(int player, int card, event_t event)
//----- (40E4A0) --------------------------------------------------------
C_card_maro(int player, int card, event_t event)
//----- (40E510) --------------------------------------------------------
card_goldmeadow_harrier(int player, int card, event_t event)
//----- (40E610) --------------------------------------------------------
card_master_healer(int player, int card, event_t event)
//----- (40E910) --------------------------------------------------------
card_megrim(int player, int card, event_t event)
//----- (40E990) --------------------------------------------------------
C_card_elvish_visionary(int player, int card, event_t event)
//----- (40EA00) --------------------------------------------------------
C_card_merchant_scroll(int player, int card, event_t event)
//----- (40EC00) --------------------------------------------------------
card_might_of_oaks(int player, int card, event_t event)
//----- (40EDE0) --------------------------------------------------------
card_mind_rot(int player, int card, event_t event)
//----- (40EE70) --------------------------------------------------------
C_card_mind_sludge(int player, int card, event_t event)
//----- (40EF30) --------------------------------------------------------
card_curse_artifact(int player, int card, event_t event)
//----- (40F200) --------------------------------------------------------
C_card_goblin_shrine(int player, int card, event_t event)
//----- (40F3F0) --------------------------------------------------------
C_card_tangle_kelp(int player, int card, event_t event)
//----- (40F5E0) --------------------------------------------------------
C_card_grave_pact(int player, int card, event_t event)
//----- (40F730) --------------------------------------------------------
C_card_bribery(int player, int card, event_t event)
//----- (40F860) --------------------------------------------------------
card_chain_lightning(int player, int card, event_t event)
//----- (40FA90) --------------------------------------------------------
card_mogg_sentry(int player, int card, event_t event)
//----- (40FBC0) --------------------------------------------------------
card_titanic_growth(int player, int card, event_t event)
//----- (40FD10) --------------------------------------------------------
card_murderous_betrayal(int player, int card, event_t event)
//----- (40FE30) --------------------------------------------------------
C_card_nantuko_disciple(int player, int card, event_t event)
//----- (40FF70) --------------------------------------------------------
card_nausea(int player, int card, event_t event)
//----- (410020) --------------------------------------------------------
card_nekrataal(int player, int card, event_t event)
//----- (4100E0) --------------------------------------------------------
card_okk(int player, int card, event_t event)
//----- (4101D0) --------------------------------------------------------
C_card_orcish_spy(int player, int card, event_t event)
//----- (4102D0) --------------------------------------------------------
card_patagia_golem(int player, int card, event_t event)
//----- (410402) --------------------------------------------------------
card_panic_attack(int player, int card, event_t event)
//----- (410560) --------------------------------------------------------
fx_no_blocking_903(int player, int card, event_t event)
//----- (4105C0) --------------------------------------------------------
C_card_peach_garden_oath(int player, int card, event_t event)
//----- (410650) --------------------------------------------------------
C_card_persecute(int player, int card, event_t event)
//----- (4108C0) --------------------------------------------------------
C_card_phantom_warrior(int player, int card, event_t event)
//----- (410900) --------------------------------------------------------
C_card_phyrexian_arena(int player, int card, event_t event)
//----- (410A00) --------------------------------------------------------
C_card_phyrexian_plaguelord(int player, int card, event_t event)
//----- (410B70) --------------------------------------------------------
card_plague_wind(int player, int card, event_t event)
//----- (410C20) --------------------------------------------------------
C_card_planar_portal(int player, int card, event_t event)
//----- (410E80) --------------------------------------------------------
card_plow_under(int player, int card, event_t event)
//----- (410FB0) --------------------------------------------------------
put_on_top_of_deck(int player, int card)
//----- (411030) --------------------------------------------------------
card_primeval_force(int player, int card, event_t event)
//----- (4110D0) --------------------------------------------------------
C_card_puppeteer(int player, int card, event_t event)
//----- (411280) --------------------------------------------------------
card_pyroclasm(int player, int card, event_t event)
//----- (411340) --------------------------------------------------------
card_rain_of_blades(int player, int card, event_t event)
//----- (411410) --------------------------------------------------------
card_rampant_growth(int player, int card, event_t event)
//----- (4115F0) --------------------------------------------------------
card_ravenous_rats(int player, int card, event_t event)
//----- (411670) --------------------------------------------------------
card_reflexes(int player, int card, event_t event)
//----- (411790) --------------------------------------------------------
remove_STATE_TARGETTED_and_STATE_CANNOT_TARGET_from_targets_of_player_card<eax>()
//----- (4117C0) --------------------------------------------------------
card_winter_blast(int player, int card, event_t event)
//----- (411980) --------------------------------------------------------
C_card_the_tabernacle_at_pendrell_vale(int player, int card, event_t event)
//----- (411A70) --------------------------------------------------------
C_card_hammerheim(int player, int card, event_t event)
//----- (411D70) --------------------------------------------------------
C_card_pendelhaven(int player, int card, event_t event)
//----- (412060) --------------------------------------------------------
C_card_tolaria(int player, int card, event_t event)
//----- (412330) --------------------------------------------------------
C_card_urborg(int player, int card, event_t event)
//----- (4126B0) --------------------------------------------------------
card_aisling_leprechaun(int player, int card, event_t event)
//----- (4127E0) --------------------------------------------------------
UNKNOWN_aisling_leprechaun_helper<eax>(int a1<ebp>, int a2<ebx>, int a3<edi>)
//----- (412810) --------------------------------------------------------
C_card_revive(int player, int card, event_t event)
//----- (412960) --------------------------------------------------------
C_revive_helper(int player, int card, color_test_t colormask, unsigned __int8 argC, const char *a4)
//----- (412A30) --------------------------------------------------------
C_revive_helper2<eax>(int a1<eax>, unsigned __int8 a2<dl>)
//----- (412A70) --------------------------------------------------------
C_card_rewind(int player, int card, event_t event)
//----- (412D00) --------------------------------------------------------
C_card_sacred_nectar(int player, int card, event_t event)
//----- (412D50) --------------------------------------------------------
card_sage_owl(int player, int card, event_t event)
//----- (412EC0) --------------------------------------------------------
C_card_sea_monster(int player, int card, event_t event)
//----- (412F20) --------------------------------------------------------
C_card_searing_wind(int player, int card, event_t event)
//----- (412FB0) --------------------------------------------------------
card_seasoned_marshal(int player, int card, event_t event)
//----- (4130C0) --------------------------------------------------------
card_seismic_assault(int player, int card, event_t event)
//----- (4131B0) --------------------------------------------------------
UNUSED_card_serpent_warrior2(int player, int card, event_t event)
//----- (413220) --------------------------------------------------------
card_vampiric_spirit(int player, int card, event_t event)
//----- (4132A0) --------------------------------------------------------
C_card_sever_soul(int player, int card, event_t event)
//----- (413380) --------------------------------------------------------
card_shifting_sky(int player, int card, event_t event)
//----- (4134B0) --------------------------------------------------------
card_unyaro_bee_sting(int player, int card, event_t event)
//----- (413540) --------------------------------------------------------
card_moonglove_extract(int player, int card, event_t event)
//----- (4135D0) --------------------------------------------------------
C_card_sizzle(int player, int card, event_t event)
//----- (413620) --------------------------------------------------------
card_slay(int player, int card, event_t event)
//----- (4136E0) --------------------------------------------------------
card_sneaky_homunculus(int player, int card, event_t event)
//----- (413770) --------------------------------------------------------
card_shield_wall(int player, int card, event_t event)
//----- (413810) --------------------------------------------------------
card_solidarity(int player, int card, event_t event)
//----- (4138B0) --------------------------------------------------------
C_card_soul_feast(int player, int card, event_t event)
//----- (413960) --------------------------------------------------------
card_spellbook(int player, int card, event_t event)
//----- (413990) --------------------------------------------------------
C_card_spiketail_hatchling(int player, int card, event_t event)
//----- (413B90) --------------------------------------------------------
card_spitting_spider(int player, int card, event_t event)
//----- (413C80) --------------------------------------------------------
card_star_compass(int player, int card, event_t event)
//----- (413EE0) --------------------------------------------------------
C_card_lone_missionary(int player, int card, event_t event)
//----- (413F50) --------------------------------------------------------
C_card_venerable_monk(int player, int card, event_t event)
//----- (413FC0) --------------------------------------------------------
card_sudden_impact(int player, int card, event_t event)
//----- (414090) --------------------------------------------------------
card_sunweb(int player, int card, event_t event)
//----- (4140F0) --------------------------------------------------------
card_swarm_of_rats(int player, int card, event_t event)
//----- (414160) --------------------------------------------------------
card_sword_dancer(int player, int card, event_t event)
//----- (4142C0) --------------------------------------------------------
C_card_telepathy(int player, int card, event_t event)
//----- (414420) --------------------------------------------------------
card_temporal_adept(int player, int card, event_t event)
//----- (414520) --------------------------------------------------------
C_card_thieving_magpie(int player, int card, event_t event)
//----- (4145E0) --------------------------------------------------------
card_trade_routes(int player, int card, event_t event)
//----- (4147C0) --------------------------------------------------------
card_treasure_trove(int player, int card, event_t event)
//----- (4148B0) --------------------------------------------------------
card_tremor(int player, int card, event_t event)
//----- (414960) --------------------------------------------------------
card_urzas_armor(int player, int card, event_t event)
//----- (4149C0) --------------------------------------------------------
C_card_vernal_bloom(int player, int card, event_t event)
//----- (414B00) --------------------------------------------------------
C_card_petra_sphinx(int player, int card, event_t event)
//----- (414D30) --------------------------------------------------------
C_petra_sphinx_helper<eax>(int player<eax>)
//----- (414DD0) --------------------------------------------------------
C_card_vexing_arcanix(int player, int card, event_t event)
//----- (415060) --------------------------------------------------------
card_viashino_sandstalker(int player, int card, event_t event)
//----- (4150F0) --------------------------------------------------------
C_card_douse_in_gloom(int player, int card, event_t event)
//----- (4151A0) --------------------------------------------------------
card_warped_devotion(int player, int card, event_t event)
//----- (415230) --------------------------------------------------------
card_western_paladin(int player, int card, event_t event)
//----- (415350) --------------------------------------------------------
card_plummet(int player, int card, event_t event)
//----- (415400) --------------------------------------------------------
card_wood_elves(int player, int card, event_t event)
//----- (4155C0) --------------------------------------------------------
C_card_worship(int player, int card, event_t event)
//----- (415620) --------------------------------------------------------
card_wrath_of_marit_lage(int player, int card, event_t event)
//----- (415870) --------------------------------------------------------
C_card_yavimaya_enchantress(int player, int card, event_t event)
//----- (415910) --------------------------------------------------------
C_card_palace_guard_and_avatar_of_hope(int player, int card, event_t event)
//----- (415C60) --------------------------------------------------------
C_card_marhault_elsdragon(int player, int card, event_t event)
//----- (415C90) --------------------------------------------------------
card_aerathi_berserker(int player, int card, event_t event)
//----- (415CA0) --------------------------------------------------------
card_craw_giant(int player, int card, event_t event)
//----- (415CB0) --------------------------------------------------------
rampage<eax>(int rampageamt<eax>, int player, int card, event_t event)
//----- (415DA0) --------------------------------------------------------
C_card_akroma_angel_of_wrath(int player, int card, event_t event)
//----- (415E80) --------------------------------------------------------
enchant_world(int player, int card, event_t event)
//----- (415F40) --------------------------------------------------------
C_card_revelation(int player, int card, event_t event)
//----- (4160A0) --------------------------------------------------------
card_storm_world(int player, int card, event_t event)
//----- (416140) --------------------------------------------------------
card_the_abyss(int player, int card, event_t event)
//----- (416220) --------------------------------------------------------
card_concordant_crossroads(int player, int card, event_t event)
//----- (416280) --------------------------------------------------------
card_gravity_sphere(int player, int card, event_t event)
//----- (4162F0) --------------------------------------------------------
C_card_mirror_universe(int player, int card, event_t event)
//----- (416460) --------------------------------------------------------
C_card_chromium(int player, int card, event_t event)
//----- (416530) --------------------------------------------------------
TENTATIVE_dlgproc_mulligan(HWND hwnd, WM_t uMsg, WPARAM wparam, LPARAM lparam)
//----- (416F30) --------------------------------------------------------
draw_initial_hand(int player, signed int a2)
//----- (417040) --------------------------------------------------------
TENTATIVE_network_related(HWND hWnd)
//----- (417090) --------------------------------------------------------
TENTATIVE_swap_cardlist_over_network()
//----- (417170) --------------------------------------------------------
TENTATIVE_wait_on_mutex<eax>(void *a1<eax>)
//----- (417190) --------------------------------------------------------
TENTATIVE_wait_on_mutex_until_success_or_abandoned_or_failed<eax>(void *a1<eax>)
//----- (4171B0) --------------------------------------------------------
TENTATIVE_network_related_0<eax>(LPARAM a1<eax>, int a2<ecx>)
//----- (4171D0) --------------------------------------------------------
get_screen_width()
//----- (4171F0) --------------------------------------------------------
get_screen_height()
//----- (417210) --------------------------------------------------------
WILDGUESS_real_shuffle(int a1<eax>)
//----- (417280) --------------------------------------------------------
get_card_instance_of_player_card_and_set_eax_to_1<esi>()
//----- (4172A0) --------------------------------------------------------
get_card_instance_of_player_card_and_set_eax_to_1_<esi>()
//----- (4172C0) --------------------------------------------------------
sub_4172C0<eax>(int a1<ebp>)
//----- (4172E0) --------------------------------------------------------
get_card_instance_of_player_card_and_set_eax_to_1__<esi>()
//----- (417300) --------------------------------------------------------
TENTATIVE_set_target_card_and_controller_to_nth_target_of_player_card<eax>(int tgtidx<eax>)
//----- (417340) --------------------------------------------------------
TENTATIVE_set_target_card_and_controller_to_nth_target_of_player_card_unless_its_player<eax>(int tgtidx<eax>)
//----- (417380) --------------------------------------------------------
select_target_frontend<eax>(const char *prompt<edx>)
//----- (417400) --------------------------------------------------------
select_target_frontend_0<eax>(const char *prompt<edx>)
//----- (417490) --------------------------------------------------------
target_available_frontend()
//----- (4174C0) --------------------------------------------------------
target_available_frontend_0()
//----- (4174F0) --------------------------------------------------------
target_available_frontend_1()
//----- (417520) --------------------------------------------------------
target_available_frontend_2()
//----- (417550) --------------------------------------------------------
target_available_frontend_3()
//----- (417580) --------------------------------------------------------
target_available_frontend_4()
//----- (4175B0) --------------------------------------------------------
target_available_frontend_5()
//----- (4175E0) --------------------------------------------------------
target_available_frontend_6()
//----- (417610) --------------------------------------------------------
target_available_frontend_7()
//----- (4176A0) --------------------------------------------------------
target_available_frontend_8()
//----- (4176E0) --------------------------------------------------------
target_available_frontend_9()
//----- (417720) --------------------------------------------------------
target_available_frontend_10()
//----- (417760) --------------------------------------------------------
target_available_frontend_11()
//----- (4177A0) --------------------------------------------------------
target_available_frontend_12()
//----- (4177E0) --------------------------------------------------------
target_available_frontend_13()
//----- (417820) --------------------------------------------------------
target_available_frontend_14()
//----- (417860) --------------------------------------------------------
target_available_frontend_15()
//----- (4178A0) --------------------------------------------------------
target_available_frontend_16<eax>(int color<eax>)
//----- (417900) --------------------------------------------------------
target_available_frontend_17()
//----- (417940) --------------------------------------------------------
target_available_frontend_18<eax>(int color<eax>)
//----- (4179A0) --------------------------------------------------------
target_available_frontend_19<eax>(int color<eax>)
//----- (4179F0) --------------------------------------------------------
target_available_frontend_20<eax>(int color<eax>)
//----- (417A50) --------------------------------------------------------
target_available_frontend_21()
//----- (417A90) --------------------------------------------------------
target_available_frontend_22()
//----- (417AD0) --------------------------------------------------------
target_available_frontend_23<eax>(int power<eax>)
//----- (417B10) --------------------------------------------------------
target_available_frontend_24<eax>(int power<eax>, int toughness<ecx>)
//----- (417B50) --------------------------------------------------------
target_available_frontend_25<eax>(int power<eax>)
//----- (417B90) --------------------------------------------------------
target_available_frontend_26()
//----- (417BD0) --------------------------------------------------------
target_available_frontend_27()
//----- (417C10) --------------------------------------------------------
target_available_frontend_28()
//----- (417C50) --------------------------------------------------------
target_available_frontend_29()
//----- (417C90) --------------------------------------------------------
target_available_frontend_30<eax>(color_t color<eax>)
//----- (417CF0) --------------------------------------------------------
target_available_frontend_31()
//----- (417D30) --------------------------------------------------------
target_available_frontend_32()
//----- (417D70) --------------------------------------------------------
target_available_frontend_33()
//----- (417DB0) --------------------------------------------------------
target_available_frontend_34()
//----- (417DF0) --------------------------------------------------------
target_available_frontend_35()
//----- (417E30) --------------------------------------------------------
target_available_frontend_36()
//----- (417E80) --------------------------------------------------------
target_available_frontend_37()
//----- (417EC0) --------------------------------------------------------
target_available_frontend_38()
//----- (417F00) --------------------------------------------------------
target_available_frontend_39()
//----- (417F40) --------------------------------------------------------
target_available_frontend_40()
//----- (417F80) --------------------------------------------------------
target_available_frontend_41()
//----- (417FC0) --------------------------------------------------------
target_available_frontend_42()
//----- (418000) --------------------------------------------------------
target_available_frontend_43()
//----- (418040) --------------------------------------------------------
target_available_frontend_44<eax>(subtype_in_card_data_t reqsubtype<edx>)
//----- (418080) --------------------------------------------------------
target_available_frontend_45<eax>()
//----- (4180C0) --------------------------------------------------------
target_available_frontend_46()
//----- (418100) --------------------------------------------------------
target_available_frontend_47()
//----- (418140) --------------------------------------------------------
target_available_frontend_48()
//----- (418180) --------------------------------------------------------
target_available_frontend_49()
//----- (4181C0) --------------------------------------------------------
target_available_frontend_50<eax>(color_t color<eax>)
//----- (418220) --------------------------------------------------------
target_available_frontend_51()
//----- (418260) --------------------------------------------------------
target_available_frontend_52()
//----- (4182A0) --------------------------------------------------------
target_available_frontend_53()
//----- (4182E0) --------------------------------------------------------
target_available_frontend_54<eax>(int extra<eax>)
//----- (418320) --------------------------------------------------------
target_available_frontend_55<eax>(int extra<eax>)
//----- (418360) --------------------------------------------------------
target_available_frontend_56()
//----- (4183A0) --------------------------------------------------------
target_available_frontend_57()
//----- (4183E0) --------------------------------------------------------
target_available_frontend_58()
//----- (418420) --------------------------------------------------------
target_available_frontend_59()
//----- (418460) --------------------------------------------------------
target_available_frontend_60()
//----- (4184A0) --------------------------------------------------------
target_available_frontend_61()
//----- (4184E0) --------------------------------------------------------
target_available_frontend_62()
//----- (418520) --------------------------------------------------------
target_available_frontend_63()
//----- (418560) --------------------------------------------------------
target_available_frontend_64()
//----- (4185A0) --------------------------------------------------------
target_available_frontend_65()
//----- (4185E0) --------------------------------------------------------
target_available_frontend_66()
//----- (418620) --------------------------------------------------------
target_available_frontend_67()
//----- (418660) --------------------------------------------------------
target_available_frontend_68()
//----- (4186A0) --------------------------------------------------------
target_available_frontend_69()
//----- (4186E0) --------------------------------------------------------
target_available_frontend_70()
//----- (418720) --------------------------------------------------------
target_available_frontend_71()
//----- (418760) --------------------------------------------------------
target_available_frontend_72<eax>(type_t reqtyp<eax>)
//----- (418820) --------------------------------------------------------
validate_target_frontend<eax>(int targetidx<eax>)
//----- (418860) --------------------------------------------------------
validate_target_frontend_0<eax>(int targetidx<eax>)
//----- (4188A0) --------------------------------------------------------
validate_target_frontend_1<eax>(int targetidx<eax>)
//----- (4188E0) --------------------------------------------------------
validate_target_frontend_2<eax>(int targetidx<eax>)
//----- (418920) --------------------------------------------------------
validate_target_frontend_3<eax>(int reqtype<edx>, int targetidx<eax>)
//----- (418960) --------------------------------------------------------
validate_target_frontend_4<eax>(int targetidx<eax>)
//----- (4189A0) --------------------------------------------------------
validate_target_frontend_5<eax>(int targetidx<eax>)
//----- (4189E0) --------------------------------------------------------
validate_target_frontend_6<eax>(color_t color<edx>, int targetidx<eax>)
//----- (418A30) --------------------------------------------------------
validate_target_frontend_7<eax>(int targetidx<eax>)
//----- (418A70) --------------------------------------------------------
validate_target_frontend_8<eax>(int targetidx<eax>)
//----- (418AB0) --------------------------------------------------------
validate_target_frontend_9<eax>(int targetidx<eax>)
//----- (418AF0) --------------------------------------------------------
validate_target_frontend_10<eax>(int targetidx<eax>)
//----- (418B30) --------------------------------------------------------
validate_target_frontend_11<eax>(int extra<edx>, int targetidx<eax>)
//----- (418B70) --------------------------------------------------------
validate_target_frontend_12<eax>(int targetidx<eax>)
//----- (418BB0) --------------------------------------------------------
validate_target_frontend_13<eax>(int targetidx<eax>)
//----- (418BF0) --------------------------------------------------------
validate_target_frontend_14<eax>(int targetidx<eax>)
//----- (418C30) --------------------------------------------------------
validate_target_frontend_15<eax>(int targetidx<eax>)
//----- (418C70) --------------------------------------------------------
validate_target_frontend_16<eax>(int targetidx<eax>)
//----- (418CB0) --------------------------------------------------------
validate_target_frontend_17<eax>(int targetidx<eax>)
//----- (418CF0) --------------------------------------------------------
validate_target_frontend_18<eax>(int targetidx<eax>)
//----- (418D30) --------------------------------------------------------
validate_target_frontend_19<eax>(int targetidx<eax>)
//----- (418D70) --------------------------------------------------------
validate_target_frontend_20<eax>(int targetidx<eax>)
//----- (418DB0) --------------------------------------------------------
validate_target_frontend_21<eax>(int targetidx<eax>)
//----- (418DF0) --------------------------------------------------------
validate_target_frontend_22<eax>(int targetidx<eax>)
//----- (418E30) --------------------------------------------------------
validate_target_frontend_23<eax>(int targetidx<eax>)
//----- (418E70) --------------------------------------------------------
validate_target_frontend_24<eax>(color_t notcolor<edx>, int targetidx<eax>)
//----- (418EC0) --------------------------------------------------------
validate_target_frontend_25<eax>(color_t notcolor<edx>, int targetidx<eax>)
//----- (418F10) --------------------------------------------------------
validate_target_frontend_26<eax>(color_t color<edx>, int targetidx<eax>)
//----- (418F60) --------------------------------------------------------
validate_target_frontend_27<eax>(color_t landwalk<edx>, int targetidx<eax>)
//----- (418FC0) --------------------------------------------------------
validate_target_frontend_28<eax>(int targetidx<eax>)
//----- (419000) --------------------------------------------------------
validate_target_frontend_29<eax>(int targetidx<eax>)
//----- (419050) --------------------------------------------------------
validate_target_frontend_30<eax>(int targetidx<eax>)
//----- (419090) --------------------------------------------------------
validate_target_frontend_31<eax>(int maxpower<edx>, int targetidx<eax>)
//----- (4190D0) --------------------------------------------------------
validate_target_frontend_32<eax>(int power<ecx>, int toughness<edx>, int targetidx<eax>)
//----- (419110) --------------------------------------------------------
validate_target_frontend_33<eax>(int minpower<edx>, int targetidx<eax>)
//----- (419150) --------------------------------------------------------
validate_target_frontend_34<eax>(int targetidx<eax>)
//----- (419190) --------------------------------------------------------
validate_target_frontend_35<eax>(int targetidx<eax>)
//----- (4191D0) --------------------------------------------------------
validate_target_frontend_36<eax>(int targetidx<eax>)
//----- (419210) --------------------------------------------------------
validate_target_frontend_37<eax>(int targetidx<eax>)
//----- (419250) --------------------------------------------------------
validate_target_frontend_38<eax>(int targetidx<eax>)
//----- (419290) --------------------------------------------------------
validate_target_frontend_39<eax>(int targetidx<eax>)
//----- (4192D0) --------------------------------------------------------
validate_target_frontend_40<eax>(subtype_in_card_data_t subtype<edx>, int targetidx<eax>)
//----- (419310) --------------------------------------------------------
validate_target_frontend_41<eax>(int targetidx<eax>)
//----- (419350) --------------------------------------------------------
validate_target_frontend_42<eax>(int targetidx<eax>)
//----- (419390) --------------------------------------------------------
validate_target_frontend_43<eax>(int targetidx<eax>)
//----- (4193D0) --------------------------------------------------------
validate_target_frontend_44<eax>(int targetidx<eax>)
//----- (419410) --------------------------------------------------------
validate_target_frontend_45<eax>(int targetidx<eax>)
//----- (419450) --------------------------------------------------------
validate_target_frontend_46<eax>(int targetidx<eax>)
//----- (419490) --------------------------------------------------------
validate_target_frontend_47<eax>(int targetidx<eax>)
//----- (4194D0) --------------------------------------------------------
validate_target_frontend_48<eax>(int targetidx<eax>)
//----- (419510) --------------------------------------------------------
validate_target_frontend_49<eax>(int targetidx<eax>)
//----- (419590) --------------------------------------------------------
select_target_frontend_1<eax>(const char *prompt<edx>)
//----- (4195F0) --------------------------------------------------------
select_target_frontend_2<eax>(const char *prompt<edx>)
//----- (419650) --------------------------------------------------------
select_target_frontend_3<eax>(const char *prompt<edx>)
//----- (4196B0) --------------------------------------------------------
select_target_frontend_4<eax>(const char *prompt<edx>)
//----- (419710) --------------------------------------------------------
select_target_frontend_5<eax>(const char *prompt<edx>)
//----- (419770) --------------------------------------------------------
select_target_frontend_6<eax>(const char *prompt<edx>)
//----- (4197A0) --------------------------------------------------------
select_target_frontend_7<eax>(const char *prompt<edx>)
//----- (419810) --------------------------------------------------------
select_target_frontend_8<eax>(const char *prompt<edx>)
//----- (419880) --------------------------------------------------------
select_target_frontend_9<eax>(const char *prompt<edx>)
//----- (4198F0) --------------------------------------------------------
select_target_frontend_10<eax>(const char *prompt<edx>)
//----- (419960) --------------------------------------------------------
select_target_frontend_11<eax>(const char *prompt<edx>, color_t color<eax>)
//----- (4199E0) --------------------------------------------------------
select_target_frontend_12<eax>(const char *prompt<edx>)
//----- (419A50) --------------------------------------------------------
select_target_frontend_13<eax>(const char *prompt<edx>, color_t color<eax>)
//----- (419AD0) --------------------------------------------------------
select_target_frontend_14<eax>(const char *prompt<edx>, color_t color<eax>)
//----- (419B50) --------------------------------------------------------
select_target_frontend_15<eax>(const char *prompt<edx>, color_t color<eax>)
//----- (419BD0) --------------------------------------------------------
select_target_frontend_16<eax>(const char *prompt<edx>)
//----- (419C40) --------------------------------------------------------
select_target_frontend_17<eax>(const char *prompt<edx>, int power<eax>)
//----- (419CB0) --------------------------------------------------------
select_target_frontend_18<eax>(const char *prompt<edx>, int power<eax>, int toughness<ecx>)
//----- (419D20) --------------------------------------------------------
select_target_frontend_19<eax>(const char *prompt<edx>, int power<eax>)
//----- (419D90) --------------------------------------------------------
select_target_frontend_20<eax>(const char *prompt<edx>)
//----- (419E00) --------------------------------------------------------
select_target_frontend_21<eax>(const char *prompt<edx>)
//----- (419E70) --------------------------------------------------------
select_target_frontend_22<eax>(const char *prompt<edx>)
//----- (419EE0) --------------------------------------------------------
select_target_frontend_23<eax>(const char *prompt<edx>)
//----- (419F50) --------------------------------------------------------
select_target_frontend_24<eax>(const char *prompt<edx>, color_t landcolor<eax>)
//----- (419FE0) --------------------------------------------------------
select_target_frontend_25<eax>(const char *prompt<edx>)
//----- (41A050) --------------------------------------------------------
select_target_frontend_26<eax>(const char *prompt<edx>)
//----- (41A0C0) --------------------------------------------------------
select_target_frontend_27<eax>(const char *prompt<edx>)
//----- (41A130) --------------------------------------------------------
select_target_frontend_28<eax>(const char *prompt<edx>)
//----- (41A1A0) --------------------------------------------------------
select_target_frontend_29<eax>(const char *prompt<edx>)
//----- (41A210) --------------------------------------------------------
select_target_frontend_30<eax>(const char *prompt<edx>)
//----- (41A280) --------------------------------------------------------
select_target_frontend_31<eax>(const char *prompt<edx>)
//----- (41A2F0) --------------------------------------------------------
select_target_frontend_32<eax>(const char *prompt<edx>)
//----- (41A360) --------------------------------------------------------
select_target_frontend_33<eax>(const char *prompt<edx>)
//----- (41A3D0) --------------------------------------------------------
select_target_frontend_34<eax>(const char *prompt<edx>)
//----- (41A440) --------------------------------------------------------
select_target_frontend_35<eax>(const char *prompt<edx>)
//----- (41A4B0) --------------------------------------------------------
select_target_frontend_36<eax>(const char *prompt<edx>)
//----- (41A520) --------------------------------------------------------
select_target_frontend_37<eax>(const char *prompt<edx>)
//----- (41A590) --------------------------------------------------------
select_target_frontend_38<eax>(const char *prompt<edx>)
//----- (41A600) --------------------------------------------------------
select_target_frontend_39<eax>(const char *prompt<edx>)
//----- (41A670) --------------------------------------------------------
select_target_frontend_40<eax>(const char *prompt<edx>)
//----- (41A6E0) --------------------------------------------------------
select_target_frontend_41<eax>(const char *prompt<edx>, subtype_in_card_data_t reqsubtype<eax>)
//----- (41A750) --------------------------------------------------------
select_target_frontend_42<eax>(const char *prompt<edx>, int TENTATIVE_ignored<eax>)
//----- (41A7C0) --------------------------------------------------------
select_target_frontend_43<eax>(const char *prompt<edx>)
//----- (41A830) --------------------------------------------------------
select_target_frontend_44<eax>(const char *prompt<edx>)
//----- (41A890) --------------------------------------------------------
select_target_frontend_45<eax>(const char *prompt<edx>)
//----- (41A900) --------------------------------------------------------
select_target_frontend_46<eax>(const char *prompt<edx>, color_t color<eax>)
//----- (41A980) --------------------------------------------------------
select_target_frontend_47<eax>(const char *prompt<edx>)
//----- (41A9F0) --------------------------------------------------------
select_target_frontend_48<eax>(const char *prompt<edx>)
//----- (41AA60) --------------------------------------------------------
select_target_frontend_49<eax>(const char *prompt<edx>)
//----- (41AAC0) --------------------------------------------------------
select_target_frontend_50<eax>(const char *prompt<edx>)
//----- (41AB30) --------------------------------------------------------
select_target_frontend_51<eax>(const char *prompt<edx>)
//----- (41ABA0) --------------------------------------------------------
select_target_frontend_52<eax>(const char *prompt<edx>)
//----- (41AC10) --------------------------------------------------------
select_target_frontend_53<eax>(const char *prompt<edx>)
//----- (41AC70) --------------------------------------------------------
select_target_frontend_54<eax>(const char *prompt<edx>)
//----- (41ACE0) --------------------------------------------------------
select_target_frontend_55<eax>(const char *prompt<edx>, int extra<eax>)
//----- (41AD50) --------------------------------------------------------
select_target_frontend_56<eax>(const char *prompt<edx>)
//----- (41ADC0) --------------------------------------------------------
select_target_frontend_57<eax>(const char *prompt<edx>)
//----- (41AE30) --------------------------------------------------------
select_target_frontend_58<eax>(const char *prompt<edx>)
//----- (41AEA0) --------------------------------------------------------
select_target_frontend_59<eax>(const char *prompt<edx>)
//----- (41AF00) --------------------------------------------------------
UNKNOWN_mirror_universe_helper<eax>()
//----- (41AF50) --------------------------------------------------------
select_target_frontend_60<eax>(const char *prompt<edx>)
//----- (41AFB0) --------------------------------------------------------
select_target_frontend_61<eax>(const char *prompt<edx>)
//----- (41B010) --------------------------------------------------------
select_target_frontend_62<eax>(const char *prompt<edx>)
//----- (41B070) --------------------------------------------------------
select_target_frontend_63<eax>(const char *prompt<edx>)
//----- (41B0D0) --------------------------------------------------------
select_target_frontend_64<eax>(const char *prompt<edx>)
//----- (41B140) --------------------------------------------------------
select_target_frontend_65<eax>(const char *prompt<edx>)
//----- (41B1A0) --------------------------------------------------------
select_target_frontend_66<eax>(const char *prompt<edx>)
//----- (41B200) --------------------------------------------------------
select_target_frontend_67<eax>(const char *prompt<edx>)
//----- (41B260) --------------------------------------------------------
select_target_frontend_68<eax>(const char *prompt<edx>)
//----- (41B2D0) --------------------------------------------------------
select_target_frontend_69<eax>(const char *prompt<edx>)
//----- (41B340) --------------------------------------------------------
select_target_frontend_70<eax>(const char *prompt<edx>)
//----- (41B3B0) --------------------------------------------------------
select_target_frontend_71<eax>(const char *prompt<edx>, type_t reqtype<eax>)
//----- (41B420) --------------------------------------------------------
very_broken__sub_41B420()
//----- (41B480) --------------------------------------------------------
very_broken__sub_41B480()
//----- (41B4D1) --------------------------------------------------------
card_call_from_the_grave(int player, int card, event_t event)
//----- (41B710) --------------------------------------------------------
card_time_walk(int player, int card, event_t event)
//----- (41B800) --------------------------------------------------------
card_balance(int player, int card, event_t event)
//----- (41BB00) --------------------------------------------------------
card_braingeyser(int player, int card, event_t event)
//----- (41BBD0) --------------------------------------------------------
card_wheel_of_fortune(int player, int card, event_t event)
//----- (41BD90) --------------------------------------------------------
card_rebirth(int player, int card, event_t event)
//----- (41BF40) --------------------------------------------------------
card_winds_of_change(int player, int card, event_t event)
//----- (41C030) --------------------------------------------------------
C_card_timetwister(int player, int card, event_t event)
//----- (41C190) --------------------------------------------------------
draw_cards(int player, int numcards)
//----- (41C1D0) --------------------------------------------------------
card_channel(int player, int card, event_t event)
//----- (41C240) --------------------------------------------------------
card_energy_tap(int player, int card, event_t event)
//----- (41C300) --------------------------------------------------------
C_card_stream_of_life(int player, int card, event_t event)
//----- (41C410) --------------------------------------------------------
C_card_tranquility(int player, int card, event_t event)
//----- (41C4A0) --------------------------------------------------------
C_card_volcanic_eruption(int player, int card, event_t event)
//----- (41C7E0) --------------------------------------------------------
C_card_earthquake(int player, int card, event_t event)
//----- (41C940) --------------------------------------------------------
C_card_hurricane(int player, int card, event_t event)
//----- (41CAA0) --------------------------------------------------------
card_armageddon(int player, int card, event_t event)
//----- (41CB30) --------------------------------------------------------
card_tsunami(int player, int card, event_t event)
//----- (41CBF0) --------------------------------------------------------
card_ashes_to_ashes(int player, int card, event_t event)
//----- (41CD40) --------------------------------------------------------
card_desert_twister(int player, int card, event_t event)
//----- (41CE00) --------------------------------------------------------
count_cards_notinplay_with_iid(int player, int iid_needle)
//----- (41CE50) --------------------------------------------------------
count_cards_inplay_with_iid(int unused, int iid_needle, int controlling_player__neg1_for_anybody)
//----- (41CEC0) --------------------------------------------------------
card_detonate(int player, int card, event_t event)
//----- (41CFA0) --------------------------------------------------------
card_mana_clash(int player, int card, event_t event)
//----- (41D150) --------------------------------------------------------
card_word_of_binding(int player, int card, event_t event)
//----- (41D3F0) --------------------------------------------------------
C_card_raise_dead(int player, int card, event_t event)
//----- (41D610) --------------------------------------------------------
card_regrowth(int player, int card, event_t event)
//----- (41D7C0) --------------------------------------------------------
card_demonic_tutor(int player, int card, event_t event)
//----- (41D9D0) --------------------------------------------------------
card_untamed_wilds(int player, int card, event_t event)
//----- (41DBB0) --------------------------------------------------------
card_visions(int player, int card, event_t event)
//----- (41DD00) --------------------------------------------------------
C_card_mind_twist(int player, int card, event_t event)
//----- (41DEA0) --------------------------------------------------------
card_mind_bomb(int player, int card, event_t event)
//----- (41E250) --------------------------------------------------------
find_idx_of_card_in_graveyard_with_highest_castingcost_1pt5x_bias_towards_colorless_cost(signed int player, type_t typ)
//----- (41E2D0) --------------------------------------------------------
find_idx_of_card_in_deck_with_highest_castingcost_affordable_by_player_2x_bias_towards_colorless_cost_extreme_bias_towards_cost_0_some_bias_for_rarity(int player, int tgtplayer, type_t typ)
//----- (41E3B0) --------------------------------------------------------
find_idx_of_card_in_deckptr_with_highest_castingcost_affordable_by_player_2x_bias_towards_colorless_cost_extreme_bias_towards_cost_0_some_bias_for_rarity(int player, int unused, type_t typ, int *deck)
//----- (41E480) --------------------------------------------------------
card_wrath_of_god(int player, int card, event_t event)
//----- (41E510) --------------------------------------------------------
card_flashfires(int player, int card, event_t event)
//----- (41E580) --------------------------------------------------------
card_pyrotechnics(int player, int card, event_t event)
//----- (41E850) --------------------------------------------------------
card_disintegrate(int player, int card, event_t event)
//----- (41E9B0) --------------------------------------------------------
C_card_drain_life(int player, int card, event_t event)
//----- (41EBD0) --------------------------------------------------------
card_stone_rain(int player, int card, event_t event)
//----- (41EC90) --------------------------------------------------------
card_drain_power(int player, int card, event_t event)
//----- (41EDC0) --------------------------------------------------------
drain_power_draw_mana_from_land(int player, int card, int iid)
//----- (41EEA0) --------------------------------------------------------
register_ChatClass(const CHAR *a1)
//----- (41EF50) --------------------------------------------------------
destroy_ChatClass()
//----- (41EF70) --------------------------------------------------------
wndproc_ChatClass(HWND hwnd, WM_t msg, WPARAM wparam, LPARAM lparam)
//----- (41F220) --------------------------------------------------------
card_contract_from_below(int player, int card, event_t event)
//----- (41F310) --------------------------------------------------------
card_darkpact(int player, int card, event_t event)
//----- (41F510) --------------------------------------------------------
card_demonic_attorney(int player, int card, event_t event)
//----- (41F560) --------------------------------------------------------
C_card_drafnas_restoration(int player, int card, event_t event)
//----- (41F7E0) --------------------------------------------------------
card_reconstruction(int player, int card, event_t event)
//----- (41F9F0) --------------------------------------------------------
C_card_resurrection(int player, int card, event_t event)
//----- (41FC00) --------------------------------------------------------
card_shatterstorm(int player, int card, event_t event)
//----- (41FC90) --------------------------------------------------------
card_transmute_artifact(int player, int card, event_t event)
//----- (41FF80) --------------------------------------------------------
C_card_mox_emerald(int player, int card, event_t event)
//----- (41FFA0) --------------------------------------------------------
C_card_mox_jet(int player, int card, event_t event)
//----- (41FFC0) --------------------------------------------------------
C_card_mox_pearl(int player, int card, event_t event)
//----- (41FFE0) --------------------------------------------------------
C_card_mox_ruby(int player, int card, event_t event)
//----- (420000) --------------------------------------------------------
C_card_mox_sapphire(int player, int card, event_t event)
//----- (420020) --------------------------------------------------------
C_card_black_lotus(int player, int card, event_t event)
//----- (420280) --------------------------------------------------------
C_card_time_vault(int player, int card, event_t event)
//----- (4204F0) --------------------------------------------------------
card_black_vise(int player, int card, event_t event)
//----- (420650) --------------------------------------------------------
card_the_rack(int player, int card, event_t event)
//----- (4207B0) --------------------------------------------------------
C_card_ivory_tower(int player, int card, event_t event)
//----- (4208B0) --------------------------------------------------------
card_cursed_rack(int player, int card, event_t event)
//----- (420940) --------------------------------------------------------
card_aladdins_lamp(int player, int card, event_t event)
//----- (420E70) --------------------------------------------------------
C_card_feldons_cane(int player, int card, event_t event)
//----- (420F40) --------------------------------------------------------
card_mishras_war_machine(int player, int card, event_t event)
//----- (4210A0) --------------------------------------------------------
C_card_primal_clay(int player, int card, event_t event)
//----- (421260) --------------------------------------------------------
card_shapeshifter(int player, int card, event_t event)
//----- (421590) --------------------------------------------------------
C_card_tetravus(int player, int card, event_t event)
//----- (421850) --------------------------------------------------------
C_tetravus_helper(int player, int card, event_t event)
//----- (4218D0) --------------------------------------------------------
C_tetravus_helper2(int player, int card)
//----- (421930) --------------------------------------------------------
C_tetravus_helper3(int player)
//----- (4219A0) --------------------------------------------------------
C_tetravus_helper4(int who_chooses, int a4)
//----- (421B70) --------------------------------------------------------
C_card_tetravite(int player, int card, event_t event)
//----- (421BD0) --------------------------------------------------------
C_card_triskelion(int player, int card, event_t event)
//----- (421D50) --------------------------------------------------------
card_urzas_avenger(int player, int card, event_t event)
//----- (421F10) --------------------------------------------------------
C_card_millstone(int player, int card, event_t event)
//----- (422120) --------------------------------------------------------
card_celestial_prism(int player, int card, event_t event)
//----- (422360) --------------------------------------------------------
C_card_fellwar_stone(int player, int card, event_t event)
//----- (422650) --------------------------------------------------------
card_ashnods_battle_gear(int player, int card, event_t event)
//----- (422950) --------------------------------------------------------
card_tawnoss_weaponry(int player, int card, event_t event)
//----- (422CC0) --------------------------------------------------------
C_card_battering_ram(int player, int card, event_t event)
//----- (422D90) --------------------------------------------------------
C_card_candelabra_of_tawnos(int player, int card, event_t event)
//----- (4230E0) --------------------------------------------------------
card_clay_statue(int player, int card, event_t event)
//----- (423110) --------------------------------------------------------
card_diabolic_machine(int player, int card, event_t event)
//----- (423140) --------------------------------------------------------
card_forcefield(int player, int card, event_t event)
//----- (4233C0) --------------------------------------------------------
card_disrupting_scepter(int player, int card, event_t event)
//----- (423530) --------------------------------------------------------
card_howling_mine(int player, int card, event_t event)
//----- (423560) --------------------------------------------------------
C_card_blue_mana_battery(int player, int card, event_t event)
//----- (423580) --------------------------------------------------------
C_card_red_mana_battery(int player, int card, event_t event)
//----- (4235A0) --------------------------------------------------------
C_card_green_mana_battery(int player, int card, event_t event)
//----- (4235C0) --------------------------------------------------------
C_card_white_mana_battery(int player, int card, event_t event)
//----- (4235E0) --------------------------------------------------------
C_card_black_mana_battery(int player, int card, event_t event)
//----- (423600) --------------------------------------------------------
C_mana_battery(int player, int card, event_t event, color_t color)
//----- (423A30) --------------------------------------------------------
card_conservator(int player, int card, event_t event)
//----- (423D20) --------------------------------------------------------
C_card_iron_star(int player, int card, event_t event)
//----- (423D40) --------------------------------------------------------
C_card_ivory_cup(int player, int card, event_t event)
//----- (423D60) --------------------------------------------------------
C_card_crystal_rod(int player, int card, event_t event)
//----- (423D80) --------------------------------------------------------
C_card_throne_of_bone(int player, int card, event_t event)
//----- (423DA0) --------------------------------------------------------
C_card_wooden_sphere(int player, int card, event_t event)
//----- (423DC0) --------------------------------------------------------
C_lucky_charm(int player, int card, event_t event, color_t color)
//----- (423F60) --------------------------------------------------------
card_ankh_of_mishra(int player, int card, event_t event)
//----- (424040) --------------------------------------------------------
C_card_armageddon_clock(int player, int card, event_t event)
//----- (424430) --------------------------------------------------------
card_dingus_egg(int player, int card, event_t event)
//----- (424560) --------------------------------------------------------
C_card_ebony_horse(int player, int card, event_t event)
//----- (424700) --------------------------------------------------------
card_jayemdae_tome(int player, int card, event_t event)
//----- (424800) --------------------------------------------------------
C_card_mana_vault(int player, int card, event_t event)
//----- (424B30) --------------------------------------------------------
C_card_sol_ring(int player, int card, event_t event)
//----- (424C20) --------------------------------------------------------
card_meekstone(int player, int card, event_t event)
//----- (424DC0) --------------------------------------------------------
C_card_jandors_saddlebags(int player, int card, event_t event)
//----- (424F30) --------------------------------------------------------
card_jade_monolith(int player, int card, event_t event)
//----- (425150) --------------------------------------------------------
redirect_damage_from_p_c_to_player(int redir_from_player, int redir_from_card, int redir_to_player)
//----- (4251E0) --------------------------------------------------------
C_card_onulet(int player, int card, event_t event)
//----- (4252B0) --------------------------------------------------------
card_amulet_of_kroog(int player, int card, event_t event)
//----- (425500) --------------------------------------------------------
card_grapeshot_catapult(int player, int card, event_t event)
//----- (4255E0) --------------------------------------------------------
card_bronze_tablet(int player, int card, event_t event)
//----- (425A40) --------------------------------------------------------
C_card_nevinyrrals_disk(int player, int card, event_t event)
//----- (425DC0) --------------------------------------------------------
destroy_creatures_then_destroy_artifacts_and_enchantments(int player, int card, int iid)
//----- (425E40) --------------------------------------------------------
card_aladdins_ring(int player, int card, event_t event)
//----- (425F70) --------------------------------------------------------
card_rod_of_ruin(int player, int card, event_t event)
//----- (426090) --------------------------------------------------------
card_winter_orb(int player, int card, event_t event)
//----- (426350) --------------------------------------------------------
card_brass_man(int player, int card, event_t event)
//----- (4263F0) --------------------------------------------------------
card_dragon_engine(int player, int card, event_t event)
//----- (4265E0) --------------------------------------------------------
C_card_clockwork_beast(int player, int card, event_t event)
//----- (426600) --------------------------------------------------------
C_card_clockwork_avian(int player, int card, event_t event)
//----- (426620) --------------------------------------------------------
C_clockwork_creature(int player, int card, event_t event, int initial_counters)
//----- (4268E0) --------------------------------------------------------
card_colossus_of_sardia(int player, int card, event_t event)
//----- (426990) --------------------------------------------------------
card_flying_carpet(int player, int card, event_t event)
//----- (426B40) --------------------------------------------------------
card_helm_of_chatzuk(int player, int card, event_t event)
//----- (426CF0) --------------------------------------------------------
card_coral_helm(int player, int card, event_t event)
//----- (426E60) --------------------------------------------------------
card_tawnoss_wand(int player, int card, event_t event)
//----- (426FF0) --------------------------------------------------------
C_card_the_hive(int player, int card, event_t event)
//----- (427110) --------------------------------------------------------
card_bottle_of_suleiman(int player, int card, event_t event)
//----- (4272C0) --------------------------------------------------------
card_glasses_of_urza(int player, int card, event_t event)
//----- (427470) --------------------------------------------------------
card_pandoras_box(int player, int card, event_t event)
//----- (427510) --------------------------------------------------------
pandoras_box_effect(int player)
//----- (427700) --------------------------------------------------------
card_sunglasses_of_urza(int player, int card, event_t event)
//----- (427740) --------------------------------------------------------
mana_producer_ai_reluctant_to_activate(int player, int card, event_t event, color_t color)
//----- (4277F0) --------------------------------------------------------
sub_4277F0(int a1, int a2, int a3)
//----- (427890) --------------------------------------------------------
free_and_rtrue(void *memory)
//----- (4278B0) --------------------------------------------------------
sub_4278B0(const MSG *lpMsg)
//----- (427A90) --------------------------------------------------------
sub_427A90(int a1, UINT uElapse)
//----- (427BF0) --------------------------------------------------------
register_window_classes()
//----- (427D70) --------------------------------------------------------
destroy_windowclasses()
//----- (427DD0) --------------------------------------------------------
setup_paths_and_load_text_etc(const char *str)
//----- (428160) --------------------------------------------------------
sub_428160()
//----- (4281A0) --------------------------------------------------------
card_alchors_tomb(int player, int card, event_t event)
//----- (428330) --------------------------------------------------------
C_card_barls_cage(int player, int card, event_t event)
//----- (428420) --------------------------------------------------------
card_bone_flute(int player, int card, event_t event)
//----- (428530) --------------------------------------------------------
helper_bone_flute(int player, int card, int t_player, int t_card, int csvid)
//----- (428580) --------------------------------------------------------
card_book_of_rass(int player, int card, event_t event)
//----- (428650) --------------------------------------------------------
C_card_fountain_of_youth(int player, int card, event_t event)
//----- (428770) --------------------------------------------------------
C_card_life_chisel(int player, int card, event_t event)
//----- (4288E0) --------------------------------------------------------
card_poison_snake(int player, int card, event_t event)
//----- (4289B0) --------------------------------------------------------
card_relic_barrier(int player, int card, event_t event)
//----- (428A90) --------------------------------------------------------
C_card_serpent_generator(int player, int card, event_t event)
//----- (428BC0) --------------------------------------------------------
card_war_barge(int player, int card, event_t event)
//----- (428D80) --------------------------------------------------------
war_barge_helper(int srcplayer, int srccard, int player, int card, int iid)
//----- (428DE0) --------------------------------------------------------
card_akron_legionnaire(int player, int card, event_t event)
//----- (428E40) --------------------------------------------------------
card_banshee(int player, int card, event_t event)
//----- (428F70) --------------------------------------------------------
card_beasts_of_bogardan(int player, int card, event_t event)
//----- (429040) --------------------------------------------------------
C_card_bog_rats(int player, int card, event_t event)
//----- (4290A0) --------------------------------------------------------
card_coal_golem(int player, int card, event_t event)
//----- (429140) --------------------------------------------------------
card_heavy_ballista(int player, int card, event_t event)
//----- (429200) --------------------------------------------------------
card_elder_spawn(int player, int card, event_t event)
//----- (429450) --------------------------------------------------------
card_elves_of_deep_shadow(int player, int card, event_t event)
//----- (4295B0) --------------------------------------------------------
card_emerald_dragonfly(int player, int card, event_t event)
//----- (429720) --------------------------------------------------------
card_exorcist(int player, int card, event_t event)
//----- (4298A0) --------------------------------------------------------
card_fallen_angel(int player, int card, event_t event)
//----- (429A70) --------------------------------------------------------
card_fire_drake(int player, int card, event_t event)
//----- (429BD0) --------------------------------------------------------
card_fire_sprites(int player, int card, event_t event)
//----- (429C60) --------------------------------------------------------
card_ghosts_of_the_damned(int player, int card, event_t event)
//----- (429DC0) --------------------------------------------------------
card_giant_turtle(int player, int card, event_t event)
//----- (429E40) --------------------------------------------------------
card_goblin_digging_team(int player, int card, event_t event)
//----- (429F10) --------------------------------------------------------
card_goblins_of_the_flarg(int player, int card, event_t event)
//----- (429F90) --------------------------------------------------------
C_card_hyperion_blacksmith(int player, int card, event_t event)
//----- (42A110) --------------------------------------------------------
C_card_ivory_guardians(int player, int card, event_t event)
//----- (42A230) --------------------------------------------------------
card_crimson_kobolds(int player, int card, event_t event)
//----- (42A280) --------------------------------------------------------
C_card_kobold_drill_sergeant(int player, int card, event_t event)
//----- (42A330) --------------------------------------------------------
card_kobold_overlord(int player, int card, event_t event)
//----- (42A3C0) --------------------------------------------------------
C_card_kobold_taskmaster(int player, int card, event_t event)
//----- (42A440) --------------------------------------------------------
C_card_merfolk_assassin(int player, int card, event_t event)
//----- (42A500) --------------------------------------------------------
card_miracle_worker(int player, int card, event_t event)
//----- (42A7B0) --------------------------------------------------------
card_mold_demon(int player, int card, event_t event)
//----- (42A910) --------------------------------------------------------
card_people_of_the_woods(int player, int card, event_t event)
//----- (42A980) --------------------------------------------------------
card_pixie_queen(int player, int card, event_t event)
//----- (42AAE0) --------------------------------------------------------
card_rabid_wombat(int player, int card, event_t event)
//----- (42ABA0) --------------------------------------------------------
card_savaen_elves(int player, int card, event_t event)
//----- (42AE40) --------------------------------------------------------
sub_42AE40()
//----- (42AEC0) --------------------------------------------------------
card_scavenger_folk(int player, int card, event_t event)
//----- (42AFC0) --------------------------------------------------------
card_spinal_villain(int player, int card, event_t event)
//----- (42B170) --------------------------------------------------------
card_drowned(int player, int card, event_t event)
//----- (42B1C0) --------------------------------------------------------
card_tracker(int player, int card, event_t event)
//----- (42B320) --------------------------------------------------------
card_wall_of_opposition(int player, int card, event_t event)
//----- (42B590) --------------------------------------------------------
card_wall_of_tombstones(int player, int card, event_t event)
//----- (42B670) --------------------------------------------------------
C_card_wall_of_wonder(int player, int card, event_t event)
//----- (42B880) --------------------------------------------------------
card_water_wurm(int player, int card, event_t event)
//----- (42B8D0) --------------------------------------------------------
card_witch_hunter(int player, int card, event_t event)
//----- (42BB80) --------------------------------------------------------
card_wormwood_treefolk(int player, int card, event_t event)
//----- (42BE60) --------------------------------------------------------
card_wall_of_light(int player, int card, event_t event)
//----- (42BED0) --------------------------------------------------------
card_mountain_yeti(int player, int card, event_t event)
//----- (42BF40) --------------------------------------------------------
card_angelic_voices(int player, int card, event_t event)
//----- (42BFF0) --------------------------------------------------------
card_blood_moon(int player, int card, event_t event)
//----- (42C100) --------------------------------------------------------
card_greater_realm_of_preservation(int player, int card, event_t event)
//----- (42C3B0) --------------------------------------------------------
C_card_lifebood(int player, int card, event_t event)
//----- (42C480) --------------------------------------------------------
card_hidden_path(int player, int card, event_t event)
//----- (42C530) --------------------------------------------------------
C_card_moat(int player, int card, event_t event)
//----- (42C570) --------------------------------------------------------
C_card_spiritual_sanctuary(int player, int card, event_t event)
//----- (42C6D0) --------------------------------------------------------
card_boomerang(int player, int card, event_t event)
//----- (42C770) --------------------------------------------------------
C_card_divine_offering(int player, int card, event_t event)
//----- (42C870) --------------------------------------------------------
card_great_defender(int player, int card, event_t event)
//----- (42C950) --------------------------------------------------------
card_hell_swarm(int player, int card, event_t event)
//----- (42CA10) --------------------------------------------------------
card_holy_light(int player, int card, event_t event)
//----- (42CB00) --------------------------------------------------------
card_riptide(int player, int card, event_t event)
//----- (42CBB0) --------------------------------------------------------
card_storm_seeker(int player, int card, event_t event)
//----- (42CCF0) --------------------------------------------------------
C_card_force_spike(int player, int card, event_t event)
//----- (42CEF0) --------------------------------------------------------
card_remove_soul(int player, int card, event_t event)
//----- (42D0A0) --------------------------------------------------------
C_card_reset(int player, int card, event_t event)
//----- (42D130) --------------------------------------------------------
card_acid_rain(int player, int card, event_t event)
//----- (42D190) --------------------------------------------------------
card_amnesia(int player, int card, event_t event)
//----- (42D3A0) --------------------------------------------------------
card_cleanse(int player, int card, event_t event)
//----- (42D450) --------------------------------------------------------
card_eternal_flame(int player, int card, event_t event)
//----- (42D530) --------------------------------------------------------
card_inquisition(int player, int card, event_t event)
//----- (42D750) --------------------------------------------------------
card_jovial_evil(int player, int card, event_t event)
//----- (42D8D0) --------------------------------------------------------
card_martyrs_cry(int player, int card, event_t event)
//----- (42D9E0) --------------------------------------------------------
C_card_syphon_soul(int player, int card, event_t event)
//----- (42DAF0) --------------------------------------------------------
card_typhoon(int player, int card, event_t event)
//----- (42DB70) --------------------------------------------------------
fx_swap_power_toughness_903(int player, int card, event_t event)
//----- (42DBF0) --------------------------------------------------------
helper_destroy_basiclandtype(int player, int card, int a5, int a6, int a7)
//----- (42DC40) --------------------------------------------------------
controls_an_enchanted_creature(int player)
//----- (42DCD0) --------------------------------------------------------
TENTATIVE_sacrifice_basic_land_type(int preferred_controller, int a4, signed int a5, int a6, int allow_cancel)
//----- (42DE60) --------------------------------------------------------
charge_mana(int player, color_t color, int amount)
//----- (42EC40) --------------------------------------------------------
sub_42EC40(int a6, int *a8, int *a9, int a4)
//----- (42EDC0) --------------------------------------------------------
sub_42EDC0(int a1, int *a2, int *a3, int a4, int *a5, int a6)
//----- (42EF80) --------------------------------------------------------
sub_42EF80(int a1, int *a2, int *a3, int a4, int *a5, int a6)
//----- (42F130) --------------------------------------------------------
pay_mana_maximally_satisfied(int *pay_mana, int x_val, int max_x_val)
//----- (42F1A0) --------------------------------------------------------
calculate_needed_mana_colors(int *a1)
//----- (42F1F0) --------------------------------------------------------
C_format_manacost_into_global_allpurpose_buffer(int ignored, int *mana_cost_array, int x_so_far, int max_x)
//----- (42F4A0) --------------------------------------------------------
try_to_pay_for_mana_by_autotapping(int player, int *amt, int *a4, autotap_t disallowed_autotap_flags, int always_one)
//----- (42FDA0) --------------------------------------------------------
can_autotap(int player, int card, autotap_t disallowed_autotap_flags)
//----- (42FED0) --------------------------------------------------------
autotap_mana_source(int player, int card)
//----- (430060) --------------------------------------------------------
sub_430060(int *a1, int a2, int a3, int *a4, int a5, int a6, int a7, int *a8, int *a9)
//----- (4300D0) --------------------------------------------------------
sub_4300D0(int *pay_mana_arr, int a2, int *mana_pool_arr, int a4, int ldblclicked, int max_x_val, int x_val)
//----- (430150) --------------------------------------------------------
sub_430150()
//----- (4301A0) --------------------------------------------------------
sub_4301A0(int a1, int a2)
//----- (4301E0) --------------------------------------------------------
sub_4301E0()
//----- (430200) --------------------------------------------------------
sub_430200(int a1)
//----- (430260) --------------------------------------------------------
TENTATIVE_update_mana_spent(int *a1)
//----- (4302C0) --------------------------------------------------------
charge_mana_w_global_cost_mod(int player, int card, color_t color, int amount)
//----- (430330) --------------------------------------------------------
card_artifact_possession(int player, int card, event_t event)
//----- (430480) --------------------------------------------------------
card_artifact_ward(int player, int card, event_t event)
//----- (4305E0) --------------------------------------------------------
card_consecrate_land(int player, int card, event_t event)
//----- (430870) --------------------------------------------------------
UNKNOWN_consecrate_land_helper(int player, int card, int iid)
//----- (4308F0) --------------------------------------------------------
C_card_cyclone(int player, int card, event_t event)
//----- (430C00) --------------------------------------------------------
should_ai_let_cyclone_be_sacced(int num_counters)
//----- (430D00) --------------------------------------------------------
card_damping_field(int player, int card, event_t event)
//----- (430FA0) --------------------------------------------------------
C_card_drop_of_honey(int player, int card, event_t event)
//----- (431380) --------------------------------------------------------
card_earthbind(int player, int card, event_t event)
//----- (4314A0) --------------------------------------------------------
card_farmstead(int player, int card, event_t event)
//----- (4316C0) --------------------------------------------------------
card_fastbond(int player, int card, event_t event)
//----- (4317B0) --------------------------------------------------------
card_fishliver_oil(int player, int card, event_t event)
//----- (431890) --------------------------------------------------------
card_gate_to_phyrexia(int player, int card, event_t event)
//----- (431BC0) --------------------------------------------------------
card_haunting_wind(int player, int card, event_t event)
//----- (431C40) --------------------------------------------------------
C_card_invisibility(int player, int card, event_t event)
//----- (431D40) --------------------------------------------------------
card_jihad(int player, int card, event_t event)
//----- (431FE0) --------------------------------------------------------
card_kudzu(int player, int card, event_t event)
//----- (4322B0) --------------------------------------------------------
card_lance(int player, int card, event_t event)
//----- (432300) --------------------------------------------------------
card_lich(int player, int card, event_t event)
//----- (432530) --------------------------------------------------------
sub_432530(int preferred_controller, int a4)
//----- (4326C0) --------------------------------------------------------
card_oubliette(int player, int card, event_t event)
//----- (4327D0) --------------------------------------------------------
card_powerleech(int player, int card, event_t event)
//----- (4328B0) --------------------------------------------------------
card_raging_river(int player, int card, event_t event)
//----- (432C80) --------------------------------------------------------
raging_river_helper(signed int result, int player, int a3)
//----- (432D60) --------------------------------------------------------
register_PlayerDirectiveClass(const CHAR *a1)
//----- (432DE0) --------------------------------------------------------
wndproc_PlayerDirectiveClass(HWND hwnd, WM_t msg, WPARAM wparam, LPARAM lparam)
//----- (432F80) --------------------------------------------------------
helper_for_startduel()
//----- (433190) --------------------------------------------------------
C_draw_a_card(int player)
//----- (433540) --------------------------------------------------------
discard(int player, int random, int shandalar_unused)
//----- (4337A0) --------------------------------------------------------
discard_card(int player, int card)
//----- (433850) --------------------------------------------------------
put_card_on_stack2(int player, int card)
//----- (4338F0) --------------------------------------------------------
put_card_on_stack(int player, int card, int second_call)
//----- (433E10) --------------------------------------------------------
C_put_card_on_stack3(int player, int card)
//----- (434040) --------------------------------------------------------
activate(int a1, int player, int card)
//----- (4346E0) --------------------------------------------------------
C_finalize_activation(int player, int card)
//----- (434800) --------------------------------------------------------
C_resolve_trigger(int player, int card, int a5)
//----- (434930) --------------------------------------------------------
in_hand(int player, int card)
//----- (434960) --------------------------------------------------------
C_human_choose_blockers(int player)
//----- (434BE0) --------------------------------------------------------
any_can_attack(int player)
//----- (434C30) --------------------------------------------------------
C_can_attack(int player, int card)
//----- (434DB0) --------------------------------------------------------
has_vigilance(int player, int card)
//----- (434DE0) --------------------------------------------------------
sub_434DE0(signed int player)
//----- (434E70) --------------------------------------------------------
any_can_block(int blocking_player)
//----- (434F30) --------------------------------------------------------
C_is_legal_block(int blocking_player, int blocking_card, int blocked_player, int blocked_card)
//----- (434F90) --------------------------------------------------------
C_is_legal_block_impl(int blocking_player, int blocking_card, int blocked_player, int blocked_card, keyword_t abils, int landtypes_controlled)
//----- (4350E0) --------------------------------------------------------
try_block(int blocking_player, int blocking_card, int blocked_player, int blocked_card)
//----- (435140) --------------------------------------------------------
TENTATIVE_backup_data_for_ai_frontend_0(int a1, int a2)
//----- (4351C0) --------------------------------------------------------
C_recalculate_all_cards_in_play()
//----- (4352D0) --------------------------------------------------------
C_get_abilities(int player, int card, event_t event, int new_attacking_card)
//----- (4358F0) --------------------------------------------------------
single_color_test_bit_to_color_t(color_test_t bit)
//----- (435940) --------------------------------------------------------
sub_435940(int player)
//----- (4359B0) --------------------------------------------------------
dispatch_event(int player, int card, event_t event)
//----- (435A40) --------------------------------------------------------
C_dispatch_event_raw(event_t event)
//----- (435B50) --------------------------------------------------------
dispatch_event_to_single_card(int player, int card, event_t event, int new_attacking_card_controller, int new_attacking_card)
//----- (435C30) --------------------------------------------------------
is_mana_source_but_not_act_ability(int player, int card)
//----- (435C80) --------------------------------------------------------
push_affected_card_stack()
//----- (435CD0) --------------------------------------------------------
pop_affected_card_stack()
//----- (435D20) --------------------------------------------------------
sub_435D20()
//----- (435F20) --------------------------------------------------------
C_phase_changed(int player, int new_phase)
//----- (435F50) --------------------------------------------------------
play_sound_effect(wav_t sound)
//----- (436380) --------------------------------------------------------
call_UnloadAllSnds_then_LoadSnd_for_first_20_wav_filenames()
//----- (436410) --------------------------------------------------------
clear_stack()
//----- (436430) --------------------------------------------------------
get_top_card_on_stack_data()
//----- (436450) --------------------------------------------------------
C_recopy_card_onto_stack(int a1)
//----- (436550) --------------------------------------------------------
C_put_card_or_activation_onto_stack(int player, int card, event_t event, int a4, int a5)
//----- (436700) --------------------------------------------------------
set_stack_damage_targets()
//----- (436740) --------------------------------------------------------
resolve_top_card_on_stack()
//----- (436930) --------------------------------------------------------
obliterate_top_card_of_stack()
//----- (436980) --------------------------------------------------------
sprintf_CastingActivatingOrProcessing_CardName(char *deststr, event_t event, int player, int card)
//----- (436A00) --------------------------------------------------------
sub_436A00()
//----- (436A20) --------------------------------------------------------
allow_response(int a1, int phase, const char *a3, int event_or_trigger_or_phase)
//----- (436B00) --------------------------------------------------------
sub_436B00(signed int a1, event_t event, const char *srcstr, int a4)
//----- (436FD0) --------------------------------------------------------
sub_436FD0(int a1)
//----- (4371A0) --------------------------------------------------------
dispatch_trigger_twice_once_with_each_player_as_reason(int reason_for_trig, trigger_t trig, const char *prompt, int a4)
//----- (4371E0) --------------------------------------------------------
C_dispatch_trigger(int player, trigger_t trig, const char *prompt, int a4)
//----- (437390) --------------------------------------------------------
clear_timestamps()
//----- (4373B0) --------------------------------------------------------
TENTATIVE_set_timestamps(int player, int card)
//----- (437400) --------------------------------------------------------
sub_437400()
//----- (4374B0) --------------------------------------------------------
check_untap_payment(int player, int card)
//----- (437510) --------------------------------------------------------
set_state_isblocked(int player)
//----- (4375D0) --------------------------------------------------------
is_ea_act_use_x(int player, int card)
//----- (437620) --------------------------------------------------------
set_all_upkeep_flags_to_0()
//----- (437670) --------------------------------------------------------
setup_upkeep_costs_and_set_untap_cost()
//----- (437700) --------------------------------------------------------
C_are_enemies_against_color(int player, int card, int player2, int card2)
//----- (4377B0) --------------------------------------------------------
is_in_play(int player, int card)
//----- (4377E0) --------------------------------------------------------
TENTATIVE_update_display_for_changed_phase(int player, int new_phase)
//----- (4378E0) --------------------------------------------------------
sub_4378E0(int a3, int a4, const char *srcstr, int a6, int a7, int a8, int a9, int a10, int *a11, target_t *a12, int a13, int a14)
//----- (437E20) --------------------------------------------------------
sub_437E20()
//----- (437E40) --------------------------------------------------------
sub_437E40(int a1, LPARAM a2, int a3)
//----- (437EC0) --------------------------------------------------------
TENTATIVE_redisplay_all(int a1, int a2)
//----- (4386B0) --------------------------------------------------------
start_timer_for_ai_speculation_0()
//----- (4386D0) --------------------------------------------------------
check_timer_for_ai_speculation_0()
//----- (4386F0) --------------------------------------------------------
set_centerwindow_txt(const char *a1)
//----- (438720) --------------------------------------------------------
sub_438720(const CHAR *a1)
//----- (4388D0) --------------------------------------------------------
copy_mana_pool_to_display()
//----- (438940) --------------------------------------------------------
WILDGUESS_shuffle_animation(int player)
//----- (438980) --------------------------------------------------------
player_gt_1_or_card_ge_150(unsigned int player, unsigned int card)
//----- (4389A0) --------------------------------------------------------
get_displayed_info_slot(unsigned int player, unsigned int card)
//----- (4389D0) --------------------------------------------------------
get_displayed_special_counters(unsigned int player, unsigned int card)
//----- (438A00) --------------------------------------------------------
get_displayed_standard_counters_counts(int player, int card, int *special_counters_byte1, int *special_counters_byte2, int *special_counters_byte3, int *counters, int *unknown0x121)
//----- (438A70) --------------------------------------------------------
get_displayed_blocking(int player, int card)
//----- (438AA0) --------------------------------------------------------
get_displayed_damage_on_card(unsigned int player, unsigned int card)
//----- (438AD0) --------------------------------------------------------
get_displayed_internal_card_id(unsigned int player, unsigned int card)
//----- (438B00) --------------------------------------------------------
get_displayed_csvid(int player, int card)
//----- (438B30) --------------------------------------------------------
get_displayed_state_code_1inplay_2onstack_else0(unsigned int player, unsigned int card)
//----- (438B80) --------------------------------------------------------
return_displayed_bitfield_of_1summonsick__2tapped__4attacking__8blocking__10damage_target_player_and_damage_target_card__20oublietted(unsigned int player, unsigned int card)
//----- (438C40) --------------------------------------------------------
get_displayed_damage_target_player_card(target_t *ret_tgt, unsigned int player, unsigned int card)
//----- (438C90) --------------------------------------------------------
get_displayed_pic_num_and_pic_csv_id_of_damage_source_playercard(target_t *ret_source, unsigned int player, unsigned int card)
//----- (438CE0) --------------------------------------------------------
get_displayed_type(int player, int card)
//----- (438D10) --------------------------------------------------------
get_displayed_power(unsigned int player, unsigned int card)
//----- (438D40) --------------------------------------------------------
get_displayed_toughness(unsigned int player, unsigned int card)
//----- (438D70) --------------------------------------------------------
get_displayed_abilities(unsigned int player, unsigned int card)
//----- (438DC0) --------------------------------------------------------
get_displayed_color(unsigned int player, unsigned int card)
//----- (438DF0) --------------------------------------------------------
get_displayed_mana_color(unsigned int player, unsigned int card)
//----- (438E20) --------------------------------------------------------
get_displayed_state(unsigned int player, unsigned int card)
//----- (438E50) --------------------------------------------------------
get_displayed_eot_toughness(unsigned int player, unsigned int card)
//----- (438E80) --------------------------------------------------------
get_displayed_counter_power_and_counter_toughness(int player, int card, int *counter_power, int *counter_toughness)
//----- (438EC0) --------------------------------------------------------
get_displayed_owned_by_opponent(int player, int card)
//----- (438EF0) --------------------------------------------------------
get_displayed_ea_manasource(int player, int card)
//----- (438F20) --------------------------------------------------------
get_displayed_sleighted_or_hacked(int player, int card)
//----- (438F60) --------------------------------------------------------
get_displayed_unknown0x70(unsigned int player, unsigned int card)
//----- (438F90) --------------------------------------------------------
get_displayed_is_targeted(unsigned int player, unsigned int card)
//----- (438FD0) --------------------------------------------------------
get_displayed_not_cannot_target(unsigned int player, unsigned int card)
//----- (439010) --------------------------------------------------------
TENTATIVE_replace_sleight_symbols(int player, int card, char *a3)
//----- (439070) --------------------------------------------------------
TENTATIVE_replace_hack_symbols(int player, int card, char *a3)
//----- (4390D0) --------------------------------------------------------
get_displayed_token_status(unsigned int player, unsigned int card)
//----- (439100) --------------------------------------------------------
get_displayed_untap_status(unsigned int player, unsigned int card)
//----- (439130) --------------------------------------------------------
get_displayed_original_internal_card_id(unsigned int player, unsigned int card)
//----- (439160) --------------------------------------------------------
get_displayed_parent_controller_and_parent_card(target_t *ret_tgt, unsigned int player, unsigned int card)
//----- (4391C0) --------------------------------------------------------
get_displayed_kill_code(unsigned int player, unsigned int card)
//----- (4391F0) --------------------------------------------------------
sub_4391F0(char *a1)
//----- (439220) --------------------------------------------------------
get_displayed_life(unsigned int a1)
//----- (439250) --------------------------------------------------------
get_displayed_poison_counters(int player)
//----- (439280) --------------------------------------------------------
get_displayed_liched(unsigned int player)
//----- (4392B0) --------------------------------------------------------
memcpy_displayed_mana_pool(void *Dst, unsigned int player)
//----- (439300) --------------------------------------------------------
memcpy_displayed_card_instance_t(void *Dst, unsigned int player, unsigned int card)
//----- (439360) --------------------------------------------------------
memcpy_displayed_graveyard_csvids(void *Dst, unsigned int a2)
//----- (4393D0) --------------------------------------------------------
memcpy_displayed_exile_csvids(void *Dst, unsigned int player)
//----- (439440) --------------------------------------------------------
memcpy_displayed_library_csvids(void *Dst, unsigned int player)
//----- (4394B0) --------------------------------------------------------
memcpy_displayed_stack(backup_stack_t *Dst)
//----- (4394F0) --------------------------------------------------------
memcpy_displayed_ante_counts_and_cards(int *ret_ante_cards_1, int *ret_ante_count_1, int *ret_ante_cards_0, int *ret_ante_count_0)
//----- (439590) --------------------------------------------------------
sub_439590(int a1, int a2)
//----- (4395C0) --------------------------------------------------------
memcpy_displayed_TENTATIVE_option_PhaseStoppers(void *Dst, unsigned int a2)
//----- (439610) --------------------------------------------------------
get_displayed_time_walk_flag(int *ret_displayed_time_walk_flag)
//----- (439620) --------------------------------------------------------
sub_439620()
//----- (439630) --------------------------------------------------------
memcpy_displayed_active_cards_counts(int *a1)
//----- (439650) --------------------------------------------------------
sub_439650()
//----- (439660) --------------------------------------------------------
sub_439660(int a1, int a2)
//----- (439690) --------------------------------------------------------
get_displayable_cardname_from_player_card(int player, int card)
//----- (439890) --------------------------------------------------------
append_displayable_cardname_to_global_all_purpose_buffer(int player, int card)
//----- (4398C0) --------------------------------------------------------
switch_phase(int a3)
//----- (439A80) --------------------------------------------------------
sub_439A80()
//----- (439B10) --------------------------------------------------------
sub_439B10(int a1, signed int phase)
//----- (439C20) --------------------------------------------------------
UNKNOWN_something_todo_with_phase_stoppers(int a1)
//----- (439C90) --------------------------------------------------------
TENTATIVE_check_for_stops(phase_t a3)
//----- (439F80) --------------------------------------------------------
sub_439F80(int a3, const char *srcstr)
//----- (43A060) --------------------------------------------------------
C_mana_burn()
//----- (43A1B0) --------------------------------------------------------
is_anyone_dead()
//----- (43A240) --------------------------------------------------------
sub_43A240(int a1, signed int a2, int a3, int a4)
//----- (43A280) --------------------------------------------------------
C_is_a_tappable_mana_source(signed int player, int card)
//----- (43A2D0) --------------------------------------------------------
C_tap_card_for_mana(int player, int card)
//----- (43A370) --------------------------------------------------------
init_turn(int a1)
//----- (43A4D0) --------------------------------------------------------
sub_43A4D0()
//----- (43A600) --------------------------------------------------------
TENTATIVE_start_turn(int player)
//----- (43A700) --------------------------------------------------------
untap_phase_exe(int player)
//----- (43ACF0) --------------------------------------------------------
C_upkeep_phase(int a3)
//----- (43AEC0) --------------------------------------------------------
draw_phase(int player)
//----- (43B110) --------------------------------------------------------
main_phase(int player, int a4, int a5)
//----- (43CEB0) --------------------------------------------------------
discard_phase(int a1, int a2)
//----- (43D120) --------------------------------------------------------
cleanup_phase(signed int player)
//----- (43D2C0) --------------------------------------------------------
end_turn_phase(int player)
//----- (43D590) --------------------------------------------------------
C_ai_decision_phase(int a1, int *a2, int *a3, int *a4)
//----- (43D9B0) --------------------------------------------------------
register_GraveyardClass(const CHAR *a1)
//----- (43DBC0) --------------------------------------------------------
destroy_GraveyardClass()
//----- (43DBE0) --------------------------------------------------------
wndproc_GraveyardClass(HWND hWndParent, int msg, int wparam, LPARAM lParam)
//----- (43E4E0) --------------------------------------------------------
wndproc_ExpandedGraveyard(HWND hWndParent, UINT Msg, WPARAM wParam, LPARAM lParam)
//----- (43E620) --------------------------------------------------------
wndproc_GraveyardCards(HWND hWndParent, UINT Msg, WPARAM wParam, LPARAM lParam)
//----- (43E860) --------------------------------------------------------
sub_43E860(HWND hWnd, int a2)
//----- (43EBE0) --------------------------------------------------------
sub_43EBE0(HWND hWnd)
//----- (43EBF0) --------------------------------------------------------
sub_43EBF0(unsigned int a1)
//----- (43EC30) --------------------------------------------------------
sub_43EC30()
//----- (43EC50) --------------------------------------------------------
TENTATIVE_dlgproc_show_ante(HWND hwnd, WM_t msg, unsigned int wparam, __int32 lparam)
//----- (43F560) --------------------------------------------------------
sub_43F560(LPRECT lprc, HWND hWnd, int a3, int a4)
//----- (43F720) --------------------------------------------------------
sub_43F720()
//----- (43F750) --------------------------------------------------------
sub_43F750(const char **a1)
//----- (43F800) --------------------------------------------------------
sub_43F800(int a1, signed int a2, const void *a3, int a4)
//----- (440520) --------------------------------------------------------
sub_440520(int a1, signed int *a2, int a3)
//----- (440B00) --------------------------------------------------------
TENTATIVE_playmovie(HWND hWndParent, int a2, int a3)
//----- (440C30) --------------------------------------------------------
createdialog_MultiDuelPage(LPARAM dwInitParam)
//----- (440C60) --------------------------------------------------------
sub_440C60()
//----- (440C90) --------------------------------------------------------
createdialog_SingleDuelPage(LPARAM dwInitParam)
//----- (440CC0) --------------------------------------------------------
sub_440CC0()
//----- (440CF0) --------------------------------------------------------
createdialog_GauntletPage(LPARAM dwInitParam)
//----- (440D20) --------------------------------------------------------
sub_440D20()
//----- (440D50) --------------------------------------------------------
createdialog_SealedDeckPage(LPARAM dwInitParam)
//----- (440D80) --------------------------------------------------------
sub_440D80()
//----- (440DB0) --------------------------------------------------------
sub_440DB0()
//----- (440E20) --------------------------------------------------------
sub_440E20()
//----- (440F00) --------------------------------------------------------
sub_440F00()
//----- (440F40) --------------------------------------------------------
sub_440F40()
//----- (440FD0) --------------------------------------------------------
sub_440FD0(int a1)
//----- (4410C0) --------------------------------------------------------
sub_4410C0()
//----- (4412B0) --------------------------------------------------------
sub_4412B0()
//----- (4413C0) --------------------------------------------------------
sub_4413C0(int a1)
//----- (4414E0) --------------------------------------------------------
sub_4414E0(HWND hDlg)
//----- (441870) --------------------------------------------------------
sub_441870(HWND hDlg, int a2)
//----- (441B60) --------------------------------------------------------
sub_441B60(HWND hDlg, int a2)
//----- (442500) --------------------------------------------------------
dlgproc_MultiDuelPage(HWND hDlg, int a2, int a3, HWND a4)
//----- (444910) --------------------------------------------------------
sub_444910()
//----- (4449F0) --------------------------------------------------------
sub_4449F0()
//----- (444A50) --------------------------------------------------------
sub_444A50()
//----- (444AB0) --------------------------------------------------------
sub_444AB0()
//----- (444B20) --------------------------------------------------------
sub_444B20(const char *Filename)
//----- (444C00) --------------------------------------------------------
sub_444C00(HWND hDlg)
//----- (4456B0) --------------------------------------------------------
dlgproc_SingleDuelPage(HWND hwnd, WM_t msg, int wparam, int lparam)
//----- (447470) --------------------------------------------------------
sub_447470(HWND hDlg)
//----- (447F70) --------------------------------------------------------
dlgproc_GauntletPage(HWND hwnd, WM_t msg, int wparam, int lparam)
//----- (449960) --------------------------------------------------------
sub_449960(HWND hDlg)
//----- (44A1F0) --------------------------------------------------------
sub_44A1F0()
//----- (44A210) --------------------------------------------------------
sub_44A210(char *Dest, int a2)
//----- (44A2A0) --------------------------------------------------------
dlgproc_SealedDeckPage(HWND hwnd, WM_t msg, int wparam, int lparam)
//----- (44C5B0) --------------------------------------------------------
sub_44C5B0(HWND hDlg)
//----- (44D100) --------------------------------------------------------
register_FaceClass(const CHAR *a1)
//----- (44D380) --------------------------------------------------------
destroy_FaceClass()
//----- (44D410) --------------------------------------------------------
wndproc_FaceClass(HWND hwnd, WM_t msg, WPARAM wparam, LPARAM lparam)
//----- (44DA00) --------------------------------------------------------
sub_44DA00(HDC hdc, const RECT *lprc, int a3)
//----- (44DCE0) --------------------------------------------------------
sub_44DCE0(int a1, int a2)
//----- (44DDF0) --------------------------------------------------------
sub_44DDF0(int a1)
//----- (44DE30) --------------------------------------------------------
sub_44DE30(int a1)
//----- (44DE60) --------------------------------------------------------
sub_44DE60(const char *Filename, int player, int always_one, int always_negone)
//----- (44DED0) --------------------------------------------------------
sub_44DED0(int a1)
//----- (44DFE0) --------------------------------------------------------
sub_44DFE0()
//----- (44E010) --------------------------------------------------------
sub_44E010()
//----- (44E050) --------------------------------------------------------
internal_rand(signed int a1)
//----- (44E070) --------------------------------------------------------
sub_44E070(int a1, signed int a2)
//----- (44E0D0) --------------------------------------------------------
sub_44E0D0(int a1)
//----- (44E140) --------------------------------------------------------
card_ashnods_altar(int player, int card, event_t event)
//----- (44E240) --------------------------------------------------------
C_card_ashnods_transmogrant(int player, int card, event_t event)
//----- (44E400) --------------------------------------------------------
C_card_basalt_monolith(int player, int card, event_t event)
//----- (44E660) --------------------------------------------------------
card_copper_tablet(int player, int card, event_t event)
//----- (44E760) --------------------------------------------------------
card_cyclopean_tomb(int player, int card, event_t event)
//----- (44EBE0) --------------------------------------------------------
sub_44EBE0(int player, int card, signed int result)
//----- (44ED10) --------------------------------------------------------
card_gauntlet_of_might(int player, int card, event_t event)
//----- (44EE60) --------------------------------------------------------
card_icy_manipulator(int player, int card, event_t event)
//----- (44EFD0) --------------------------------------------------------
C_card_jade_statue(int player, int card, event_t event)
//----- (44F2A0) --------------------------------------------------------
card_jalum_tome(int player, int card, event_t event)
//----- (44F3C0) --------------------------------------------------------
card_jandors_ring(int player, int card, event_t event)
//----- (44F560) --------------------------------------------------------
C_card_jeweled_bird(int player, int card, event_t event)
//----- (44F670) --------------------------------------------------------
card_living_wall(int player, int card, event_t event)
//----- (44F6B0) --------------------------------------------------------
card_mana_crypt(int player, int card, event_t event)
//----- (44F960) --------------------------------------------------------
card_mightstone(int player, int card, event_t event)
//----- (44F9F0) --------------------------------------------------------
card_obelisk_of_undoing(int player, int card, event_t event)
//----- (44FAF0) --------------------------------------------------------
card_pyramids(int player, int card, event_t event)
//----- (44FC70) --------------------------------------------------------
card_rakalite(int player, int card, event_t event)
//----- (44FE80) --------------------------------------------------------
card_ring_of_maruf(int player, int card, event_t event)
//----- (4500F0) --------------------------------------------------------
card_rocket_launcher(int player, int card, event_t event)
//----- (450260) --------------------------------------------------------
card_sandals_of_abdallah(int player, int card, event_t event)
//----- (4503F0) --------------------------------------------------------
card_staff_of_zegon(int player, int card, event_t event)
//----- (450530) --------------------------------------------------------
card_su_chi(int player, int card, event_t event)
//----- (450580) --------------------------------------------------------
C_card_tablet_of_epityr(int player, int card, event_t event)
//----- (450720) --------------------------------------------------------
card_tawnoss_coffin(int player, int card, event_t event)
//----- (450B10) --------------------------------------------------------
C_card_urzas_chalice(int player, int card, event_t event)
//----- (450C90) --------------------------------------------------------
card_urzas_miter(int player, int card, event_t event)
//----- (450E40) --------------------------------------------------------
card_weakstone(int player, int card, event_t event)
//----- (450E90) --------------------------------------------------------
C_card_abu_jafar(int player, int card, event_t event)
//----- (450F90) --------------------------------------------------------
card_aladdin(int player, int card, event_t event)
//----- (451340) --------------------------------------------------------
card_argivian_archaeologist(int player, int card, event_t event)
//----- (451550) --------------------------------------------------------
card_argivian_blacksmith(int player, int card, event_t event)
//----- (4518A0) --------------------------------------------------------
card_argothian_pixies(int player, int card, event_t event)
//----- (451910) --------------------------------------------------------
card_argothian_treefolk(int player, int card, event_t event)
//----- (451970) --------------------------------------------------------
card_atog(int player, int card, event_t event)
//----- (451B30) --------------------------------------------------------
card_camel(int player, int card, event_t event)
//----- (451C40) --------------------------------------------------------
C_card_citanul_druid(int player, int card, event_t event)
//----- (451CF0) --------------------------------------------------------
C_card_clone(int player, int card, event_t event)
//----- (4520D0) --------------------------------------------------------
card_cuombajj_witches(int player, int card, event_t event)
//----- (4522B0) --------------------------------------------------------
helper_cuombajj_witches(int a1, int a2, signed int player, target_t *a4)
//----- (452400) --------------------------------------------------------
sub_452400(int a1<eax>)
//----- (452610) --------------------------------------------------------
card_desert_nomads(int player, int card, event_t event)
//----- (452710) --------------------------------------------------------
card_dwarven_demolition_team(int player, int card, event_t event)
//----- (4527E0) --------------------------------------------------------
C_card_dwarven_weaponsmith(int player, int card, event_t event)
//----- (452950) --------------------------------------------------------
add_1_1_counters(int howmany<eax>, card_instance_t *instance<esi>)
//----- (452970) --------------------------------------------------------
add_p1p1_counter(card_instance_t *instance<esi>)
//----- (452990) --------------------------------------------------------
C_card_erhnam_djinn(int player, int card, event_t event)
//----- (452BC0) --------------------------------------------------------
C_erhnam_djinn_ai_pick_target(int player, int card, target_t *tgt)
//----- (452CD0) --------------------------------------------------------
card_gaeas_avenger(int player, int card, event_t event)
//----- (452D40) --------------------------------------------------------
card_ghazban_ogre(int player, int card, event_t event)
//----- (452F00) --------------------------------------------------------
card_giant_badger(int player, int card, event_t event)
//----- (452F90) --------------------------------------------------------
card_goblin_artisans(int player, int card, event_t event)
//----- (4531B0) --------------------------------------------------------
card_granite_gargoyle(int player, int card, event_t event)
//----- (453420) --------------------------------------------------------
card_guardian_beast(int player, int card, event_t event)
//----- (4536A0) --------------------------------------------------------
UNKNOWN_guardian_beast_helper(int a3, int a4, int player, int card, int a7)
//----- (453700) --------------------------------------------------------
card_hasran_ogress(int player, int card, event_t event)
//----- (453880) --------------------------------------------------------
card_ifh_biff_efreet(int player, int card, event_t event)
//----- (453BF0) --------------------------------------------------------
UNKNOWN_ifh_biff_efreet_helper(int a1, int a2, int player, int card, int a5)
//----- (453C40) --------------------------------------------------------
C_card_juggernaut(int player, int card, event_t event)
//----- (453CE0) --------------------------------------------------------
card_khabal_ghoul(int player, int card, event_t event)
//----- (453D80) --------------------------------------------------------
card_king_suleiman(int player, int card, event_t event)
//----- (453E50) --------------------------------------------------------
C_card_kird_ape(int player, int card, event_t event)
//----- (453EB0) --------------------------------------------------------
C_card_martyrs_of_korlis(int player, int card, event_t event)
//----- (453FD0) --------------------------------------------------------
C_card_merchant_ship(int player, int card, event_t event)
//----- (454080) --------------------------------------------------------
card_mijae_djinn(int player, int card, event_t event)
//----- (454170) --------------------------------------------------------
C_card_nettling_imp(int player, int card, event_t event)
//----- (454270) --------------------------------------------------------
card_old_man_of_the_sea(int player, int card, event_t event)
//----- (454890) --------------------------------------------------------
card_orcish_mechanics(int player, int card, event_t event)
//----- (4549D0) --------------------------------------------------------
C_card_phyrexian_gremlins(int player, int card, event_t event)
//----- (454D80) --------------------------------------------------------
card_priest_of_yawgmoth(int player, int card, event_t event)
//----- (454ED0) --------------------------------------------------------
C_card_rock_hydra(int player, int card, event_t event)
//----- (455300) --------------------------------------------------------
card_rukh_egg(int player, int card, event_t event)
//----- (455360) --------------------------------------------------------
card_sage_of_lat_nam(int player, int card, event_t event)
//----- (4553E0) --------------------------------------------------------
card_sedge_troll(int player, int card, event_t event)
//----- (455490) --------------------------------------------------------
card_serendib_djinn(int player, int card, event_t event)
//----- (4557E0) --------------------------------------------------------
card_juzam_djinn(int player, int card, event_t event)
//----- (4558E0) --------------------------------------------------------
card_singing_tree(int player, int card, event_t event)
//----- (4559F0) --------------------------------------------------------
C_card_two_headed_giant_of_foriys(int player, int card, event_t event)
//----- (455BE0) --------------------------------------------------------
C_card_vesuvan_doppelganger(int player, int card, event_t event)
//----- (4565E0) --------------------------------------------------------
C_clone_and_vesuvan_doppelganger_helper(int a1, int a2, int player, int card, int a5)
//----- (456640) --------------------------------------------------------
C_vesuvan_doppelganger_helper(int a1, int a2, int player, int card, int iid)
//----- (456690) --------------------------------------------------------
card_veteran_bodyguard(int player, int card, event_t event)
//----- (4567F0) --------------------------------------------------------
UNKNOWN_martyrs_of_korlis_and_veteran_bodyguard_helper(int a1, int a2, int player, int card, int a5)
//----- (4568C0) --------------------------------------------------------
UNKNOWN_martyrs_of_korlis_and_veteran_bodyguard_helper2(int a1, int a2, int player, int card, int a5)
//----- (456910) --------------------------------------------------------
UNKNOWN_martyrs_of_korlis_and_veteran_bodyguard_helper3(int a1, int a2, int a3, int a4, int a5)
//----- (456950) --------------------------------------------------------
card_wyluli_wolf(int player, int card, event_t event)
//----- (456A60) --------------------------------------------------------
card_yawgmoth_demon(int player, int card, event_t event)
//----- (456BF0) --------------------------------------------------------
card_ydwen_efreet(int player, int card, event_t event)
//----- (456D30) --------------------------------------------------------
give_2_0_if_attacking(int player, int card, int a3, int a4)
//----- (456D80) --------------------------------------------------------
fx_guardian_beast_903(int player, int card, event_t event)
//----- (456EC0) --------------------------------------------------------
fx_blaze_of_glory_903(int player, int card, event_t event)
//----- (457310) --------------------------------------------------------
fx_control_903(int player, int card, event_t event)
//----- (457470) --------------------------------------------------------
fx_cyclopean_tomb_903(int player, int card, event_t event)
//----- (457720) --------------------------------------------------------
fx_desert_903(int player, int card, event_t event)
//----- (4577B0) --------------------------------------------------------
C_fx_erhnam_djinn_903(int player, int card, event_t event)
//----- (457860) --------------------------------------------------------
fx_guardian_angel_903(int player, int card, event_t event)
//----- (457AC0) --------------------------------------------------------
fx_phyrexian_gremlin_freeze_artifact_903(int player, int card, event_t event)
//----- (457B10) --------------------------------------------------------
fx_nettling_imp_903(int player, int card, event_t event)
//----- (457C20) --------------------------------------------------------
fx_animate_land_903(int player, int card, event_t event)
//----- (457D00) --------------------------------------------------------
fx_raging_river_903(int player, int card, event_t event)
//----- (457E10) --------------------------------------------------------
fx_custom_903(int player, int card, event_t event)
//----- (457EC0) --------------------------------------------------------
fx_sewers_of_estark_903(int player, int card, event_t event)
//----- (457FA0) --------------------------------------------------------
fx_ashnods_transmogrant_903(int player, int card, event_t event)
//----- (457FF0) --------------------------------------------------------
C_helper_jeweled_bird(int a3, int player, int card)
//----- (458070) --------------------------------------------------------
C_ante_top_card_of_library(int player)
//----- (458150) --------------------------------------------------------
sub_458150(int a1, int player, target_t *a3)
//----- (458430) --------------------------------------------------------
card_multiblocker_impl(int player, int card, event_t event)
//----- (4585B0) --------------------------------------------------------
redirect_dtgtpc_to_cur_tgts_dsrcpc(int cur_tgt_player, int cur_tgt_card, int player, int card, int unused)
//----- (4585F0) --------------------------------------------------------
backend_of_control_magic(int player, int card)
//----- (458780) --------------------------------------------------------
sub_458780(int player, int card)
//----- (458870) --------------------------------------------------------
C_helper_arena2(int a1, int a2)
//----- (458920) --------------------------------------------------------
sub_458920(int a1, int a2, int a3)
//----- (458A20) --------------------------------------------------------
helper_argivian_blacksmith()
//----- (458AB0) --------------------------------------------------------
helper_raging_river1(int player, int card, int a3, int a4, int a5)
//----- (458B20) --------------------------------------------------------
helper_raging_river2(int player, int card, int a3, int a4, int a5)
//----- (458B90) --------------------------------------------------------
sub_458B90(int a1, unsigned __int8 a2)
//----- (458C40) --------------------------------------------------------
sub_458C40(int player, int card, int extra)
//----- (458CE0) --------------------------------------------------------
C_helper_jeweled_bird2(int a1, signed int a2)
//----- (458EC0) --------------------------------------------------------
helper_aladdin(int player, int card, int tgt_player, int tgt_card)
//----- (458F40) --------------------------------------------------------
card_repentant_blacksmith(int player, int card, event_t event)
//----- (458FB0) --------------------------------------------------------
C_helper_arena(int a1, int a2, unsigned __int8 a3, target_t *a4)
//----- (459100) --------------------------------------------------------
sub_459100(int a1, type_t a2, unsigned __int8 a3)
//----- (459240) --------------------------------------------------------
C_helper_drop_of_honey(int a1, int a2, target_t *a3)
//----- (459350) --------------------------------------------------------
helper_serendib_djinn(int player, target_t *a2, int a3, int a4)
//----- (459520) --------------------------------------------------------
is_basic_or_beta_dual_land_and_specific_land_subtype(int a1, int a2)
//----- (459590) --------------------------------------------------------
sub_459590(int player, int card, int a3)
//----- (459630) --------------------------------------------------------
urzatron_parts(signed int player)
//----- (4596B0) --------------------------------------------------------
gain_life(int player, int amt)
//----- (459760) --------------------------------------------------------
oubliette_helper_1(int a3, int a4)
//----- (4597A0) --------------------------------------------------------
oubliette_helper_4(int player, int card)
//----- (4597F0) --------------------------------------------------------
oubliette_effect(int player, int card)
//----- (459840) --------------------------------------------------------
oubliette_helper_5(int player, int card)
//----- (4598F0) --------------------------------------------------------
oubliette_helper_2(int a3, int a4)
//----- (459970) --------------------------------------------------------
oubliette_helper_3(int a1, int a2, int player, int card, int a5)
//----- (459AA0) --------------------------------------------------------
dispatch_function_to_all_cards_in_play(int player, int card, int (__cdecl *fn)(_DWORD, _DWORD, _DWORD, _DWORD, _DWORD), int only_if_this_player_controls_or_negone)
//----- (459B40) --------------------------------------------------------
UNKNOWN_part_of_loading_graphics<eax>(int a1<ebp>)
//----- (459B60) --------------------------------------------------------
sub_459B60<eax>(int result<eax>, int a2<ecx>, int a3<ebp>)
//----- (459B70) --------------------------------------------------------
sub_459B70()
//----- (459B80) --------------------------------------------------------
sub_459B80<eax>(int a1<eax>, int a2<ebp>)
//----- (459B90) --------------------------------------------------------
sub_459B90<eax>(int result<eax>, int a2<ebp>)
//----- (459BA0) --------------------------------------------------------
sub_459BA0<eax>(int a1<eax>, int a2<ebp>)
//----- (459BC0) --------------------------------------------------------
sub_459BC0<eax>(int a1<eax>)
//----- (459FA0) --------------------------------------------------------
read_graphics_file(int a1)
//----- (45A040) --------------------------------------------------------
load_magsnd_dll(int a1, int a2, int a3)
//----- (45A160) --------------------------------------------------------
TENTATIVE_finalize_sound_system()
//----- (45A1C0) --------------------------------------------------------
call_LoadSnd(char *a1, int a2, int *a3)
//----- (45A1F0) --------------------------------------------------------
call_UnloadSnd(int a1)
//----- (45A220) --------------------------------------------------------
call_UnloadAllSnds()
//----- (45A240) --------------------------------------------------------
call_PlaySnd(int a1, int *a2)
//----- (45A270) --------------------------------------------------------
call_IsSndLoaded(wav_t a1, wav_t *a2)
//----- (45A2A0) --------------------------------------------------------
call_GetLRUSnd(wav_t *a1, wav_t a2, wav_t a3)
//----- (45A2E0) --------------------------------------------------------
clear_magsnd_dll_fn_pointers()
//----- (45A310) --------------------------------------------------------
register_SpellChainClass(const CHAR *a1)
//----- (45A5F0) --------------------------------------------------------
destroy_SpellChainClass()
//----- (45A720) --------------------------------------------------------
wndproc_SpellChainClass(HWND hwnd, WM_t msg, WPARAM wparam, LPARAM lparam)
//----- (45C020) --------------------------------------------------------
sub_45C020(HWND hWnd, int a2, int a3, int a4, int a5, int a6, int a7, int a8, int a9, int a10, int a11, int a12, int a13, int a14, int a15, int a16, int a17, int a18, int a19, int a20, int a21, int a22, int a23, int a24, int a25, int a26, int a27, int a28, int a29, int a30, int a31, int a32, int a33, int a34, int a35, int a36, int a37, int a38, int a39, int a40, int a41, int a42, int a43, int a44, signed int a45)
//----- (45C0B0) --------------------------------------------------------
sub_45C0B0(HWND hWnd, signed int a2)
//----- (45C1D0) --------------------------------------------------------
sub_45C1D0(int a1, HWND hWnd, int a3, int a4, int a5, int a6, int a7, int a8, int a9, int a10, int a11, int a12, int a13, int a14, int a15, int a16, int a17, int a18, int a19, int a20, int a21, int a22, char a23, int a24, int a25, int a26, int a27, int a28, int a29, int a30, int a31, int a32, int a33, int a34, int a35, int a36, int a37, int a38, int a39, int a40, int a41, int a42, int a43, int a44, int a45, int a46, int a47, int a48, int a49, int a50, int a51, int a52, int a53, int a54, int a55, int a56, int a57, int a58, int a59, int a60, int a61, int a62, int a63)
//----- (45C240) --------------------------------------------------------
sub_45C240(HWND hWnd, int a2, int a3, int a4, int a5, int a6, int a7, int a8, int a9, int a10, int a11, int a12, int a13, int a14, int a15, int a16, int a17, int a18, int a19, int a20, int a21, int a22, int a23, int a24, int a25, int a26, int a27, int a28, int a29, int a30, int a31, int a32, int a33, int a34, int a35, int a36, int a37, int a38, int a39, int a40, int a41, int a42, int a43, int a44, LONG a45)
//----- (45C420) --------------------------------------------------------
sub_45C420(HWND hWnd, signed int a2)
//----- (45C4F0) --------------------------------------------------------
sub_45C4F0(HWND hWndParent, int a2, int a3, int a4, _BYTE *a5, int a6, int a7, int a8, int a9, int a10, int a11, int a12, int a13, int a14, int a15, int a16, int a17, int a18, int a19, int a20, int a21, int a22, int a23, int a24, int a25, int a26, int a27, int a28, int a29, int a30, int a31, int a32, int a33, int a34, int a35, int a36, int a37, int a38, int a39, int a40, int a41, int a42, int a43, int a44, int a45)
//----- (45C620) --------------------------------------------------------
sub_45C620(HWND hDlg, LPRECT lprcDst)
//----- (45CBA0) --------------------------------------------------------
wndproc_SpellMinimized(HWND hwnd, WM_t msg, WPARAM wparam, LPARAM lparam)
//----- (45CEC0) --------------------------------------------------------
sub_45CEC0()
//----- (45CF00) --------------------------------------------------------
sub_45CF00()
//----- (45CF40) --------------------------------------------------------
sub_45CF40(HWND hWndParent)
//----- (45D0B0) --------------------------------------------------------
sub_45D0B0(HWND hWndParent)
//----- (45D0E0) --------------------------------------------------------
sub_45D0E0(HWND hwnd, WM_t msg, int wparam, int lparam)
//----- (45D8E0) --------------------------------------------------------
sub_45D8E0(int a1, int a2, int a3, int a4, int a5, void *a6, int a7, int a8)
//----- (45D9C0) --------------------------------------------------------
sub_45D9C0(void *a1, HGDIOBJ ho, void *a3, void *a4)
//----- (45DA10) --------------------------------------------------------
TENTATIVE_read_options_from_registry()
//----- (45E0F0) --------------------------------------------------------
sub_45E0F0()
//----- (45E4A0) --------------------------------------------------------
TENTATIVE_get_opponent_data_from_registry()
//----- (45E7A0) --------------------------------------------------------
TENTATIVE_set_soloduel_data_in_registry()
//----- (45E990) --------------------------------------------------------
TENTATIVE_get_gauntlet_data_from_registry()
//----- (45EBE0) --------------------------------------------------------
TENTATIVE_set_gauntlet_data_in_registry()
//----- (45ED70) --------------------------------------------------------
TENTATIVE_get_sealeddeck_data_from_registry()
//----- (45F1E0) --------------------------------------------------------
sub_45F1E0()
//----- (45F420) --------------------------------------------------------
sub_45F420()
//----- (45F720) --------------------------------------------------------
sub_45F720()
//----- (45F930) --------------------------------------------------------
createdialog_screennamepage(LPARAM dwInitParam)
//----- (45F960) --------------------------------------------------------
sub_45F960()
//----- (45F990) --------------------------------------------------------
rfalse()
//----- (45F9A0) --------------------------------------------------------
rtrue()
//----- (45F9B0) --------------------------------------------------------
dlgproc_screennamepage(HWND hwnd, WM_t msg, int wparam, int lparam)
//----- (4618D0) --------------------------------------------------------
sub_4618D0(HWND a1)
//----- (461A30) --------------------------------------------------------
sub_461A30(HWND hDlg)
//----- (4625A0) --------------------------------------------------------
sub_4625A0(HWND hDlg)
//----- (462700) --------------------------------------------------------
sub_462700()
//----- (4627C0) --------------------------------------------------------
sub_4627C0()
//----- (462880) --------------------------------------------------------
sub_462880()
//----- (4628E0) --------------------------------------------------------
sub_4628E0()
//----- (462940) --------------------------------------------------------
sub_462940()
//----- (462A00) --------------------------------------------------------
sub_462A00(char *deststr, int a2)
//----- (462B90) --------------------------------------------------------
sub_462B90(HWND hDlg)
//----- (462FC0) --------------------------------------------------------
sub_462FC0(HWND hwnd, WM_t msg, WPARAM wparam, __int32 lparam)
//----- (463370) --------------------------------------------------------
sub_463370(HWND hWndTo, LPRECT lpPoints)
//----- (4633B0) --------------------------------------------------------
register_ScrollbarClass(const CHAR *a1)
//----- (463460) --------------------------------------------------------
destroy_ScrollbarClass()
//----- (4634A0) --------------------------------------------------------
wndproc_ScrollbarClass(HWND hwnd, WM_t msg, WPARAM wparam, LPARAM lparam)
//----- (4644F0) --------------------------------------------------------
sub_4644F0(HWND hWnd, LONG a2)
//----- (464570) --------------------------------------------------------
sub_464570(HWND hWnd, LONG a2)
//----- (4645F0) --------------------------------------------------------
sub_4645F0(HWND hWnd, LPRECT lprc)
//----- (464740) --------------------------------------------------------
sub_464740()
//----- (464790) --------------------------------------------------------
sub_464790()
//----- (4647D0) --------------------------------------------------------
stash_standard_text()
//----- (466020) --------------------------------------------------------
get_path_base(char *path_base)
//----- (466050) --------------------------------------------------------
SelectPalette_RealizePalette_GdiFlush_SetStretchBltMode_to_COLORONCOLOR(HDC hdc)
//----- (466080) --------------------------------------------------------
sub_466080(int a1, int a2, int a3, const BITMAPINFO *lpbmi, int a5, int a6, int a7)
//----- (4661C0) --------------------------------------------------------
checked_DeleteDC_DeleteObject(HDC hdc, HGDIOBJ ho)
//----- (4661F0) --------------------------------------------------------
sub_4661F0(RECT *a1, RECT *a2, signed int a3, signed int a4)
//----- (4662F0) --------------------------------------------------------
sub_4662F0(HDC hdcDest, RECT *a2, HANDLE h)
//----- (466350) --------------------------------------------------------
sub_466350(HDC hdcDest, RECT *a2, HGDIOBJ h, int xSrc, int ySrc, int a6, int a7)
//----- (466450) --------------------------------------------------------
sub_466450(HDC hdcDest, RECT *r, HANDLE h)
//----- (466540) --------------------------------------------------------
stretchblt_with_transparency_where_stencil_is_right_half(HDC hdc, RECT *r, HANDLE h)
//----- (4665C0) --------------------------------------------------------
stretchblt_with_transparency(HDC hdc, RECT *r, HGDIOBJ h, int src_width, int src_height, int img_x, int img_y, int stencil_x, int stencil_y)
//----- (466720) --------------------------------------------------------
TENTATIVE_read_bmp_file(const char *a1)
//----- (466A00) --------------------------------------------------------
checked_DeleteObject(HGDIOBJ ho)
//----- (466A20) --------------------------------------------------------
TENTATIVE_setup_palette()
//----- (466CD0) --------------------------------------------------------
sub_466CD0()
//----- (466CF0) --------------------------------------------------------
sub_466CF0(int a1, int a2, const RECT *lprcSrc)
//----- (466E90) --------------------------------------------------------
find_and_replace(char *str, const char *needle, int case_sensitive, char *replacement)
//----- (466FC0) --------------------------------------------------------
TENTATIVE_raw_replace_hack_symbol(char *str, int a2, int a3, int a4)
//----- (4670B0) --------------------------------------------------------
TENTATIVE_raw_replace_sleight_symbol(char *str, int a2, int a3, int a4)
//----- (4671A0) --------------------------------------------------------
sub_4671A0(signed int a1)
//----- (4671D0) --------------------------------------------------------
sub_4671D0(HWND hWnd, const char *srcstr)
//----- (4672C0) --------------------------------------------------------
sub_4672C0(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
//----- (467310) --------------------------------------------------------
sub_467310(LPCSTR str, COLORREF color, HBRUSH hbr)
//----- (4673C0) --------------------------------------------------------
sub_4673C0(int a1, HBRUSH hbr, void *a3, HGDIOBJ h, COLORREF color, int a6)
//----- (4673F0) --------------------------------------------------------
sub_4673F0(int a1, HBRUSH hbr, void *a3, HGDIOBJ h, COLORREF color, int a6)
//----- (467420) --------------------------------------------------------
sub_467420(int a1, void *a2, void *a3, HGDIOBJ h, COLORREF color, int a6)
//----- (467450) --------------------------------------------------------
sub_467450(int a1, HBRUSH hbr, void *a3, void *a4, HGDIOBJ h, COLORREF color, int a7, UINT format)
//----- (467830) --------------------------------------------------------
sub_467830(int a1, void *a2, HANDLE h, void *a4, COLORREF color, int a6)
//----- (467A40) --------------------------------------------------------
sub_467A40(HWND hWndParent)
//----- (467A60) --------------------------------------------------------
sub_467A60(HWND hWnd, int a2)
//----- (467AA0) --------------------------------------------------------
sub_467AA0(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
//----- (467BA0) --------------------------------------------------------
sub_467BA0(HWND hWnd)
//----- (467BE0) --------------------------------------------------------
prepare_logfont_from_dueldat(const char *name, int italic)
//----- (467CE0) --------------------------------------------------------
sub_467CE0()
//----- (467E30) --------------------------------------------------------
sub_467E30(int a1, signed int a2)
//----- (467E90) --------------------------------------------------------
sub_467E90(HWND hWndInsertAfter, int a2, signed int a3)
//----- (467F40) --------------------------------------------------------
rgb_to_colorref_of_palette(int a1)
//----- (467FA0) --------------------------------------------------------
sub_467FA0(HWND hwnd, WM_t msg, WPARAM wparam, LPARAM lparam)
//----- (468110) --------------------------------------------------------
sub_468110(HWND hWnd, int a2)
//----- (468160) --------------------------------------------------------
C_get_card_image_number(int csvid, int player, int card)
//----- (4681A0) --------------------------------------------------------
sub_4681A0<eax>(int a1, int a2)
//----- (4681F0) --------------------------------------------------------
sub_4681F0(int a1)
//----- (468210) --------------------------------------------------------
sub_468210(LPSTR Str, DWORD nSize, LPCVOID lpSource, ...)
//----- (468280) --------------------------------------------------------
sub_468280(signed int a1, signed int a2, char a3)
//----- (4682F0) --------------------------------------------------------
sub_4682F0()
//----- (4683B0) --------------------------------------------------------
sub_4683B0(int a1, signed int a2, int a3, int a4, int a5)
//----- (4684B0) --------------------------------------------------------
sub_4684B0(int a1, signed int a2, int a3, int a4, int a5, unsigned int a6)
//----- (4685A0) --------------------------------------------------------
register_LibraryClass(const CHAR *a1)
//----- (468700) --------------------------------------------------------
destroy_LibraryClass()
//----- (468750) --------------------------------------------------------
redraw_libraries()
//----- (468780) --------------------------------------------------------
wndproc_LibraryClass(HWND hwnd, WM_t msg, WPARAM wparam, LPARAM lparam)
//----- (469150) --------------------------------------------------------
sub_469150(int a1)
//----- (469170) --------------------------------------------------------
wndproc_ShuffleCard(HWND hwnd, WM_t msg, WPARAM wparam, LPARAM lparam)
//----- (469490) --------------------------------------------------------
TENTATIVE_prompt_for_mulligan(int *a1, int *a2, __int32 a3, int a4, int a5, int a6, int a7, int a8, int a9, int *a10)
//----- (469690) --------------------------------------------------------
TENTATIVE_dlgproc_play_or_draw(HWND hwnd, WM_t msg, WPARAM wparam, int lparam)
//----- (469FB0) --------------------------------------------------------
sub_469FB0(HWND hWnd)
//----- (46A000) --------------------------------------------------------
sub_46A000(HGDIOBJ *a1, COLORREF *a2, HGDIOBJ *a3, HGDIOBJ *a4, int *a5, int *a6)
//----- (46A0B0) --------------------------------------------------------
sub_46A0B0(HGDIOBJ ho, void *a2, void *a3)
//----- (46A0F0) --------------------------------------------------------
sub_46A0F0(HGDIOBJ *arg0, COLORREF *a2, HGDIOBJ *a3, HGDIOBJ *a4, HGDIOBJ *a5, int *a6, int *a7)
//----- (46A1D0) --------------------------------------------------------
sub_46A1D0(HGDIOBJ ho, void *a2, void *a3, void *a4)
//----- (46A220) --------------------------------------------------------
end_the_game(int a1, const char *a2, int a3, const char *a4, int a5, const char *a6, int a7)
//----- (46A530) --------------------------------------------------------
dlgproc_endduel(HWND hWndTo, WM_t msg, HDC wparam, int lparam)
//----- (46B6F0) --------------------------------------------------------
sub_46B6F0(HGDIOBJ *a1, COLORREF *a2, COLORREF *a3, HPEN *a4, HPEN *a5, HPEN *a6, COLORREF *a7, COLORREF *a8)
//----- (46B7D0) --------------------------------------------------------
sub_46B7D0(void *a1, HGDIOBJ ho, void *a3, void *a4)
//----- (46B820) --------------------------------------------------------
sub_46B820(int a1)
//----- (46B950) --------------------------------------------------------
dlgproc_gauntletendduel(HWND hwnd, WM_t msg, HDC wparam, int lparam)
//----- (46C370) --------------------------------------------------------
sub_46C370(HGDIOBJ *a1, COLORREF *a2, COLORREF *a3, HBRUSH *a4, HPEN *a5, HPEN *a6, COLORREF *a7, COLORREF *a8)
//----- (46C450) --------------------------------------------------------
sub_46C450(void *a1, HGDIOBJ ho, void *a3, void *a4)
//----- (46C4A0) --------------------------------------------------------
sub_46C4A0(int *deck, int a2, int a3, int num_cards, const char *prompt, int suppress_done_txt, const char *done)
//----- (46C660) --------------------------------------------------------
sub_46C660(int player, const int *deck, int unk1, int *scratch, int num_cards, const char **prompt, int a6, int *a7, int a8, int num_in_scratch)
//----- (46C8F0) --------------------------------------------------------
dlgfunc_show_deck(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
//----- (46D870) --------------------------------------------------------
sub_46D870(HBRUSH *a1, HPEN *a2, HPEN *a3, HPEN *a4, HBRUSH *a5, COLORREF *a6)
//----- (46D940) --------------------------------------------------------
sub_46D940(HGDIOBJ ho, void *a2, void *a3, void *a4, void *a5)
//----- (46D9A0) --------------------------------------------------------
wndproc_ShowListCard(HWND hWndParent, WM_t msg, LPARAM lparam, WPARAM wparam)
//----- (46DCF0) --------------------------------------------------------
choose_a_number_dialog(int player, const char *prompt, int maxnum)
//----- (46DD40) --------------------------------------------------------
dlgproc_choose_a_number(HWND hwnd, WM_t msg, WPARAM wparam, choose_a_number_args_t *lparam)
//----- (46E200) --------------------------------------------------------
sub_46E200(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
//----- (46E260) --------------------------------------------------------
sub_46E260(HGDIOBJ *a1, COLORREF *a2, HBRUSH *a3, HPEN *a4, HPEN *a5, COLORREF *a6, COLORREF *a7)
//----- (46E330) --------------------------------------------------------
sub_46E330(void *a1, HGDIOBJ ho, void *a3, void *a4)
//----- (46E380) --------------------------------------------------------
choose_a_color_dialog(int player, const char *prompt, int use_color_names_instead_of_land, int ai_choice, color_test_t a5)
//----- (46E540) --------------------------------------------------------
dlgproc_choose_a_color(HWND hwnd, WM_t msg, WPARAM wparam, LPARAM lparam)
//----- (46F240) --------------------------------------------------------
sub_46F240(HGDIOBJ *a1, COLORREF *a2, HGDIOBJ *a3, int a4, COLORREF *a5, HBRUSH *a6, HPEN *a7, HPEN *a8, COLORREF *a9, COLORREF *a10)
//----- (46F450) --------------------------------------------------------
sub_46F450(void *a1, void *a2, int a3, HGDIOBJ ho, void *a5, void *a6)
//----- (46F4F0) --------------------------------------------------------
sub_46F4F0(LPRECT lpPoints, HWND hWndTo, int a3)
//----- (46F680) --------------------------------------------------------
sub_46F680(int a1, int a2, LPARAM a3, int a4, int a5)
//----- (46F780) --------------------------------------------------------
TENTATIVE_dlgproc_hack_or_sleight(HWND hwnd, WM_t msg, HDC wparam, int lparam)
//----- (470130) --------------------------------------------------------
sub_470130(HGDIOBJ *a1, COLORREF *a2, HBRUSH *a3, HPEN *a4, HPEN *a5, COLORREF *a6, COLORREF *a7)
//----- (470200) --------------------------------------------------------
sub_470200(void *a1, HGDIOBJ ho, void *a3, void *a4)
//----- (470250) --------------------------------------------------------
sub_470250(LPARAM a1, int a2)
//----- (4702A0) --------------------------------------------------------
TENTATIVE_dlgproc_manaburn(HWND hwnd, WM_t msg, unsigned int wparam, LPARAM lparam)
//----- (470690) --------------------------------------------------------
coin_flip(int player, const char *dialog_title, int show_dialog_if_animation_is_off)
//----- (4707C0) --------------------------------------------------------
dlgproc_coin_flip(HWND hwnd, WM_t msg, UINT_PTR wparam, LPARAM lparam)
//----- (470DF0) --------------------------------------------------------
sub_470DF0(HBRUSH *a1, COLORREF *a2)
//----- (470E10) --------------------------------------------------------
sub_470E10(HGDIOBJ ho)
//----- (470E30) --------------------------------------------------------
sub_470E30(int a1, int a2, LPARAM a3, int a4, int a5, int a6, int a7)
//----- (470F80) --------------------------------------------------------
dlgproc_fireball(HWND hwnd, WM_t msg, WPARAM wparam, LPARAM lparam)
//----- (471A00) --------------------------------------------------------
sub_471A00(HGDIOBJ *a1, COLORREF *a2, HBRUSH *a3, HPEN *a4, HPEN *a5, COLORREF *a6, COLORREF *a7)
//----- (471AD0) --------------------------------------------------------
sub_471AD0(void *a1, HGDIOBJ ho, void *a3, void *a4)
//----- (471B20) --------------------------------------------------------
sub_471B20(int a1, signed int a2)
//----- (471B60) --------------------------------------------------------
raw_do_dialog(int bigcard_player, int bigcard_card, int smallcard_player, int smallcard_card, const char *str, int human_chooses)
//----- (471C30) --------------------------------------------------------
get_csvid_from_player_card(int player, int card)
//----- (471C70) --------------------------------------------------------
CardTypeFromID(int a1)
//----- (471CB0) --------------------------------------------------------
CardIDFromType(int a1)
//----- (471CE0) --------------------------------------------------------
CardInDeck(int a1)
//----- (471D00) --------------------------------------------------------
SetCardInDeck(int a1, int a2)
//----- (471D30) --------------------------------------------------------
display_error_message<eax>(const char *a1<eax>)
//----- (471D60) --------------------------------------------------------
call_sub_437E20_unless_ai_is_speculating()
//----- (471D70) --------------------------------------------------------
TENTATIVE_centerwindow_message(const char *srcstr)
//----- (471DA0) --------------------------------------------------------
sub_471DA0(int *deck, int num_cards, const char *prompt, int suppress_done_txt, const char *done)
//----- (471DE0) --------------------------------------------------------
sub_471DE0(int a1, int a2, int a3, int a4, int a5, const char *a6)
//----- (471E20) --------------------------------------------------------
sub_471E20(int player, const int *deck, int *scratch, int num_cards, const char **prompt, int a6, int *a7, int a8, int num_in_scratch)
//----- (471E70) --------------------------------------------------------
sub_471E70(int a1)
//----- (471EB0) --------------------------------------------------------
do_dialog(int who_sees, int bigcard_player, int bigcard_card, int smallcard_player, int smallcard_card, const char *prompt, int ai_choice)
//----- (4720E0) --------------------------------------------------------
sub_4720E0(int a1, int a2)
//----- (472120) --------------------------------------------------------
choose_a_number(int player, char *prompt, int maxnum)
//----- (4721C0) --------------------------------------------------------
choose_a_color(int player, const char *prompt, int use_color_names_instead_of_land, int ai_choice, color_test_t colors)
//----- (472260) --------------------------------------------------------
TENTATIVE_reassess_all_cards()
//----- (472400) --------------------------------------------------------
count_mana()
//----- (4725C0) --------------------------------------------------------
C_count_colors_of_lands_in_play()
//----- (472980) --------------------------------------------------------
sub_472980()
//----- (472B90) --------------------------------------------------------
sub_472B90()
//----- (472C90) --------------------------------------------------------
get_usertime_of_current_thread_in_ms()
//----- (472D00) --------------------------------------------------------
start_timer_for_ai_speculation()
//----- (472D10) --------------------------------------------------------
check_timer_for_ai_speculation()
//----- (472D30) --------------------------------------------------------
sub_472D30(int a1, int a2, int a3)
//----- (472DD0) --------------------------------------------------------
card_aswan_jaguar(int player, int card, event_t event)
//----- (473080) --------------------------------------------------------
UNKNOWN_aswan_jaguar_helper(int player, int card, int32_t a3)
//----- (4730F0) --------------------------------------------------------
C_card_goblin_polka_band(int player, int card, event_t event)
//----- (473210) --------------------------------------------------------
C_goblin_polka_band_helper(int player, int card, signed int a3)
//----- (473370) --------------------------------------------------------
faerie_dragon_helper3(int a1, int a2, target_t *a3)
//----- (4734E0) --------------------------------------------------------
C_goblin_polka_band_helper2(int a1, int a2)
//----- (473560) --------------------------------------------------------
C_fx_polka_903(int player, int card, event_t event)
//----- (4735C0) --------------------------------------------------------
C_card_faerie_dragon(int player, int card, event_t event)
//----- (473720) --------------------------------------------------------
C_faerie_dragon_helper(int player, int card, signed int a3)
//----- (473F50) --------------------------------------------------------
card_whimsy(int player, int card, event_t event)
//----- (4741D0) --------------------------------------------------------
UNKNOWN_whimsy_helper(int a1, int a2, target_t *a3, __int16 a4)
//----- (474280) --------------------------------------------------------
whimsy_effects(int player, int card, event_t event)
//----- (474C30) --------------------------------------------------------
C_card_army_of_allah(int player, int card, event_t event)
//----- (474CC0) --------------------------------------------------------
card_blaze_of_glory(int player, int card, event_t event)
//----- (474D90) --------------------------------------------------------
card_guardian_angel(int player, int card, event_t event)
//----- (474F60) --------------------------------------------------------
card_natural_selection(int player, int card, event_t event)
//----- (475170) --------------------------------------------------------
card_psionic_blast(int player, int card, event_t event)
//----- (475260) --------------------------------------------------------
C_card_reverse_polarity(int player, int card, event_t event)
//----- (475710) --------------------------------------------------------
card_sewers_of_estark(int player, int card, event_t event)
//----- (475810) --------------------------------------------------------
card_artifact_blast(int player, int card, event_t event)
//----- (475960) --------------------------------------------------------
card_sacrifice(int player, int card, event_t event)
//----- (475A30) --------------------------------------------------------
sub_475A30(int TENTATIVE_player, const char *srcstr)
//----- (476840) --------------------------------------------------------
sub_476840(int a1)
//----- (476B90) --------------------------------------------------------
sub_476B90(int player, int card)
//----- (476F10) --------------------------------------------------------
C_current_trigger_or_event_is_forced()
//----- (477070) --------------------------------------------------------
resolve_damage_cards_and_prevent_damage()
//----- (477340) --------------------------------------------------------
C_after_damage()
//----- (477410) --------------------------------------------------------
TENTATIVE_reassess_all_cards_helper(int player, int card)
//----- (477970) --------------------------------------------------------
dispatch_trigger_to_one_card(int player, int card, int event)
//----- (4779F0) --------------------------------------------------------
C_kill_card(int player, int card, kill_t kill_code)
//----- (477A90) --------------------------------------------------------
regenerate_or_graveyard_triggers()
//----- (477B30) --------------------------------------------------------
kill_card_guts(int player, int card)
//----- (477D90) --------------------------------------------------------
C_destroy_attached_auras_and_obliterate_card(int player, int card)
//----- (477EC0) --------------------------------------------------------
C_raw_put_card_in_graveyard(int player, int card)
//----- (477F30) --------------------------------------------------------
C_remove_card_from_grave(int player, int position)
//----- (477F60) --------------------------------------------------------
put_card_in_exile(int whose_exile, int card, int player)
//----- (477FD0) --------------------------------------------------------
register_CueCardClass(const CHAR *a1)
//----- (4780E0) --------------------------------------------------------
destroy_CueCardClass()
//----- (478170) --------------------------------------------------------
wndproc_CueCardClass(HWND hwnd, WM_t msg, WPARAM wparam, LPARAM lparam)
//----- (4785E0) --------------------------------------------------------
sub_4785E0(int a1)
//----- (478AC0) --------------------------------------------------------
start_duel()
//----- (479110) --------------------------------------------------------
assess_mulligan_oldstyle(int *player0_can_mulligan, int *player1_can_mulligan, int *player1_should_mulligan)
//----- (4791F0) --------------------------------------------------------
lose_the_game(DWORD dwExitCode)
//----- (479220) --------------------------------------------------------
sub_479220(int a1, int a2, int a3, int a4, int a5, int a6, const char *a7)
//----- (4792C0) --------------------------------------------------------
sub_4792C0(int a1, int a2, int a3, int a4, int a5, int a6, const char *a7)
//----- (479360) --------------------------------------------------------
show_deck(int player, int *deck, int num_cards, const char *prompt, int suppress_done_txt, const char *done)
//----- (47936D) --------------------------------------------------------
show_deck_finish(int player, int *deck, int num_cards, const char *prompt, int suppress_done_txt, const char *done)
//----- (479620) --------------------------------------------------------
add_card_to_hand(int player, int iid)
//----- (4796A0) --------------------------------------------------------
C_create_card_instance(int player, unsigned int iid, int card)
//----- (479840) --------------------------------------------------------
sub_479840()
//----- (479880) --------------------------------------------------------
C_calc_attack_rating(int player, int card)
//----- (4799B0) --------------------------------------------------------
sub_4799B0(int a1)
//----- (479AE0) --------------------------------------------------------
sub_479AE0(int a1)
//----- (479B20) --------------------------------------------------------
sub_479B20(signed int a1)
//----- (479B50) --------------------------------------------------------
sub_479B50(int a1, int a2, signed int a3)
//----- (479BF0) --------------------------------------------------------
shuffle(int TENTATIVE_controller_of_shuffle_effect, int player_to_shuffle)
//----- (479C60) --------------------------------------------------------
WILDGUESS_send_deck_order_over_network(int a1)
//----- (479CC0) --------------------------------------------------------
WILDGUESS_receive_deck_order_from_network(int a1)
//----- (479D10) --------------------------------------------------------
remove_card_from_deck(int a1, int a2)
//----- (479D40) --------------------------------------------------------
put_card_on_bottom_of_deck(int a1, int a2)
//----- (479D80) --------------------------------------------------------
real_put_on_top_of_deck(int who, int original_iid)
//----- (479DB0) --------------------------------------------------------
card_data_rarity_but_change_to_4_first_if_extra_ability_has_0x180_or_expansion_is_64(int iid)
//----- (479DF0) --------------------------------------------------------
get_internal_card_id_from_csv_id(card_id_t csvid)
//----- (479E30) --------------------------------------------------------
sub_479E30(int a1)
//----- (479E90) --------------------------------------------------------
card_swamp(int player, int card, event_t event)
//----- (479EB0) --------------------------------------------------------
card_island(int player, int card, event_t event)
//----- (479ED0) --------------------------------------------------------
card_forest(int player, int card, event_t event)
//----- (479EF0) --------------------------------------------------------
card_mountain(int player, int card, event_t event)
//----- (479F10) --------------------------------------------------------
card_plains(int player, int card, event_t event)
//----- (479F30) --------------------------------------------------------
card_gem_bazaar(int player, int card, event_t event)
//----- (47A010) --------------------------------------------------------
C_card_oasis(int player, int card, event_t event)
//----- (47A2C0) --------------------------------------------------------
card_strip_mine(int player, int card, event_t event)
//----- (47A480) --------------------------------------------------------
card_library_of_alexandria(int player, int card, event_t event)
//----- (47A660) --------------------------------------------------------
card_mishras_factory(int player, int card, event_t event)
//----- (47AE00) --------------------------------------------------------
card_assembly_worker(int player, int card, event_t event)
//----- (47B570) --------------------------------------------------------
UNKNOWN_assembly_worker_helper(int player, int card, int a5)
//----- (47B5E0) --------------------------------------------------------
C_card_mishras_workshop(int player, int card, event_t event)
//----- (47B6C0) --------------------------------------------------------
mana_producer_sound_on_resolve(int player, int card, event_t event, color_t color)
//----- (47B7A0) --------------------------------------------------------
calloc_48_bytes()
//----- (47B7C0) --------------------------------------------------------
TENTATIVE_read_palette(const char *Filename, const char *a2)
//----- (47BA00) --------------------------------------------------------
sub_47BA00()
//----- (47BA50) --------------------------------------------------------
sub_47BA50(int *a1, char *a2, int *a3)
//----- (47BAC0) --------------------------------------------------------
sub_47BAC0(int **a1)
//----- (47BBE0) --------------------------------------------------------
sub_47BBE0(int a1, const char *Str, int a3)
//----- (47BCC0) --------------------------------------------------------
sub_47BCC0(void *memory)
//----- (47BD50) --------------------------------------------------------
sub_47BD50()
//----- (47BE40) --------------------------------------------------------
sub_47BE40(int a1, int a2)
//----- (47BEC0) --------------------------------------------------------
sub_47BEC0()
//----- (47BEE0) --------------------------------------------------------
register_HandClass(const CHAR *a1)
//----- (47BFC0) --------------------------------------------------------
destroy_HandClass()
//----- (47C000) --------------------------------------------------------
wndproc_HandClass(HWND hwnd, WM_t msg, WPARAM wparam, LPARAM lparam)
//----- (47D2F0) --------------------------------------------------------
TENTATIVE_update_hand_window(HWND hWnd)
//----- (47D5F0) --------------------------------------------------------
sub_47D5F0(HDC hdcDest, RECT *a2, RECT *a3, int a4, int a5, int a6, int a7, int a8, HANDLE h)
//----- (47D850) --------------------------------------------------------
sub_47D850(int a1, int a2, signed int a3, int a4, int a5, int a6, int a7, int a8, int a9, int a10)
//----- (47D8E0) --------------------------------------------------------
sub_47D8E0(char *lpString, HWND hWnd, int a3)
//----- (47D940) --------------------------------------------------------
C_real_target_available(int *return_count, int unknown_a4, int who_chooses, int allowed_controller, int preferred_controller, target_zone_t zone, type_t required_type, type_t illegal_type, keyword_t required_abilities, keyword_t illegal_abilities, color_test_t required_color, color_test_t illegal_color, int extra, subtype_in_card_data_t required_subtype, int power_requirement, int toughness_requirement, target_special_t special, target_state_t required_state, target_state_t illegal_state)
//----- (47DB90) --------------------------------------------------------
C_real_validate_target(int tgt_player, int tgt_card, char *return_error_str, int who_chooses, int allowed_controller, int preferred_controller, target_zone_t zone, type_t required_type, type_t illegal_type, keyword_t required_abilities, keyword_t illegal_abilities, color_test_t required_color, color_test_t illegal_color, int extra, subtype_in_card_data_t required_subtype, int power_requirement, int toughness_requirement, target_special_t special, target_state_t required_state, target_state_t illegal_state)
//----- (47E980) --------------------------------------------------------
sub_47E980(int a1, int a2)
//----- (47E9D0) --------------------------------------------------------
C_real_select_target(int who_chooses, int allowed_controller, int preferred_controller, target_zone_t zone, type_t required_type, type_t illegal_type, keyword_t required_abilities, keyword_t illegal_abilities, color_test_t required_color, color_test_t illegal_color, int extra, subtype_in_card_data_t required_subtype, int power_requirement, int toughness_requirement, target_special_t special, target_state_t required_state, target_state_t illegal_state, const char *prompt, int allow_cancel, target_t *ret_tgt)
//----- (47EF10) --------------------------------------------------------
sub_47EF10(int player, int card)
//----- (47EF50) --------------------------------------------------------
sub_47EF50(int a1, int a2)
//----- (47F050) --------------------------------------------------------
sub_47F050()
//----- (47F070) --------------------------------------------------------
clamp(int val, int lo, int hi)
//----- (47F090) --------------------------------------------------------
sub_47F090(LPCSTR lpFileName)
//----- (47F3C0) --------------------------------------------------------
read_Rarity_csv(LPCSTR lpFileName)
//----- (47F6D0) --------------------------------------------------------
sub_47F6D0(char a1)
//----- (47F720) --------------------------------------------------------
sub_47F720(char *a1, int a2)
//----- (47F7E0) --------------------------------------------------------
sub_47F7E0()
//----- (47F940) --------------------------------------------------------
sub_47F940(int a1)
//----- (47F960) --------------------------------------------------------
sub_47F960(HDC hdc, const RECT *lprect, signed int a3)
//----- (47F9C0) --------------------------------------------------------
sub_47F9C0(HDC hdc, RECT *a2, signed int a3, int a4, int a5)
//----- (47FCC0) --------------------------------------------------------
sub_47FCC0(HDC hdc, RECT *a2, signed int a3, int a4, int a5, int a6)
//----- (480190) --------------------------------------------------------
sub_480190(HDC hdc, RECT *a2, signed int a3, int a4, int a5)
//----- (480470) --------------------------------------------------------
sub_480470(HDC hdc, RECT *a2, signed int a3, int a4, int a5, int a6)
//----- (480880) --------------------------------------------------------
sub_480880(HDC hdc, RECT *a2, int a3, HANDLE h)
//----- (4809A0) --------------------------------------------------------
sub_4809A0(signed int a1, int a2)
//----- (480A00) --------------------------------------------------------
sub_480A00()
//----- (480AA0) --------------------------------------------------------
sub_480AA0(signed int a1, int a2)
//----- (480C50) --------------------------------------------------------
sub_480C50(signed int a1, int a2, int a3)
//----- (480DE0) --------------------------------------------------------
sub_480DE0(signed int a1, signed int a2, int a3, int a4, int a5, int a6)
//----- (481180) --------------------------------------------------------
sub_481180(int a1, int a2, int a3, int a4)
//----- (4812B0) --------------------------------------------------------
sub_4812B0(int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8, int a9)
//----- (481BE0) --------------------------------------------------------
sub_481BE0(int a1, int a2, int a3)
//----- (481DC0) --------------------------------------------------------
sub_481DC0(int a1)
//----- (4822E0) --------------------------------------------------------
sub_4822E0(int a1, int a2, int a3, signed int a4, int a5, signed int a6, int a7)
//----- (482490) --------------------------------------------------------
sub_482490(int a1)
//----- (482780) --------------------------------------------------------
sub_482780(int a1, int a2)
//----- (482910) --------------------------------------------------------
sub_482910(HWND hWndParent, int a2, int a3)
//----- (482940) --------------------------------------------------------
dlgproc_FoilPackScreen(HWND hwnd, WM_t msg, WPARAM wparam, int lparam)
//----- (483FA0) --------------------------------------------------------
sub_483FA0(HDC hDC, const RECT *lprcSrc, HDC hdcSrc, int a4, int wSrc, int hSrc)
//----- (484010) --------------------------------------------------------
sub_484010(HDC hdc, int a2, int a3, int a4, HANDLE h)
//----- (484200) --------------------------------------------------------
sub_484200(HDC hdc, const RECT *lprcSrc, int a3)
//----- (4842A0) --------------------------------------------------------
sub_4842A0(HDC hdc, int a2, const RECT *lprcSrc, const RECT *a4)
//----- (4843E0) --------------------------------------------------------
sub_4843E0(HGDIOBJ *a1, HGDIOBJ *a2, LPRECT lprc, RECT *a4, struct tagRECT *a5, struct tagRECT *a6, COLORREF *a7, COLORREF *a8, COLORREF *a9, HBRUSH *a10, HPEN *a11, HPEN *a12, COLORREF *a13)
//----- (4845F0) --------------------------------------------------------
sub_4845F0(void *a1, void *a2, HGDIOBJ ho, void *a4, void *a5)
//----- (484650) --------------------------------------------------------
sub_484650(LPRECT lprcDst, signed int a2, signed int a3, RECT *a4)
//----- (484920) --------------------------------------------------------
sub_484920(HWND hWnd, BOOL a2, signed int a3, int a4, HANDLE h, const RECT *lprcSrc, const RECT *a7)
//----- (484A60) --------------------------------------------------------
sub_484A60(signed int a1, HWND hWnd)
//----- (484C10) --------------------------------------------------------
sub_484C10(HWND hWndParent, LPARAM dwInitParam)
//----- (484C40) --------------------------------------------------------
dlgproc_LadderScreen(HWND hwnd, WM_t msg, WPARAM wparam, int lparam)
//----- (4873E0) --------------------------------------------------------
sub_4873E0(int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8, int a9, void *a10, int a11, int a12)
//----- (487540) --------------------------------------------------------
sub_487540(void *a1, void *a2, void *a3, void *a4, HGDIOBJ ho, void *a6)
//----- (4875C0) --------------------------------------------------------
sub_4875C0(HWND hDlg)
//----- (487910) --------------------------------------------------------
sub_487910(signed int a1, signed int a2, int a3)
//----- (487980) --------------------------------------------------------
sub_487980(HWND hWnd, HGDIOBJ h, int a3, int a4, int a5, LPRECT lprcDst, struct tagRECT *a7)
//----- (488010) --------------------------------------------------------
sub_488010(int a1, int a2, int a3, POINT pt, LPRECT lprcDst)
//----- (488220) --------------------------------------------------------
sub_488220(HWND hWndParent, LPARAM a2)
//----- (488280) --------------------------------------------------------
dlgproc_Rogue(HWND hwnd, WM_t msg, HDC wparam, int lparam)
//----- (488980) --------------------------------------------------------
sub_488980(int a1, int a2, int a3, int a4, int a5, void *a6, int a7)
//----- (488A50) --------------------------------------------------------
sub_488A50(void *a1, HGDIOBJ ho, void *a3, void *a4)
//----- (488AA0) --------------------------------------------------------
sub_488AA0(HWND hWndParent, rogue_t *a2, signed int a3)
//----- (488CA0) --------------------------------------------------------
sub_488CA0(int a1)
//----- (488EC0) --------------------------------------------------------
sub_488EC0(int a1, int a2, int a3)
//----- (489040) --------------------------------------------------------
sub_489040()
//----- (489530) --------------------------------------------------------
sub_489530()
//----- (489760) --------------------------------------------------------
register_CardClass(const CHAR *a1)
//----- (489D10) --------------------------------------------------------
destroy_CardClass()
//----- (489D60) --------------------------------------------------------
wndproc_CardClass(HWND hwnd, WM_t msg, WPARAM wparam, LPARAM lparam)
//----- (48C700) --------------------------------------------------------
sub_48C700(HWND hWnd)
//----- (48C7B0) --------------------------------------------------------
sub_48C7B0(HWND hWnd)
//----- (48C950) --------------------------------------------------------
sub_48C950(HWND hWnd)
//----- (48C9D0) --------------------------------------------------------
C_card_instances_should_be_displayed_identically(card_instance_t *inst1, card_instance_t *inst2)
//----- (48CB68) --------------------------------------------------------
corpse_of_original_card_instances_have_same_cached_abilities<eax>(int a1<edx>, int a2<ebp>)
//----- (48CB80) --------------------------------------------------------
C_get_special_counters_name(char *Dest, int csvid, int num)
//----- (48CF40) --------------------------------------------------------
sub_48CF40(char *Dest, unsigned int a2, int a3)
//----- (48CFB0) --------------------------------------------------------
sub_48CFB0(HDC hdc, int a2, int a3, int a4)
//----- (48D0B0) --------------------------------------------------------
TENTATIVE_does_this_hwnd_correspond_to_p_c(HWND hWnd, target_t *p_c)
//----- (48D110) --------------------------------------------------------
sub_48D110(HWND hWnd, int a2)
//----- (48D160) --------------------------------------------------------
TENTATIVE_get_displayed_csvid_of_cardclass_hwnd(HWND hWnd)
//----- (48D1C0) --------------------------------------------------------
sub_48D1C0(HWND hWnd)
//----- (48D1E0) --------------------------------------------------------
sub_48D1E0(int a1)
//----- (48D3E0) --------------------------------------------------------
TENTATIVE_savegame(phase_t a1)
//----- (48D500) --------------------------------------------------------
sub_48D500(LPCSTR lpNewFileName)
//----- (48D5B0) --------------------------------------------------------
register_AttackClass(const CHAR *a1)
//----- (48D9E0) --------------------------------------------------------
destroy_AttackClass()
//----- (48DBC0) --------------------------------------------------------
wndproc_AttackClass(HWND hWndParent, WM_t msg, HDC wparam, LPARAM lparam)
//----- (4910E0) --------------------------------------------------------
sub_4910E0(int a1, int a2, int a3)
//----- (4911B0) --------------------------------------------------------
sub_4911B0(HWND hDlg)
//----- (491BA0) --------------------------------------------------------
sub_491BA0(unsigned int player, unsigned int card)
//----- (491BF0) --------------------------------------------------------
wndproc_AttackSwordShield(HWND hWndParent, int msg, int wParam, LPARAM lParam)
//----- (4925B0) --------------------------------------------------------
sub_4925B0(HWND hWnd)
//----- (492620) --------------------------------------------------------
wndproc_AttackMinimized(HWND hWndParent, int msg, int deststr, LPARAM lParam)
//----- (492940) --------------------------------------------------------
TENTATIVE_find_csvid_hwnd_controllingplayer_of_pc(HWND hWnd, target_t *tgt, int *ret_csvid, HWND *ret_hwnd, int *TENTATIVE_ret_controlling_player)
//----- (492BB0) --------------------------------------------------------
sub_492BB0(HWND hWnd, int a2)
//----- (492D40) --------------------------------------------------------
read_deckfile(const char *filename, csvid_and_numcards *a2, int always_one, int always_negone)
//----- (4931F0) --------------------------------------------------------
assert(int condition, const char *file, int line, const char *fmt, ...)
//----- (493330) --------------------------------------------------------
register_MainClass(const CHAR *a1)
//----- (4933C0) --------------------------------------------------------
wndproc_MainClass(HWND hwnd, WM_t msg, WPARAM wparam, LPARAM lparam)
//----- (4946C0) --------------------------------------------------------
start_duel_thread(int a1)
//----- (494710) --------------------------------------------------------
create_windows(HWND hWndParent)
//----- (494D20) --------------------------------------------------------
sub_494D20(HWND hWnd, int a2)
//----- (495BE0) --------------------------------------------------------
sub_495BE0(int a1)
//----- (495DC0) --------------------------------------------------------
sub_495DC0(int arg0, int a2, int a3)
//----- (4961F0) --------------------------------------------------------
sub_4961F0()
//----- (496230) --------------------------------------------------------
WILDGUESS_dlgproc_fake_dialog_to_popup_StillThinking_window(HWND hwnd, WM_t msg, HDC wparam, LPARAM lparam)
//----- (4964D0) --------------------------------------------------------
sub_4964D0(HWND hDlg, int nIDDlgItem, int a3, int a4, int a5)
//----- (4967F0) --------------------------------------------------------
sub_4967F0(HWND hDlg, int nIDDlgItem, int a3, int a4)
//----- (496A70) --------------------------------------------------------
is_basic_land_by_csvid(int a1)
//----- (496AB0) --------------------------------------------------------
is_csvid_restricted(int csvid)
//----- (496B00) --------------------------------------------------------
is_csvid_restrictionflag_unknown4(int a1)
//----- (496B50) --------------------------------------------------------
is_csvid_ante(int a1)
//----- (496BA0) --------------------------------------------------------
sub_496BA0(const char *Filename, int a2, int a3, int a4)
//----- (496EF0) --------------------------------------------------------
sub_496EF0(const char *Filename, int a2)
//----- (4970F0) --------------------------------------------------------
SellPrice()
//----- (497100) --------------------------------------------------------
register_LifeClass(const CHAR *a1)
//----- (497270) --------------------------------------------------------
destroy_LifeClass()
//----- (497300) --------------------------------------------------------
wndproc_LifeClass(HWND hwnd, WM_t msg, WPARAM wparam, LPARAM lparam)
//----- (497FE0) --------------------------------------------------------
sub_497FE0(int a1)
//----- (498020) --------------------------------------------------------
register_IconButtonClass(const CHAR *a1)
//----- (4980A0) --------------------------------------------------------
wndproc_IconButtonClass(HWND hwnd, WM_t msg, WPARAM wparam, LPARAM lparam)
//----- (4983C0) --------------------------------------------------------
TENTATIVE_ai_plays_land(int player)
//----- (498D90) --------------------------------------------------------
C_backup_data_for_ai_0()
//----- (498DF0) --------------------------------------------------------
C_restore_data_for_ai_0()
//----- (498E30) --------------------------------------------------------
C_backup_data_for_ai()
//----- (498E70) --------------------------------------------------------
C_restore_data_for_ai()
//----- (498EB0) --------------------------------------------------------
sub_498EB0()
//----- (498ED0) --------------------------------------------------------
TENTATIVE_restore_data_for_ai_frontend_0()
//----- (498F20) --------------------------------------------------------
sub_498F20()
//----- (498FD0) --------------------------------------------------------
sub_498FD0(int a1)
//----- (499010) --------------------------------------------------------
sub_499010(int a1)
//----- (499050) --------------------------------------------------------
sub_499050()
//----- (4990B0) --------------------------------------------------------
sub_4990B0()
//----- (499130) --------------------------------------------------------
sub_499130()
//----- (499140) --------------------------------------------------------
sub_499140()
//----- (499160) --------------------------------------------------------
ai_opinion_of_gamestate(int player)
//----- (499740) --------------------------------------------------------
ai_opinion_of_gamestate_continued(int player, int overall_opinion)
//----- (499DF0) --------------------------------------------------------
landwalks_controlled(int *ret_player1_landtypes, int *ret_player0_landtypes)
//----- (499E50) --------------------------------------------------------
register_TerritoryClass(const CHAR *a1)
//----- (49A150) --------------------------------------------------------
destroy_TerritoryClass()
//----- (49A1A0) --------------------------------------------------------
wndproc_TerritoryClass(HWND hwnd, WM_t msg, WPARAM wparam, LPARAM lparam)
//----- (49BD60) --------------------------------------------------------
sub_49BD60(HWND hWnd)
//----- (49BE50) --------------------------------------------------------
TENTATIVE_rearrange_cards(HWND hWnd)
//----- (49C040) --------------------------------------------------------
sub_49C040(HWND hWnd)
//----- (49C140) --------------------------------------------------------
TENTATIVE_rearrange_one_card(HWND hWnd, target_t *p_c, int a3, int *x, int *y, int a6)
//----- (49C5E0) --------------------------------------------------------
sub_49C5E0(HWND hWnd, HWND a2)
//----- (49C6C0) --------------------------------------------------------
TENTATIVE_find_csvid_hwnd_of_pc(HWND hWnd, target_t *p_c, int *ret_csvid, HWND *ret_hwnd)
//----- (49C7B0) --------------------------------------------------------
sub_49C7B0(HWND hWnd, target_t *a2)
//----- (49C840) --------------------------------------------------------
sub_49C840(HWND hWnd, int a2)
//----- (49C8D0) --------------------------------------------------------
sub_49C8D0(int a3, int a4, int a5, int a6, int a7, const char *a8, int a9)
//----- (49CC40) --------------------------------------------------------
sub_49CC40(HWND hWndParent, int a2)
//----- (49CD60) --------------------------------------------------------
sub_49CD60(int a1)
//----- (49CD90) --------------------------------------------------------
sub_49CD90(int a1)
//----- (49CDC0) --------------------------------------------------------
sub_49CDC0(int a1, char a2)
//----- (49CFD0) --------------------------------------------------------
TENTATIVE_showmovie(HWND hWndParent)
//----- (49D0D0) --------------------------------------------------------
sub_49D0D0()
//----- (49D140) --------------------------------------------------------
sub_49D140()
//----- (49D1A0) --------------------------------------------------------
sub_49D1A0(int a1, MCIERROR mcierr)
//----- (49D200) --------------------------------------------------------
wndproc_MovieClass(HWND hwnd, WM_t msg, WPARAM wparam, LPARAM lparam)
//----- (49D240) --------------------------------------------------------
declare_mana_available_hex(int player, color_test_t colors, int amount)
//----- (49D2E0) --------------------------------------------------------
undeclare_mana_available_hex(int player, color_test_t color, int amt)
//----- (49D380) --------------------------------------------------------
sunglasses_of_urza_effect(int player, color_t from_color, color_t to_color)
//----- (49D430) --------------------------------------------------------
declare_mana_available(int player, color_t color, int amt)
//----- (49D460) --------------------------------------------------------
undeclare_mana_available(int player, color_t color, int amt)
//----- (49D490) --------------------------------------------------------
produce_mana(int player, color_t color, int amt)
//----- (49D4C0) --------------------------------------------------------
unproduce_mana(int player, int card, int amt)
//----- (49D4F0) --------------------------------------------------------
undeclare_mana_available_and_produce_it(int player, color_t color, int amt)
//----- (49D510) --------------------------------------------------------
has_mana(int player, color_t color, int amount)
//----- (49D710) --------------------------------------------------------
has_mana_w_global_cost_mod(int player, int card, color_t color, int amt)
//----- (49D7D0) --------------------------------------------------------
register_TellUserClass(const CHAR *a1)
//----- (49D9F0) --------------------------------------------------------
destroy_TellUserClass()
//----- (49DB10) --------------------------------------------------------
wndproc_TellUserClass(HWND hWndTo, UINT Msg, LPSTR wParam, LPARAM lParam)
//----- (49E490) --------------------------------------------------------
sub_49E490(HWND hWnd, LPCSTR str, int a3)
//----- (49E800) --------------------------------------------------------
sub_49E800(HWND hWnd)
//----- (49E8A0) --------------------------------------------------------
sub_49E8A0(HWND hWndTo, HDC hdc, RECT *a3)
//----- (49E9E0) --------------------------------------------------------
sub_49E9E0(int a1, int a2, int a3)
//----- (49EA40) --------------------------------------------------------
write_and_return_false_on_failure(int fd, const void *buf, size_t count)
//----- (49EA70) --------------------------------------------------------
save_or_load_ver1()
//----- (49F500) --------------------------------------------------------
save_or_load_data(void *buf, size_t count)
//----- (49F560) --------------------------------------------------------
save_gametype0(const char *path)
//----- (49F600) --------------------------------------------------------
load_gametype0(const char *path)
//----- (49F740) --------------------------------------------------------
save_or_load_ver2()
//----- (4A02C0) --------------------------------------------------------
UNKNOWN_called_from_save_and_load_ver2_functions()
//----- (4A0340) --------------------------------------------------------
save_soloduel(const char *path)
//----- (4A03D0) --------------------------------------------------------
load_soloduel(const char *path)
//----- (4A04B0) --------------------------------------------------------
save_gauntlet(const char *path)
//----- (4A05A0) --------------------------------------------------------
load_gauntlet(const char *path)
//----- (4A06F0) --------------------------------------------------------
save_sealeddeck(const char *path)
//----- (4A0780) --------------------------------------------------------
load_sealeddeck(const char *path)
//----- (4A0850) --------------------------------------------------------
sub_4A0850()
//----- (4A09A0) --------------------------------------------------------
create_legacy_effect(int player, int card, int legacy_iid, int target_player, int target_card)
//----- (4A0AE0) --------------------------------------------------------
fx_powerup_903(int player, int card, event_t event)
//----- (4A0BB0) --------------------------------------------------------
fx_unblockable_903(int player, int card, event_t event)
//----- (4A0C10) --------------------------------------------------------
C_fx_noattack_903(int player, int card, event_t event)
//----- (4A0C70) --------------------------------------------------------
fx_add_ability_903(int player, int card, event_t event)
//----- (4A0E70) --------------------------------------------------------
fx_take_ability_903(int player, int card, event_t event)
//----- (4A0F00) --------------------------------------------------------
C_fx_damage_901(int player, int card, event_t event)
//----- (4A12A0) --------------------------------------------------------
C_fx_asp_sting_902(int player, int card, event_t event)
//----- (4A1420) --------------------------------------------------------
fx_stoning_903(int player, int card, event_t event)
//----- (4A14B0) --------------------------------------------------------
fx_time_elemental_903(int player, int card, event_t event)
//----- (4A1570) --------------------------------------------------------
fx_marsh_gas_903(int player, int card, event_t event)
//----- (4A1660) --------------------------------------------------------
fx_fog_903(int player, int card, event_t event)
//----- (4A16F0) --------------------------------------------------------
fx_channel_902(int player, int card, event_t event)
//----- (4A1890) --------------------------------------------------------
fx_generic_903(int player, int card, event_t event)
//----- (4A1BB0) --------------------------------------------------------
C_fx_asterisk_903(int player, int card, event_t event)
//----- (4A1FD0) --------------------------------------------------------
fx_sorceress_queen_903(int player, int card, event_t event)
//----- (4A20F0) --------------------------------------------------------
fx_animate_artifact_lose_abilities_902(int player, int card, event_t event)
//----- (4A21F0) --------------------------------------------------------
UNKNOWN_helper_animate_artifact_lose_abilities(int player, int card, int a4, int a5, int a6)
//----- (4A2270) --------------------------------------------------------
fx_disintegrate_903(int player, int card, event_t event)
//----- (4A22D0) --------------------------------------------------------
C_fx_sirens_call_903(int player, int card, event_t event)
//----- (4A23A0) --------------------------------------------------------
UNKNOWN_sirens_call_helper(int player, int card, int internal_card_id)
//----- (4A23F0) --------------------------------------------------------
fx_maze_of_ith_903(int player, int card, event_t event)
//----- (4A24E0) --------------------------------------------------------
fx_animate_artifact_keep_abilities_903(int player, int card, event_t event)
//----- (4A2620) --------------------------------------------------------
fx_damage_legacy_902(int player, int card, event_t event)
//----- (4A27B0) --------------------------------------------------------
fx_activation_906(int player, int card, event_t event)
//----- (4A2820) --------------------------------------------------------
fx_draw_card_904(int player, int card, event_t event)
//----- (4A28B0) --------------------------------------------------------
C_card_fork(int player, int card, event_t event)
//----- (4A2A30) --------------------------------------------------------
card_mana_short(int player, int card, event_t event)
//----- (4A2B40) --------------------------------------------------------
UNKNOWN_mana_short_helper(int player, int card, int a3)
//----- (4A2BF0) --------------------------------------------------------
C_card_sirens_call(int player, int card, event_t event)
//----- (4A2C50) --------------------------------------------------------
card_ancestral_recall(int player, int card, event_t event)
//----- (4A2D40) --------------------------------------------------------
C_card_simulacrum(int player, int card, event_t event)
//----- (4A2E60) --------------------------------------------------------
card_shatter(int player, int card, event_t event)
//----- (4A2F10) --------------------------------------------------------
card_disenchant(int player, int card, event_t event)
//----- (4A2FC0) --------------------------------------------------------
C_card_twiddle(int player, int card, event_t event)
//----- (4A3170) --------------------------------------------------------
C_tap_card(int player, int card)
//----- (4A31C0) --------------------------------------------------------
card_tunnel(int player, int card, event_t event)
//----- (4A3260) --------------------------------------------------------
card_marsh_gas(int player, int card, event_t event)
//----- (4A3310) --------------------------------------------------------
C_card_howl_from_beyond(int player, int card, event_t event)
//----- (4A3450) --------------------------------------------------------
card_berserk(int player, int card, event_t event)
//----- (4A3550) --------------------------------------------------------
card_righteousness(int player, int card, event_t event)
//----- (4A3610) --------------------------------------------------------
card_blood_lust(int player, int card, event_t event)
//----- (4A3700) --------------------------------------------------------
card_fog(int player, int card, event_t event)
//----- (4A3750) --------------------------------------------------------
card_swords_to_plowshares(int player, int card, event_t event)
//----- (4A3810) --------------------------------------------------------
C_card_death_ward(int player, int card, event_t event)
//----- (4A3910) --------------------------------------------------------
card_hurkyls_recall(int player, int card, event_t event)
//----- (4A3B50) --------------------------------------------------------
card_jump(int player, int card, event_t event)
//----- (4A3C50) --------------------------------------------------------
card_morale(int player, int card, event_t event)
//----- (4A3CF0) --------------------------------------------------------
UNKNOWN_morale_helper(int player, int card, event_t event)
//----- (4A3D50) --------------------------------------------------------
card_piety(int player, int card, event_t event)
//----- (4A3E00) --------------------------------------------------------
UNKNOWN_piety_helper(int player, int card, event_t event)
//----- (4A3E70) --------------------------------------------------------
card_terror(int player, int card, event_t event)
//----- (4A3F20) --------------------------------------------------------
card_lightning_bolt(int player, int card, event_t event)
//----- (4A3FB0) --------------------------------------------------------
C_card_crumble(int player, int card, event_t event)
//----- (4A4080) --------------------------------------------------------
card_giant_growth(int player, int card, event_t event)
//----- (4A41C0) --------------------------------------------------------
card_unsummon(int player, int card, event_t event)
//----- (4A4270) --------------------------------------------------------
card_sandstorm(int player, int card, event_t event)
//----- (4A42D0) --------------------------------------------------------
UNKNOWN_sandstorm_helper(int a1, int a2, int player, int card)
//----- (4A4310) --------------------------------------------------------
card_purelace(int player, int card, event_t event)
//----- (4A4560) --------------------------------------------------------
UNKNOWN_magical_hack_helper(int player, int card, int a3, char a4)
//----- (4A45B0) --------------------------------------------------------
card_magical_hack(int player, int card, event_t event)
//----- (4A49B0) --------------------------------------------------------
UNKNOWN_sleight_of_mind_helper(int player, int card, int a3, char a4)
//----- (4A49F0) --------------------------------------------------------
card_sleight_of_mind(int player, int card, event_t event)
//----- (4A4DE0) --------------------------------------------------------
card_blue_elemental_blast(int player, int card, event_t event)
//----- (4A4FE0) --------------------------------------------------------
card_counterspell(int player, int card, event_t event)
//----- (4A5150) --------------------------------------------------------
card_power_sink(int player, int card, event_t event)
//----- (4A54A0) --------------------------------------------------------
card_red_elemental_blast(int player, int card, event_t event)
//----- (4A56B0) --------------------------------------------------------
card_dark_ritual(int player, int card, event_t event)
//----- (4A5730) --------------------------------------------------------
card_alabaster_potion(int player, int card, event_t event)
//----- (4A5750) --------------------------------------------------------
C_card_healing_salve(int player, int card, event_t event)
//----- (4A5770) --------------------------------------------------------
gain_life_or_prevent_damage(int player, int card, event_t event, int amount)
//----- (4A5B90) --------------------------------------------------------
C_card_reverse_damage(int player, int card, event_t event)
//----- (4A6010) --------------------------------------------------------
card_eye_for_an_eye(int player, int card, event_t event)
//----- (4A61A0) --------------------------------------------------------
card_inferno(int player, int card, event_t event)
//----- (4A62B0) --------------------------------------------------------
card_fissure(int player, int card, event_t event)
//----- (4A6360) --------------------------------------------------------
C_card_orcish_catapult(int player, int card, event_t event)
//----- (4A6500) --------------------------------------------------------
create_a_card_type(int a1)
//----- (4A6580) --------------------------------------------------------
sub_4A6580(int a1)
//----- (4A65A0) --------------------------------------------------------
get_hacked_color(int player, int card, int a3)
//----- (4A65D0) --------------------------------------------------------
get_sleighted_color(int player, int card, color_t color)
//----- (4A65F0) --------------------------------------------------------
bounce_permanent(int player, int card)
//----- (4A66C0) --------------------------------------------------------
damage_creature(int tgt_player, int tgt_card, int32_t amt, int src_player, int src_card)
//----- (4A68A0) --------------------------------------------------------
damage_player(int tgt_player, int amt, int src_player, int src_card)
//----- (4A68C0) --------------------------------------------------------
sub_4A68C0(int a1, signed int a2)
//----- (4A69A0) --------------------------------------------------------
sub_4A69A0(int a1, int a2)
//----- (4A6A20) --------------------------------------------------------
sub_4A6A20(int a1, int a2)
//----- (4A6AD0) --------------------------------------------------------
sub_4A6AD0()
//----- (4A6B90) --------------------------------------------------------
sub_4A6B90()
//----- (4A6BC0) --------------------------------------------------------
TENTATIVE_send_network_result(int a1, signed int a2)
//----- (4A6F50) --------------------------------------------------------
sub_4A6F50(int a1, char a2)
//----- (4A70C0) --------------------------------------------------------
TENTATIVE_wait_for_network_result(int a1, signed int a2)
//----- (4A78A0) --------------------------------------------------------
sub_4A78A0()
//----- (4A78E0) --------------------------------------------------------
sub_4A78E0()
//----- (4A7940) --------------------------------------------------------
sub_4A7940(int a1, int a2, int a3)
//----- (4A7A80) --------------------------------------------------------
sub_4A7A80(int a1)
//----- (4A7AD0) --------------------------------------------------------
sub_4A7AD0(int a1)
//----- (4A7B30) --------------------------------------------------------
AddCardToCLPacket(int a1)
//----- (4A7BD0) --------------------------------------------------------
GetCardFromCLPacket(int a1)
//----- (4A7C50) --------------------------------------------------------
sub_4A7C50(const char *str)
//----- (4A7D80) --------------------------------------------------------
append_to_trace_txt(const char *str)
//----- (4A7DD0) --------------------------------------------------------
sub_4A7DD0(int a1)
//----- (4A7EB0) --------------------------------------------------------
C_card_arena(int player, int card, event_t event)
//----- (4A8520) --------------------------------------------------------
card_magus_of_the_bazaar(int player, int card, event_t event)
//----- (4A86B0) --------------------------------------------------------
C_card_city_of_brass(int player, int card, event_t event)
//----- (4A8970) --------------------------------------------------------
C_card_desert(int player, int card, event_t event)
//----- (4A8B40) --------------------------------------------------------
C_card_diamond_valley(int player, int card, event_t event)
//----- (4A8D40) --------------------------------------------------------
C_card_elephant_graveyard(int player, int card, event_t event)
//----- (4A8F70) --------------------------------------------------------
C_card_island_of_wak_wak(int player, int card, event_t event)
//----- (4A90B0) --------------------------------------------------------
C_card_urzas_mine_card_urzas_power_plant(int player, int card, event_t event)
//----- (4A91E0) --------------------------------------------------------
C_card_urzas_tower(int player, int card, event_t event)
//----- (4A9310) --------------------------------------------------------
tap_for_multicolor_mana(int player, int card, event_t event, int colors)
//----- (4A95E0) --------------------------------------------------------
card_badlands(int player, int card, event_t event)
//----- (4A9630) --------------------------------------------------------
card_bayou(int player, int card, event_t event)
//----- (4A9650) --------------------------------------------------------
card_plateau(int player, int card, event_t event)
//----- (4A9670) --------------------------------------------------------
card_savannah(int player, int card, event_t event)
//----- (4A9690) --------------------------------------------------------
card_scrubland(int player, int card, event_t event)
//----- (4A96B0) --------------------------------------------------------
card_taiga(int player, int card, event_t event)
//----- (4A96D0) --------------------------------------------------------
card_tropical_island(int player, int card, event_t event)
//----- (4A96F0) --------------------------------------------------------
card_tundra(int player, int card, event_t event)
//----- (4A9710) --------------------------------------------------------
card_underground_sea(int player, int card, event_t event)
//----- (4A9730) --------------------------------------------------------
card_volcanic_island(int player, int card, event_t event)
//----- (4A9750) --------------------------------------------------------
choose_a_card(int a1, int a2, int a3)
//----- (4A97E0) --------------------------------------------------------
dlgproc_choose_a_card(HWND hwnd, WM_t msg, WPARAM wparam, LPARAM lparam)
//----- (4A9EB0) --------------------------------------------------------
UNKNOWN_dlgproc_choose_a_card_helper(HWND hWnd, int color, char type)
//----- (4AA090) --------------------------------------------------------
dlgproc_do_dialog(HWND hwnd, WM_t msg, WPARAM wparam, dialog_args_t *lparam)
//----- (4AABF0) --------------------------------------------------------
register_BigCardCardClass(const CHAR *a1)
//----- (4AAC80) --------------------------------------------------------
wndproc_BigCardCardClass(HWND hwnd, WM_t msg, WPARAM wparam, LPARAM lparam)
//----- (4AAD90) --------------------------------------------------------
register_BigCardChoiceClass(const CHAR *a1)
//----- (4AAE60) --------------------------------------------------------
destroy_BigCardChoiceClass()
//----- (4AAE90) --------------------------------------------------------
wndproc_BigCardChoiceClass(HWND hwnd, WM_t msg, WPARAM wparam, LPARAM lparam)
//----- (4ABEB0) --------------------------------------------------------
sub_4ABEB0(HWND hWnd, __int32 a2)
//----- (4ABF40) --------------------------------------------------------
sub_4ABF40()
//----- (4AC030) --------------------------------------------------------
sub_4AC030()
//----- (4AC050) --------------------------------------------------------
sub_4AC050()
//----- (4AC070) --------------------------------------------------------
sub_4AC070(int a1)
//----- (4AC1C0) --------------------------------------------------------
TENTATIVE_save_soloduel_via_getsavefilename(HWND a1)
//----- (4AC210) --------------------------------------------------------
TENTATIVE_save_gauntlet_via_getsavefilename(HWND a1)
//----- (4AC260) --------------------------------------------------------
TENTATIVE_save_sealeddeck_via_getsavefilename(HWND a1)
//----- (4AC2B0) --------------------------------------------------------
sub_4AC2B0()
//----- (4AC380) --------------------------------------------------------
sub_4AC380(int a1, signed int a2, int a3, int a4, int a5, int a6, void (*a7)(void), int a8)
//----- (4AD200) --------------------------------------------------------
sub_4AD200(int a1, int a2, HWND hWnd)
//----- (4AD5C0) --------------------------------------------------------
sub_4AD5C0(int a1, HWND hWndParent, int a3, rogue_t *srcstr, int a5, int a6)
//----- (4AD9A0) --------------------------------------------------------
sub_4AD9A0(int a1)
//----- (4ADA10) --------------------------------------------------------
sub_4ADA10()
//----- (4ADF90) --------------------------------------------------------
sub_4ADF90()
//----- (4AE0D0) --------------------------------------------------------
sub_4AE0D0()
//----- (4AE150) --------------------------------------------------------
sub_4AE150(const char *Filename, const char *a2)
//----- (4AE470) --------------------------------------------------------
sub_4AE470(HWND hWndParent, LPARAM a2, int a3, LPCSTR a4, int a5, int a6, LPCSTR a7, int a8, int a9)
//----- (4AE610) --------------------------------------------------------
dlgproc_Versus(HWND hwnd, WM_t msg, HDC wparam, int lparam)
//----- (4AEF80) --------------------------------------------------------
sub_4AEF80(int a1, int a2, int a3, int a4, int a5, void *a6, int a7)
//----- (4AF050) --------------------------------------------------------
sub_4AF050(void *a1, HGDIOBJ ho, void *a3, void *a4)
//----- (4AF0A0) --------------------------------------------------------
ai_opinion_of_game_state_after_this_is_obliterated_and_event_should_ai_play_dispatched(int player, int card)
//----- (4AF110) --------------------------------------------------------
UNKNOWN_ai_checks_for_specific_cards(int player)
//----- (4AF6F0) --------------------------------------------------------
TENTATIVE_ai_speculate_on_combat(int player)
//----- (4B02C0) --------------------------------------------------------
WILDGUESS_ai_choose_attackers(int player)
//----- (4B18E0) --------------------------------------------------------
C_ai_assign_blockers(int a3)
//----- (4B1A50) --------------------------------------------------------
sub_4B1A50(int a1, int a2)
//----- (4B1B90) --------------------------------------------------------
sub_4B1B90()
//----- (4B23C0) --------------------------------------------------------
sub_4B23C0(int allowed_controller)
//----- (4B3850) --------------------------------------------------------
C_process_multiblock(int player, int card, uint16_t pow)
//----- (4B39E0) --------------------------------------------------------
deals_damage_this_subphase(int first_strike, keyword_t keywords)
//----- (4B3A20) --------------------------------------------------------
sub_4B3A20(int player)
//----- (4B3AA0) --------------------------------------------------------
clear_dword_607DB8()
//----- (4B3AC0) --------------------------------------------------------
sub_4B3AC0(int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8)
//----- (4B3E90) --------------------------------------------------------
sub_4B3E90(int player, int a2, int a3, int *a4, int a5, int a6, int a7, int a8)
//----- (4B41F0) --------------------------------------------------------
check_pump()
//----- (4B42F0) --------------------------------------------------------
set_status_unknown20000(int player, int card, int on_or_off)
//----- (4B4320) --------------------------------------------------------
set_status_unknown20000_0(int player, int card, int on_or_off)
//----- (4B4350) --------------------------------------------------------
sub_4B4350(int player, int card)
//----- (4B4490) --------------------------------------------------------
TENTATIVE_ai_opinion_of_combat_effectiveness(int player, int card)
//----- (4B4590) --------------------------------------------------------
is_this_enchanted_by_gaseous_form(int player, int card)
//----- (4B4600) --------------------------------------------------------
rfalse_0()
//----- (4B4610) --------------------------------------------------------
C_card_island_sanctuary(int player, int card, event_t event)
//----- (4B48C0) --------------------------------------------------------
card_kormus_bell(int player, int card, event_t event)
//----- (4B4A80) --------------------------------------------------------
card_living_lands(int player, int card, event_t event)
//----- (4B4C30) --------------------------------------------------------
is_this_an_animate_land_legacy_targeting_affected_card(int a1, int a2, int player, int card, int a5)
//----- (4B4C80) --------------------------------------------------------
change_type_unanimate_land(int a3, int a4, int player, int card, int a7)
//----- (4B4CD0) --------------------------------------------------------
card_sylvan_library(int player, int card, event_t event)
//----- (4B50D0) --------------------------------------------------------
C_card_land_tax(int player, int card, event_t event)
//----- (4B56E0) --------------------------------------------------------
C_card_kismet(int player, int card, event_t event)
//----- (4B5780) --------------------------------------------------------
C_card_animate_dead(int player, int card, event_t event)
//----- (4B5D90) --------------------------------------------------------
put_into_play(int player, int card)
//----- (4B5ED0) --------------------------------------------------------
card_animate_artifact(int player, int card, event_t event)
//----- (4B6030) --------------------------------------------------------
C_card_titanias_song(int player, int card, event_t event)
//----- (4B6340) --------------------------------------------------------
UNKNOWN_titanias_song_helper(int player, int card, int a3)
//----- (4B6370) --------------------------------------------------------
UNKNOWN_titanias_song_helper2(int player, int card, int a3)
//----- (4B63A0) --------------------------------------------------------
C_card_animate_wall(int player, int card, event_t event)
//----- (4B64C0) --------------------------------------------------------
card_control_magic(int player, int card, event_t event)
//----- (4B6510) --------------------------------------------------------
card_steal_artifact(int player, int card, event_t event)
//----- (4B65B0) --------------------------------------------------------
midend_of_control_magic(int player, int card, event_t event, type_t reqtype)
//----- (4B67E0) --------------------------------------------------------
UNKNOWN_fx_control_903_helper(int player, int card, int iid)
//----- (4B6860) --------------------------------------------------------
gain_control(int player, int card)
//----- (4B6A10) --------------------------------------------------------
exchange_control(int player1, int card1, int player2, int card2)
//----- (4B6C90) --------------------------------------------------------
card_feedback(int player, int card, event_t event)
//----- (4B6E90) --------------------------------------------------------
card_brainwash(int player, int card, event_t event)
//----- (4B7110) --------------------------------------------------------
check_attack_legality_if_enchanting_affected_card(int player, int card, int iid)
//----- (4B7180) --------------------------------------------------------
C_card_spirit_shackle(int player, int card, event_t event)
//----- (4B72D0) --------------------------------------------------------
C_spirit_shackle_helper(int player, int card)
//----- (4B7310) --------------------------------------------------------
card_relic_bind(int player, int card, event_t event)
//----- (4B7560) --------------------------------------------------------
card_power_leak(int player, int card, event_t event)
//----- (4B7880) --------------------------------------------------------
card_energy_flux(int player, int card, event_t event)
//----- (4B7960) --------------------------------------------------------
card_erosion(int player, int card, event_t event)
//----- (4B7CD0) --------------------------------------------------------
card_cursed_land(int player, int card, event_t event)
//----- (4B7EC0) --------------------------------------------------------
card_karma(int player, int card, event_t event)
//----- (4B8110) --------------------------------------------------------
card_evil_presence(int player, int card, event_t event)
//----- (4B8280) --------------------------------------------------------
C_card_living_artifact(int player, int card, event_t event)
//----- (4B8590) --------------------------------------------------------
card_blight(int player, int card, event_t event)
//----- (4B8720) --------------------------------------------------------
card_psychic_venom(int player, int card, event_t event)
//----- (4B8880) --------------------------------------------------------
card_manabarbs(int player, int card, event_t event)
//----- (4B8940) --------------------------------------------------------
C_card_mana_flare(int player, int card, event_t event)
//----- (4B8AC0) --------------------------------------------------------
C_card_lifetap(int player, int card, event_t event)
//----- (4B8B90) --------------------------------------------------------
C_card_fortified_area(int player, int card, event_t event)
//----- (4B8C30) --------------------------------------------------------
card_sunken_city(int player, int card, event_t event)
//----- (4B8D50) --------------------------------------------------------
card_bad_moon(int player, int card, event_t event)
//----- (4B8DD0) --------------------------------------------------------
card_orcish_oriflamme(int player, int card, event_t event)
//----- (4B8E60) --------------------------------------------------------
card_crusade(int player, int card, event_t event)
//----- (4B8EE0) --------------------------------------------------------
card_aspect_of_wolf(int player, int card, event_t event)
//----- (4B9050) --------------------------------------------------------
C_card_lure(int player, int card, event_t event)
//----- (4B9220) --------------------------------------------------------
C_is_legal_block2(int blocking_player, int blocking_card, int blocked_player, int blocked_card)
//----- (4B9280) --------------------------------------------------------
C_card_spirit_link(int player, int card, event_t event)
//----- (4B9440) --------------------------------------------------------
card_creature_bond(int player, int card, event_t event)
//----- (4B95A0) --------------------------------------------------------
card_gaseous_form(int player, int card, event_t event)
//----- (4B96D0) --------------------------------------------------------
card_backfire(int player, int card, event_t event)
//----- (4B9910) --------------------------------------------------------
C_card_holy_armor(int player, int card, event_t event)
//----- (4B9D20) --------------------------------------------------------
C_card_blessing(int player, int card, event_t event)
//----- (4BA0F0) --------------------------------------------------------
card_firebreathing(int player, int card, event_t event)
//----- (4BA4C0) --------------------------------------------------------
C_card_fear(int player, int card, event_t event)
//----- (4BA5D0) --------------------------------------------------------
card_seeker(int player, int card, event_t event)
//----- (4BA700) --------------------------------------------------------
card_web(int player, int card, event_t event)
//----- (4BA8E0) --------------------------------------------------------
card_stasis(int player, int card, event_t event)
//----- (4BAB00) --------------------------------------------------------
card_magnetic_mountain(int player, int card, event_t event)
//----- (4BAC90) --------------------------------------------------------
C_card_paralyze(int player, int card, event_t event)
//----- (4BAEB0) --------------------------------------------------------
another_copy_already_attached(int player, int card, int iid)
//----- (4BAF40) --------------------------------------------------------
card_smoke(int player, int card, event_t event)
//----- (4BB1F0) --------------------------------------------------------
card_power_surge(int player, int card, event_t event)
//----- (4BB3C0) --------------------------------------------------------
card_burrowing(int player, int card, event_t event)
//----- (4BB4A0) --------------------------------------------------------
card_wanderlust(int player, int card, event_t event)
//----- (4BB6B0) --------------------------------------------------------
C_card_instill_energy(int player, int card, event_t event)
//----- (4BB9C0) --------------------------------------------------------
card_flood(int player, int card, event_t event)
//----- (4BBAC0) --------------------------------------------------------
card_greed(int player, int card, event_t event)
//----- (4BBB90) --------------------------------------------------------
C_card_pestilence(int player, int card, event_t event)
//----- (4BBEF0) --------------------------------------------------------
C_damage_1_if_is_a_creature(int a1, int a2, int a3, int a4, int a5)
//----- (4BBF30) --------------------------------------------------------
aura_ability(int player, int card, event_t event, keyword_t abil)
//----- (4BC000) --------------------------------------------------------
card_holy_strength(int player, int card, event_t event)
//----- (4BC050) --------------------------------------------------------
card_giant_strength(int player, int card, event_t event)
//----- (4BC0A0) --------------------------------------------------------
card_immolation(int player, int card, event_t event)
//----- (4BC0F0) --------------------------------------------------------
card_divine_transformation(int player, int card, event_t event)
//----- (4BC140) --------------------------------------------------------
card_unholy_strength(int player, int card, event_t event)
//----- (4BC190) --------------------------------------------------------
card_weakness(int player, int card, event_t event)
//----- (4BC1E0) --------------------------------------------------------
aura_pump(signed int player, int card, event_t event, int power_mod, int toughness_mod)
//----- (4BC340) --------------------------------------------------------
card_castle(int player, int card, event_t event)
//----- (4BC3A0) --------------------------------------------------------
card_black_ward(int player, int card, event_t event)
//----- (4BC3D0) --------------------------------------------------------
card_green_ward(int player, int card, event_t event)
//----- (4BC400) --------------------------------------------------------
card_blue_ward(int player, int card, event_t event)
//----- (4BC430) --------------------------------------------------------
card_red_ward(int player, int card, event_t event)
//----- (4BC460) --------------------------------------------------------
card_white_ward(int player, int card, event_t event)
//----- (4BC490) --------------------------------------------------------
common_ward(int player, int card, event_t event, color_t color)
//----- (4BC7A0) --------------------------------------------------------
C_card_unstable_mutation(int player, int card, event_t event)
//----- (4BCA00) --------------------------------------------------------
C_card_copy_artifact(int player, int card, event_t event)
//----- (4BCCE0) --------------------------------------------------------
card_warp_artifact(int player, int card, event_t event)
//----- (4BCF00) --------------------------------------------------------
card_power_struggle(int player, int card, event_t event)
//----- (4BD380) --------------------------------------------------------
C_card_necropolis_of_azar(int player, int card, event_t event)
//----- (4BD5E0) --------------------------------------------------------
card_regeneration(int player, int card, event_t event)
//----- (4BD800) --------------------------------------------------------
card_eternal_warrior(int player, int card, event_t event)
//----- (4BD960) --------------------------------------------------------
card_the_brute(int player, int card, event_t event)
//----- (4BDB90) --------------------------------------------------------
card_circle_of_protection_black(int player, int card, event_t event)
//----- (4BDBC0) --------------------------------------------------------
card_circle_of_protection_white(int player, int card, event_t event)
//----- (4BDBF0) --------------------------------------------------------
card_circle_of_protection_red(int player, int card, event_t event)
//----- (4BDC20) --------------------------------------------------------
card_circle_of_protection_blue(int player, int card, event_t event)
//----- (4BDC50) --------------------------------------------------------
card_circle_of_protection_green(int player, int card, event_t event)
//----- (4BDC80) --------------------------------------------------------
cop2(int player, int card, event_t event, color_t color)
//----- (4BDEE0) --------------------------------------------------------
card_circle_of_protection_artifacts(int player, int card, event_t event)
//----- (4BE140) --------------------------------------------------------
card_phantasmal_terrain(int player, int card, event_t event)
//----- (4BE300) --------------------------------------------------------
card_conversion(int player, int card, event_t event)
//----- (4BE490) --------------------------------------------------------
card_wild_growth(int player, int card, event_t event)
//----- (4BE620) --------------------------------------------------------
card_flight(int player, int card, event_t event)
//----- (4BE740) --------------------------------------------------------
card_lifeforce(int player, int card, event_t event)
//----- (4BE930) --------------------------------------------------------
card_deathgrip(int player, int card, event_t event)
//----- (4BEB20) --------------------------------------------------------
ai_choose_best_card_to_untap(int player, type_t type_to_untap)
//----- (4BEFA0) --------------------------------------------------------
register_PhaseDisplayClass(const CHAR *a1)
//----- (4BF0E0) --------------------------------------------------------
destroy_PhaseDisplayClass()
//----- (4BF140) --------------------------------------------------------
register_AttackPhaseDisplayClass(const CHAR *a1)
//----- (4BF200) --------------------------------------------------------
destroy_AttackPhaseDisplayClass()
//----- (4BF230) --------------------------------------------------------
wndproc_PhaseDisplayClass(HWND hwnd, WM_t msg, WPARAM wparam, LPARAM lparam)
//----- (4BFFA0) --------------------------------------------------------
sub_4BFFA0(POINT *a1, const RECT *lprcSrc, int a3, int a4)
//----- (4C0340) --------------------------------------------------------
sub_4C0340(LPRECT lprc, int a2, int a3, int xRight, int a5)
//----- (4C0520) --------------------------------------------------------
sub_4C0520(HDC hdc, int a2)
//----- (4C0760) --------------------------------------------------------
wndproc_AttackPhaseDisplayClass(HWND hwnd, WM_t msg, WPARAM wparam, LPARAM lparam)
//----- (4C15B0) --------------------------------------------------------
sub_4C15B0(POINT *a1, const RECT *lprcSrc, int a3)
//----- (4C1750) --------------------------------------------------------
sub_4C1750(LPRECT lprc, int a2, int xRight, int a4)
//----- (4C18E0) --------------------------------------------------------
sub_4C18E0(HDC hdc, int a2)
//----- (4C1B30) --------------------------------------------------------
sub_4C1B30(int *a1, int *a2, char *a3)
//----- (4C1DC0) --------------------------------------------------------
register_ManaSummaryClass(const CHAR *a1)
//----- (4C1F50) --------------------------------------------------------
destroy_ManaSummaryClass()
//----- (4C1FC0) --------------------------------------------------------
wndproc_ManaSummaryClass(HWND hwnd, WM_t msg, WPARAM wparam, LPARAM lparam)
//----- (4C30B0) --------------------------------------------------------
sub_4C30B0(__int32 lprc, char *hWnd, int a3)
//----- (4C3240) --------------------------------------------------------
card_prismatic_dragon(int player, int card, event_t event)
//----- (4C3560) --------------------------------------------------------
card_rainbow_knights(int player, int card, event_t event)
//----- (4C3B60) --------------------------------------------------------
card_sindbad(int player, int card, event_t event)
//----- (4C3C30) --------------------------------------------------------
card_tempest_efreet(int player, int card, event_t event)
//----- (4C3F60) --------------------------------------------------------
card_xenic_poltergeist(int player, int card, event_t event)
//----- (4C4090) --------------------------------------------------------
card_uncle_istvan(int player, int card, event_t event)
//----- (4C4100) --------------------------------------------------------
card_ironclaw_orcs(int player, int card, event_t event)
//----- (4C4150) --------------------------------------------------------
card_amrou_kithkin(int player, int card, event_t event)
//----- (4C41A0) --------------------------------------------------------
C_card_elven_riders(int player, int card, event_t event)
//----- (4C4210) --------------------------------------------------------
C_card_hypnotic_specter(int player, int card, event_t event)
//----- (4C42D0) --------------------------------------------------------
card_marsh_viper(int player, int card, event_t event)
//----- (4C4390) --------------------------------------------------------
card_pit_scorpion(int player, int card, event_t event)
//----- (4C4450) --------------------------------------------------------
card_nafs_asp(int player, int card, event_t event)
//----- (4C4520) --------------------------------------------------------
asp_sting(int player, int card)
//----- (4C45C0) --------------------------------------------------------
card_giant_tortoise(int player, int card, event_t event)
//----- (4C4600) --------------------------------------------------------
card_personal_incarnation(int player, int card, event_t event)
//----- (4C48A0) --------------------------------------------------------
C_card_ali_from_cairo(int player, int card, event_t event)
//----- (4C4990) --------------------------------------------------------
card_carrion_ants(int player, int card, event_t event)
//----- (4C4C10) --------------------------------------------------------
C_card_shivan_dragon(int player, int card, event_t event)
//----- (4C4E90) --------------------------------------------------------
card_dragon_whelp(int player, int card, event_t event)
//----- (4C51E0) --------------------------------------------------------
card_goblin_balloon_brigade(int player, int card, event_t event)
//----- (4C5320) --------------------------------------------------------
C_card_whirling_dervish(int player, int card, event_t event)
//----- (4C5450) --------------------------------------------------------
C_card_nightmare(int player, int card, event_t event)
//----- (4C54C0) --------------------------------------------------------
card_angry_mob(int player, int card, event_t event)
//----- (4C5570) --------------------------------------------------------
card_gaeas_liege(int player, int card, event_t event)
//----- (4C5790) --------------------------------------------------------
card_plague_rats(int player, int card, event_t event)
//----- (4C57F0) --------------------------------------------------------
card_keldon_warlord(int player, int card, event_t event)
//----- (4C5850) --------------------------------------------------------
card_ghost_ship(int player, int card, event_t event)
//----- (4C5890) --------------------------------------------------------
regenerate(int player, int card, event_t event, color_t color, int amount)
//----- (4C59C0) --------------------------------------------------------
regenerate_target(int player, int card)
//----- (4C5AC0) --------------------------------------------------------
card_uthden_troll(int player, int card, event_t event)
//----- (4C5B40) --------------------------------------------------------
card_vampire_bats(int player, int card, event_t event)
//----- (4C5DE0) --------------------------------------------------------
card_frozen_shade(int player, int card, event_t event)
//----- (4C6000) --------------------------------------------------------
card_killer_bees(int player, int card, event_t event)
//----- (4C6290) --------------------------------------------------------
card_wall_of_water(int player, int card, event_t event)
//----- (4C6500) --------------------------------------------------------
card_rag_man(int player, int card, event_t event)
//----- (4C6840) --------------------------------------------------------
C_card_time_elemental(int player, int card, event_t event)
//----- (4C6A70) --------------------------------------------------------
C_card_northern_paladin(int player, int card, event_t event)
//----- (4C6BA0) --------------------------------------------------------
card_royal_assassin(int player, int card, event_t event)
//----- (4C6CD0) --------------------------------------------------------
C_card_osai_vultures(int player, int card, event_t event)
//----- (4C6EA0) --------------------------------------------------------
card_murk_dwellers(int player, int card, event_t event)
//----- (4C6F60) --------------------------------------------------------
C_card_sorceress_queen(int player, int card, event_t event)
//----- (4C7170) --------------------------------------------------------
card_stone_giant(int player, int card, event_t event)
//----- (4C73D0) --------------------------------------------------------
card_dwarven_warriors(int player, int card, event_t event)
//----- (4C75B0) --------------------------------------------------------
card_cave_people(int player, int card, event_t event)
//----- (4C77D0) --------------------------------------------------------
card_pradesh_gypsies(int player, int card, event_t event)
//----- (4C79F0) --------------------------------------------------------
card_samite_healer(int player, int card, event_t event)
//----- (4C7BD0) --------------------------------------------------------
card_verduran_enchantress(int player, int card, event_t event)
//----- (4C7CE0) --------------------------------------------------------
card_serra_angel(int player, int card, event_t event)
//----- (4C7D10) --------------------------------------------------------
UNUSED_card_giant_spider(int player, int card, event_t event)
//----- (4C7D50) --------------------------------------------------------
card_matca_rioters(int player, int card, event_t event)
//----- (4C7DF0) --------------------------------------------------------
helper_zombie_master(int player, int card, int internal_card_id)
//----- (4C7E30) --------------------------------------------------------
card_scathe_zombies(int player, int card, event_t event)
//----- (4C7E70) --------------------------------------------------------
C_card_goblin_king(int player, int card, event_t event)
//----- (4C7EF0) --------------------------------------------------------
card_erg_raiders(int player, int card, event_t event)
//----- (4C8070) --------------------------------------------------------
C_card_el_hajjaj(int player, int card, event_t event)
//----- (4C81D0) --------------------------------------------------------
C_card_leviathan(int player, int card, event_t event)
//----- (4C8450) --------------------------------------------------------
C_leviathan_helper<eax>(int a1<ebx>, int a2<edi>, card_instance_t *a3<esi>, int preferred_controller, int a5, unsigned int a6)
//----- (4C85F0) --------------------------------------------------------
card_brothers_of_fire(int player, int card, event_t event)
//----- (4C8700) --------------------------------------------------------
card_crimson_manticore(int player, int card, event_t event)
//----- (4C8800) --------------------------------------------------------
card_prodigal_sorcerer(int player, int card, event_t event)
//----- (4C8990) --------------------------------------------------------
sub_4C8990(int who_chooses, int card)
//----- (4C8B90) --------------------------------------------------------
damage_creature_or_player(int player, int card, event_t event, int amount)
//----- (4C8C60) --------------------------------------------------------
card_pirate_ship(int player, int card, event_t event)
//----- (4C8D30) --------------------------------------------------------
card_sea_serpent(int player, int card, event_t event)
//----- (4C8DD0) --------------------------------------------------------
C_card_elder_land_wurm(int player, int card, event_t event)
//----- (4C8E40) --------------------------------------------------------
C_card_island_fish_jasconius(int player, int card, event_t event)
//----- (4C9030) --------------------------------------------------------
C_card_lord_of_atlantis(int player, int card, event_t event)
//----- (4C90F0) --------------------------------------------------------
C_card_goblin_rock_sled(int player, int card, event_t event)
//----- (4C9180) --------------------------------------------------------
card_orcish_artillery(int player, int card, event_t event)
//----- (4C9270) --------------------------------------------------------
card_psionic_entity(int player, int card, event_t event)
//----- (4C93B0) --------------------------------------------------------
C_card_scavenging_ghoul(int player, int card, event_t event)
//----- (4C94F0) --------------------------------------------------------
card_sengir_vampire(int player, int card, event_t event)
//----- (4C96D0) --------------------------------------------------------
card_junun_efreet(int player, int card, event_t event)
//----- (4C9770) --------------------------------------------------------
card_phantasmal_forces(int player, int card, event_t event)
//----- (4C9810) --------------------------------------------------------
card_force_of_nature(int player, int card, event_t event)
//----- (4C9900) --------------------------------------------------------
C_card_llanowar_elves(int player, int card, event_t event)
//----- (4C9A30) --------------------------------------------------------
C_card_birds_of_paradise(int player, int card, event_t event)
//----- (4C9CF0) --------------------------------------------------------
card_cosmic_horror(int player, int card, event_t event)
//----- (4C9E10) --------------------------------------------------------
card_lord_of_the_pit(int player, int card, event_t event)
//----- (4CA090) --------------------------------------------------------
controls_any_other_creatures(signed int player, int card)
//----- (4CA0F0) --------------------------------------------------------
card_ball_lightning(int player, int card, event_t event)
//----- (4CA1D0) --------------------------------------------------------
card_cyclopean_mummy(int player, int card, event_t event)
//----- (4CA200) --------------------------------------------------------
C_card_nether_shadow(int player, int card, event_t event)
//----- (4CA2C0) --------------------------------------------------------
unused_fx_nether_shadow(int player, int card, event_t event)
//----- (4CA440) --------------------------------------------------------
card_white_knight(int player, int card, event_t event)
//----- (4CA4A0) --------------------------------------------------------
card_black_knight(int player, int card, event_t event)
//----- (4CA500) --------------------------------------------------------
make_colored_auras_fall_off(int player, int card, color_t color)
//----- (4CA5A0) --------------------------------------------------------
card_ali_baba(int player, int card, event_t event)
//----- (4CA670) --------------------------------------------------------
C_card_ley_druid(int player, int card, event_t event)
//----- (4CA760) --------------------------------------------------------
C_card_venom(int player, int card, event_t event)
//----- (4CA9E0) --------------------------------------------------------
C_card_thicket_basilisk(int player, int card, event_t event)
//----- (4CAB80) --------------------------------------------------------
C_card_wall_of_dust(int player, int card, event_t event)
//----- (4CAC70) --------------------------------------------------------
card_sisters_of_the_flame(int player, int card, event_t event)
//----- (4CAD10) --------------------------------------------------------
C_card_apprentice_wizard(int player, int card, event_t event)
//----- (4CADC0) --------------------------------------------------------
C_card_abomination(int player, int card, event_t event)
//----- (4CAFB0) --------------------------------------------------------
card_radjan_spirit(int player, int card, event_t event)
//----- (4CB0F0) --------------------------------------------------------
card_hurr_jackal(int player, int card, event_t event)
//----- (4CB230) --------------------------------------------------------
any_permanents_controlled_of_type(signed int player, type_t type)
//----- (4CB280) --------------------------------------------------------
call_function_for_each_card_in_play(void (__cdecl *fn)(int, int, int), int only_if_controlled_by_this_player_or_this_is_negative_1)
//----- (4CB2E0) --------------------------------------------------------
add_special_counter(int player, int card)
//----- (4CB310) --------------------------------------------------------
C_remove_special_counter(int player, int card)
//----- (4CB330) --------------------------------------------------------
C_add_special_counters(int player, int card, char a3)
//----- (4CB360) --------------------------------------------------------
C_remove_special_counters(int player, int card, int a3)
//----- (4CB380) --------------------------------------------------------
assign_special_counters(int player, int card, signed int a3)
//----- (4CB3B0) --------------------------------------------------------
C_get_special_counters(int player, int card)
//----- (4CB3D0) --------------------------------------------------------
TENTATIVE_sacrifice_a_creature(int player)
//----- (4CB520) --------------------------------------------------------
sacrifice_a_land(int player)
//----- (4CB590) --------------------------------------------------------
sub_4CB590(int preferred_controller)
//----- (4CB600) --------------------------------------------------------
sub_4CB600(HWND hWndParent, LPARAM a2, const RECT *lprcSrc)
//----- (4CB6A0) --------------------------------------------------------
sub_4CB6A0(HANDLE *a1, int a2, int a3, int a4, int a5, int a6)
//----- (4CB760) --------------------------------------------------------
sub_4CB760(void *a1, HGDIOBJ ho, void *a3)
//----- (4CB7A0) --------------------------------------------------------
TENTATIVE_dlgproc_credits(HWND hwnd, WM_t msg, WPARAM wparam, LPARAM lparam)
//----- (4CBDE0) --------------------------------------------------------
sub_4CBDE0(HINSTANCE a1, int a2, char *Str1, int a4)
//----- (4CC3D0) --------------------------------------------------------
sub_4CC3D0(HWND hWnd, int a2)
//----- (4CC6F0) --------------------------------------------------------
sub_4CC6F0(const CHAR *a1)
//----- (4CC770) --------------------------------------------------------
wndproc_MagicShellClass(HWND hwnd, WM_t msg, WPARAM wparam, LPARAM lparam)
//----- (4CEA20) --------------------------------------------------------
sub_4CEA20()
//----- (4CEC40) --------------------------------------------------------
sub_4CEC40(int a1, int a5)
//----- (4CEEB0) --------------------------------------------------------
sub_4CEEB0()
//----- (4CEF00) --------------------------------------------------------
sub_4CEF00(HWND hDlg, HANDLE *a2, LPVOID pv, int a4, int a5, int a6, int a7, int a8, int a9, int a10, WPARAM *a11, WPARAM *a12, int a13, int a14)
//----- (4CF050) --------------------------------------------------------
sub_4CF050(HWND hDlg, int a2, WPARAM *a3, WPARAM *a4)
//----- (4CF1B0) --------------------------------------------------------
sub_4CF1B0(HWND hDlg, HGDIOBJ ho, void *a3, int a4, void *a5, void *a6, void *a7, void *a8, void *a9)
//----- (4CF2E0) --------------------------------------------------------
sub_4CF2E0(HWND hDlg, HGDIOBJ ho, void *a3, void *a4)
//----- (4CF3A0) --------------------------------------------------------
sub_4CF3A0()
//----- (4CF4D0) --------------------------------------------------------
sub_4CF4D0(BOOL a1)
//----- (4CF540) --------------------------------------------------------
sub_4CF540(int a1)
//----- (4CF5B0) --------------------------------------------------------
sub_4CF5B0(int a1)
//----- (4CF610) --------------------------------------------------------
sub_4CF610(HWND hDlg)
//----- (4CF810) --------------------------------------------------------
sub_4CF810(HWND hDlg, int a2, int nPos)
//----- (4CFA30) --------------------------------------------------------
sub_4CFA30(HDC hdc, HWND hWnd, HANDLE h)
//----- (4CFAB0) --------------------------------------------------------
sub_4CFAB0(HWND hWnd, HANDLE h, const RECT *lprcSrc, LPRECT lprcDst)
//----- (4CFB30) --------------------------------------------------------
sub_4CFB30(HWND hWnd, HANDLE h, const RECT *lprcSrc)
//----- (4CFB80) --------------------------------------------------------
sub_4CFB80(int a1)
//----- (4CFBD0) --------------------------------------------------------
sub_4CFBD0(HWND hDlg, unsigned int a2, HDC a3, int a4)
//----- (4CFE20) --------------------------------------------------------
register_FullCardClass(const CHAR *a1)
//----- (4CFF00) --------------------------------------------------------
destroy_FullCardClass()
//----- (4CFF20) --------------------------------------------------------
wndproc_FullCardClass(HWND hwnd, WM_t msg, WPARAM wparam, LPARAM lparam)
//----- (4D0DF0) --------------------------------------------------------
sub_4D0DF0()
//----- (4D1390) --------------------------------------------------------
sub_4D1390()
//----- (4D1780) --------------------------------------------------------
draw_fullcard_normal(int a1, int a2, int a3, int player, int card, int a6, int a7)
//----- (4D1950) --------------------------------------------------------
draw_fullcard_activation_card(HDC hdc, RECT *rect, int player, int card)
//----- (4D1BC0) --------------------------------------------------------
draw_fullcard_special_effect_card(HDC hdc, const RECT *rect, int csvid, int player, int card)
//----- (4D25D0) --------------------------------------------------------
draw_smallcard_special_effect_card(HDC hdc, RECT *dest_rect, int a3, int player, int card)
//----- (4D2B30) --------------------------------------------------------
draw_smallcard_activation_card(HDC hdc, RECT *rect, int csvid, int player, int card, int parent_player, int parent_card)
//----- (4D2D80) --------------------------------------------------------
draw_smallcard(HDC hdc, RECT *card_rect, int player, int card, int a5, int a6, int a7, int a8, int a9)
//----- (4D3320) --------------------------------------------------------
draw_powertoughness_on_smallcard(HDC hdc, RECT *card_rect, int pow, int tgh)
//----- (4D34E0) --------------------------------------------------------
sub_4D34E0(HDC hdc, RECT *a2, int a3)
//----- (4D3630) --------------------------------------------------------
draw_damage(HDC hdc, RECT *rect, int a3)
//----- (4D3770) --------------------------------------------------------
copy_damage_rect(RECT *a1, LPRECT lprc)
//----- (4D3800) --------------------------------------------------------
draw_id_tag(HDC hdc, RECT *rect, int player, int card, int show_id_tags)
//----- (4D3A00) --------------------------------------------------------
C_draw_special_counters(HDC hdc, RECT *rect, special_counter_type counter_type, int num)
//----- (4D3B60) --------------------------------------------------------
copy_special_counter_rect(LPRECT lprcDst, RECT *rect, int num)
//----- (4D3CC0) --------------------------------------------------------
C_draw_standard_counters(HDC hdc, RECT *a2, int special_counters_byte1, int special_counters_byte2, int special_counters_byte3, int counters, int unknown0x121)
//----- (4D4130) --------------------------------------------------------
draw_abilities(HDC hdc, RECT *rect, keyword_t keyword_bits_to_display)
//----- (4D43D0) --------------------------------------------------------
copy_rect_for_ability(LPRECT lprcDst, keyword_t keyword_bit, RECT *rect, keyword_t displayable_keyword_bits)
//----- (4D4600) --------------------------------------------------------
draw_manastripes(HDC hdc, RECT *rect, int player, int card)
//----- (4D4820) --------------------------------------------------------
blt_summon_pic(HDC hdc, RECT *rect)
//----- (4D48B0) --------------------------------------------------------
draw_oublietted_effect(HDC hdc, RECT *rect)
//----- (4D49B0) --------------------------------------------------------
blt_dying_pic(HDC hdc, RECT *rect)
//----- (4D4A40) --------------------------------------------------------
draw_target_canttarget(HDC hdc, RECT *a2, int draw_target, int draw_canttarget)
//----- (4D4B50) --------------------------------------------------------
sub_4D4B50(HDC hdc, RECT *rect, int visible)
//----- (4D4BE0) --------------------------------------------------------
sub_4D4BE0(HDC hdc, RECT *a2, char a3)
//----- (4D4C20) --------------------------------------------------------
copy_untap_status_rect(LPRECT lprc, RECT *a2)
//----- (4D4CA0) --------------------------------------------------------
C_get_counter_type_by_id(int a1)
//----- (4D4CBE) --------------------------------------------------------
C_tail_get_counter_type_by_id<eax>(int a1<ebp>)
//----- (4D4ED0) --------------------------------------------------------
copy_string_alternative_by_pipe_hash_digit(char *dest, const char *source, int alternative)
//----- (4D4FC0) --------------------------------------------------------
sub_4D4FC0(HDC hdc, UINT *a2)
//----- (4D5080) --------------------------------------------------------
sub_4D5080(int a1)
//----- (4D5240) --------------------------------------------------------
strcpy(char *deststr, const char *srcstr)
//----- (4D5260) --------------------------------------------------------
strcat(char *deststr, const char *srcstr)
//----- (4D5290) --------------------------------------------------------
j_strlen<eax>(const char *str<eax>)
//----- (4D52E0) --------------------------------------------------------
start(int a1<eax>)
//----- (4D5450) --------------------------------------------------------
sub_4D5450()
//----- (4D8312) --------------------------------------------------------
card_serpent_warrior(int player, int card, event_t event)
//----- (4D8500) --------------------------------------------------------
card_cathodion(int player, int card, event_t event)
//----- (4D8900) --------------------------------------------------------
sub_4D8900(signed int player, int card, int event)
//----- (4D8AE0) --------------------------------------------------------
sub_4D8AE0(int player, int card, char a3)
//----- (4D8F80) --------------------------------------------------------
C_card_shadowmage_infiltrator(int player, int card, event_t event)
//----- (4D8FD0) --------------------------------------------------------
C_card_blazing_specter(int player, int card, event_t event)
//----- (4D9020) --------------------------------------------------------
C_card_ravenous_baloth(int player, int card, event_t event)
//----- (4D9200) --------------------------------------------------------
card_frogmite(int player, int card, event_t event)
//----- (4D94E0) --------------------------------------------------------
card_bull_cerodon(int player, int card, event_t event)
//----- (4D9530) --------------------------------------------------------
card_stoic_angel(int player, int card, event_t event)
//----- (4D9580) --------------------------------------------------------
C_card_steward_of_valeron(int player, int card, event_t event)
//----- (4D95D0) --------------------------------------------------------
card_vulshok_sorcerer(int player, int card, event_t event)
//----- (4D9620) --------------------------------------------------------
C_card_visara_the_dreadful_nonlegendary(int player, int card, event_t event)
//----- (4D9750) --------------------------------------------------------
sub_4D9750(int a1<ebp>, int a2<esi>)
//----- (4D9790) --------------------------------------------------------
card_manta_riders(int player, int card, event_t event)
//----- (4D98E0) --------------------------------------------------------
card_glimmering_angel(int player, int card, event_t event)
//----- (4D9A30) --------------------------------------------------------
sub_4D9A30(int player, int card, int a3)
//----- (4D9BA0) --------------------------------------------------------
UNKNOWN_card_17(int player, int card, event_t event)
//----- (4D9CC0) --------------------------------------------------------
card_masticore(int player, int card, event_t event)
//----- (4D9D20) --------------------------------------------------------
card_ashcoat_bear(int player, int card, event_t event)
//----- (4D9DD0) --------------------------------------------------------
C_card_counsel_of_the_soratami(int player, int card, event_t event)
//----- (4D9E11) --------------------------------------------------------
card_paladin_en_vec(int player, int card, event_t event)
//----- (4D9E50) --------------------------------------------------------
card_oxidize(int player, int card, event_t event)
//----- (4D9F00) --------------------------------------------------------
C_card_lava_spike(int player, int card, event_t event)
//----- (4D9F7F) --------------------------------------------------------
card_primal_frenzy(int player, int card, event_t event)
//----- (4D9FC9) --------------------------------------------------------
C_card_armadillo_cloak(int player, int card, event_t event)
//----- (4DA214) --------------------------------------------------------
card_diplomatic_immunity(int player, int card, event_t event)
//----- (4DA260) --------------------------------------------------------
card_serras_embrace__very_broken(int player, int card, event_t event)
//----- (4DA517) --------------------------------------------------------
card_infest(int player, int card, event_t event)
//----- (4DA5D0) --------------------------------------------------------
C_card_kiss_of_the_amesha(int player, int card, event_t event)
//----- (4DA656) --------------------------------------------------------
C_card_goblin_king_0(int player, int card, event_t event)
//----- (4DA6FF) --------------------------------------------------------
card_explosive_vegetation(int player, int card, event_t event)
//----- (4DA730) --------------------------------------------------------
C_card_slith_firewalker(int player, int card, event_t event)
//----- (4DA761) --------------------------------------------------------
C_card_slith_bloodletter(int player, int card, event_t event)
//----- (4DA7AA) --------------------------------------------------------
card_annex(int player, int card, event_t event)
//----- (4DA800) --------------------------------------------------------
helper_matca_rioters(int a1, int a2, int a3, int a4)
//----- (4DA854) --------------------------------------------------------
C_card_wild_nacatl(int player, int card, event_t event)
//----- (4DA8AD) --------------------------------------------------------
sub_4DA8AD(signed int player, int card, char event)
//----- (4DA8DE) --------------------------------------------------------
sub_4DA8DE<eax>(int a1<ebx>, int a2<edi>, signed int a3, int a4, char a5)
//----- (4DA90F) --------------------------------------------------------
C_card_eladamri_lord_of_leaves(int player, int card, event_t event)
//----- (4DA950) --------------------------------------------------------
sub_4DA950(int player, int card, int event)
//----- (4DAABA) --------------------------------------------------------
sub_4DAABA(int player, int card, int event)
//----- (4DAB90) --------------------------------------------------------
card_warriors_honor(int player, int card, event_t event)
//----- (4DAC25) --------------------------------------------------------
sub_4DAC25(int a1<esi>)
//----- (4DAC3A) --------------------------------------------------------
UNKNOWN_card_3(int player, int card, event_t event)
//----- (4DAD59) --------------------------------------------------------
sub_4DAD59(int player, int card, char a3)
//----- (4DADFC) --------------------------------------------------------
sub_4DADFC(int player, int card, char a3)
//----- (4DAF3F) --------------------------------------------------------
card_battering_sliver(int player, int card, event_t event)
//----- (4DAFE2) --------------------------------------------------------
sub_4DAFE2(int player, int card, char a3)
//----- (4DB085) --------------------------------------------------------
sub_4DB085(int player, int card, char a3)
//----- (4DB1C8) --------------------------------------------------------
sub_4DB1C8(int player, int card, char a3)
//----- (4DB26B) --------------------------------------------------------
C_card_plated_sliver(int player, int card, event_t event)
//----- (4DB30E) --------------------------------------------------------
sub_4DB30E(int player, int card, char a3)
//----- (4DB3B1) --------------------------------------------------------
card_spitting_sliver(int player, int card, event_t event)
//----- (4DB454) --------------------------------------------------------
sub_4DB454(int player, int card, char a3)
//----- (4DB5A0) --------------------------------------------------------
TENTATIVE_urza_untap_land_effect(int a1<ebx>, int player, int card, char a4, int a5, int a6, int a7, int a8, int a9)
//----- (4DB683) --------------------------------------------------------
sub_4DB683(int player, int card, char a5)
//----- (4DB720) --------------------------------------------------------
sub_4DB720<eax>(int a1<ebx>, int a2<edi>, int a3, int a4, char a5, int a6, int a7, int a8, int a9)
//----- (4DB760) --------------------------------------------------------
C_card_treachery(int player, int card, event_t event)
//----- (4DB7A0) --------------------------------------------------------
C_card_time_spiral(int player, int card, event_t event)
//----- (4DB7E0) --------------------------------------------------------
sub_4DB7E0(int a1<ebx>, int a2, int a3, char a4, int a5, int a6, int a7, int a8)
//----- (4DB820) --------------------------------------------------------
C_card_snap(int player, int card, event_t event)
//----- (4DB860) --------------------------------------------------------
card_auratog(int player, int card, event_t event)
//----- (4DBA20) --------------------------------------------------------
target_available_frontend_73()
//----- (4DBA61) --------------------------------------------------------
C_card_night_of_souls_betrayal(int player, int card, event_t event)
//----- (4DBADD) --------------------------------------------------------
card_serra_advocate(int player, int card, event_t event)
//----- (4DBC2B) --------------------------------------------------------
C_card_sleepers_robe(int player, int card, event_t event)
//----- (4DBF30) --------------------------------------------------------
C_card_gilded_lotus(int player, int card, event_t event)
//----- (4DC1A0) --------------------------------------------------------
card_blightning(int player, int card, event_t event)
//----- (4DC2A3) --------------------------------------------------------
C_card_zephids_embrace(int player, int card, event_t event)
//----- (4DC440) --------------------------------------------------------
card_nightscape_apprentice(int player, int card, event_t event)
//----- (4DCBD2) --------------------------------------------------------
UNKNOWN_card(int player, int card, event_t event)
//----- (4DCEC2) --------------------------------------------------------
card_dismiss(int player, int card, event_t event)
//----- (4DD078) --------------------------------------------------------
UNKNOWN_card_38(int player, int card, event_t event)
//----- (4DD154) --------------------------------------------------------
card_mogg_fanatic(int player, int card, event_t event)
//----- (4DD1DE) --------------------------------------------------------
sub_4DD1DE(int player, int card, char a3)
//----- (4DD281) --------------------------------------------------------
C_card_absorb(int player, int card, event_t event)
//----- (4DD437) --------------------------------------------------------
card_tidings(int player, int card, event_t event)
//----- (4DD472) --------------------------------------------------------
card_opportunity(int player, int card, event_t event)
//----- (4DD559) --------------------------------------------------------
card_seething_song(int player, int card, event_t event)
//----- (4DD5D1) --------------------------------------------------------
C_card_lotus_petal(int player, int card, event_t event)
//----- (4DD82F) --------------------------------------------------------
UNKNOWN_card_30(int player, int card, event_t event)
//----- (4DD924) --------------------------------------------------------
UNKNOWN_card_7(int player, int card, event_t event)
//----- (4DD9E3) --------------------------------------------------------
C_card_time_stretch(int player, int card, event_t event)
//----- (4DDBB6) --------------------------------------------------------
C_card_allay(int player, int card, event_t event)
//----- (4DDBE5) --------------------------------------------------------
C_card_disturbed_burial(int player, int card, event_t event)
//----- (4DDC14) --------------------------------------------------------
C_backend_card_elvish_fury(int player, int card, event_t event)
//----- (4DDD55) --------------------------------------------------------
C_card_elvish_fury(int player, int card, event_t event)
//----- (4DDD84) --------------------------------------------------------
C_card_evincars_justice(int player, int card, event_t event)
//----- (4DDDB3) --------------------------------------------------------
C_backend_card_evincars_justice(int player, int card, event_t event)
//----- (4DDEC0) --------------------------------------------------------
C_card_fanning_the_flames(int player, int card, event_t event)
//----- (4DDEEF) --------------------------------------------------------
C_card_reiterate(int a1<ebx>, int a2<edi>, card_instance_t *a3<esi>, int a4, int a5, int a6, int a7, int a8, int a9, int a10)
//----- (4DDF1E) --------------------------------------------------------
C_card_searing_touch(int player, int card, event_t event)
//----- (4DDF4D) --------------------------------------------------------
C_backend_card_searing_touch(int player, int card, event_t event)
//----- (4DDFD0) --------------------------------------------------------
C_card_seething_anger(int player, int card, event_t event)
//----- (4DDFFF) --------------------------------------------------------
C_backend_card_seething_anger(int player, int card, event_t event)
//----- (4DE140) --------------------------------------------------------
C_card_shattering_pulse(int player, int card, event_t event)
//----- (4DE16F) --------------------------------------------------------
C_card_spell_burst(int a1<ebx>, int a2<edi>, card_instance_t *a3<esi>, int a4, int a5, int a6, int a7, int a8, int a9, int a10)
//----- (4DE19E) --------------------------------------------------------
C_card_whispers_of_the_muse(int player, int card, event_t event)
//----- (4DE1CD) --------------------------------------------------------
C_buyback_card_whispers_of_the_muse(int player, int card, event_t event)
//----- (4DE208) --------------------------------------------------------
C_card_sliver_queen(int player, int card, event_t event)
//----- (4DE3BD) --------------------------------------------------------
card_careful_study(int player, int card, event_t event)
//----- (4DE61A) --------------------------------------------------------
C_card_frantic_search(int player, int card, event_t event)
//----- (4DE6A1) --------------------------------------------------------
sub_4DE6A1(int player, int card, event_t event, int a4, int a5, int a6, int a7)
//----- (4DE6C9) --------------------------------------------------------
card_pillage(int player, int card, event_t event)
//----- (4DE789) --------------------------------------------------------
C_card_timetwister_clone(int player, int card, event_t event)
//----- (4DE8DE) --------------------------------------------------------
sub_4DE8DE<eax>(char a1<al>, int a2<ebx>, int a3<edi>, int a4, int a5, char a6)
//----- (4DE911) --------------------------------------------------------
sub_4DE911(int player, int card, char a5)
//----- (4DE95C) --------------------------------------------------------
create_token_and_put_it_into_play(int player, card_id_t csvid)
//----- (4DE9A7) --------------------------------------------------------
C_card_icatian_town(int player, int card, event_t event)
//----- (4DEA12) --------------------------------------------------------
C_card_bitterblossom(int a3, int a4, int a5)
//----- (4DEB11) --------------------------------------------------------
sub_4DEB11()
//----- (4DEB1D) --------------------------------------------------------
C_card_broodmate_dragon(int a3, int a4, char a5)
//----- (4DEB8F) --------------------------------------------------------
card_last_gasp(int player, int card, event_t event)
//----- (4DECD0) --------------------------------------------------------
C_card_dragon_fodder(int player, int card, event_t event)
//----- (4DED1D) --------------------------------------------------------
C_card_raise_the_alarm(int player, int card, char a5)
//----- (4DED6A) --------------------------------------------------------
sub_4DED6A(int player, int card, char a5)
//----- (4DEDB7) --------------------------------------------------------
card_remand(int player, int card, event_t event)
//----- (4DEF65) --------------------------------------------------------
card_memory_lapse(int player, int card, event_t event)
//----- (4DF0D6) --------------------------------------------------------
card_hymn_to_tourach(int player, int card, event_t event)
//----- (4DF159) --------------------------------------------------------
card_stupor(int player, int card, event_t event)
//----- (4DF1DB) --------------------------------------------------------
card_final_judgment(int player, int card, event_t event)
//----- (4DF26A) --------------------------------------------------------
C_card_field_marshal(int player, int card, event_t event)
//----- (4DF31C) --------------------------------------------------------
card_wizened_cenn(int player, int card, event_t event)
//----- (4DF3E0) --------------------------------------------------------
UNKNOWN_card_1(int player, int card, event_t event)
//----- (4DF4A4) --------------------------------------------------------
sub_4DF4A4(int player, int card, char a3)
//----- (4DF556) --------------------------------------------------------
sub_4DF556(int player, int card, char a3)
//----- (4DF608) --------------------------------------------------------
sub_4DF608(int player, int card, char a3)
//----- (4DF6BA) --------------------------------------------------------
sub_4DF6BA(signed int player, int card, char a3)
//----- (4DF77E) --------------------------------------------------------
sub_4DF77E<eax>(char a1<al>, int a2<ebx>, int a3<edi>, int player, int card, char a6)
//----- (4DF886) --------------------------------------------------------
sub_4DF886(signed int player, int card, char a3)
//----- (4DF94A) --------------------------------------------------------
sub_4DF94A<eax>(void *a1<ebp>)
//----- (4DF9EA) --------------------------------------------------------
sub_4DF9EA(int a3, int a4, char a5)
//----- (4DFA66) --------------------------------------------------------
sub_4DFA66(int a1, int a2, char a3)
//----- (4DFAA9) --------------------------------------------------------
UNKNOWN_card_0(int player, int card, event_t event)
//----- (4DFD9E) --------------------------------------------------------
UNKNOWN_card_2(int player, int card, event_t event)
//----- (4E0093) --------------------------------------------------------
card_sakura_tribe_elder(int player, int card, event_t event)
//----- (4E0388) --------------------------------------------------------
card_terminate(int player, int card, event_t event)
//----- (4E042C) --------------------------------------------------------
sub_4E042C(int a1<eax>, int a2<ebx>, int a3<edi>, card_instance_t *a4<esi>)
//----- (4E0442) --------------------------------------------------------
UNKNOWN_card_6(int player, int card, event_t event)
//----- (4E0737) --------------------------------------------------------
C_card_slith_ascendant(int player, int card, event_t event)
//----- (4E080B) --------------------------------------------------------
C_card_spawnwrithe(int player, int card, event_t event)
//----- (4E08E1) --------------------------------------------------------
C_card_kaysa(int player, int card, event_t event)
//----- (4E0980) --------------------------------------------------------
C_card_tolsimir_wolfblood(int player, int card, event_t event)
//----- (4E0B04) --------------------------------------------------------
create_voja_token()
//----- (4E0B17) --------------------------------------------------------
sub_4E0B17(int player, int card, int a3, signed int a4)
//----- (4E0DA9) --------------------------------------------------------
UNKNOWN_card_12(int player, int card, event_t event)
//----- (4E0DDC) --------------------------------------------------------
sub_4E0DDC(int player, int card, int a5)
//----- (4E0E45) --------------------------------------------------------
TENTATIVE_original_card_lord_of_the_pit(int player, int card, event_t event)
//----- (4E0F2A) --------------------------------------------------------
UNKNOWN_card_14(int player, int card, event_t event)
//----- (4E121D) --------------------------------------------------------
UNKNOWN_card_18(int player, int card, event_t event)
//----- (4E1510) --------------------------------------------------------
UNKNOWN_card_19(int player, int card, event_t event)
//----- (4E1803) --------------------------------------------------------
UNKNOWN_card_20(int player, int card, event_t event)
//----- (4E1AF6) --------------------------------------------------------
sub_4E1AF6(int player, int card, char a3)
//----- (4E1BA8) --------------------------------------------------------
card_undermine(int player, int card, event_t event)
//----- (4E1DEB) --------------------------------------------------------
sub_4E1DEB<eax>(int a1<ebx>, int a2<edi>, int player, int a4, char a5)
//----- (4E1EDB) --------------------------------------------------------
sub_4E1EDB<eax>(int a1<ebx>, int a2<ebp>, int a3<edi>)
//----- (4E1EEE) --------------------------------------------------------
sub_4E1EEE<eax>(int a1<ebx>, int a2<edi>, int player, int a4, char a5)
//----- (4E1FDE) --------------------------------------------------------
sub_4E1FDE<eax>(int a1<ebx>, int a2<ebp>, int a3<edi>)
//----- (4E1FF1) --------------------------------------------------------
card_veteran_armorer(int player, int card, event_t event)
//----- (4E203F) --------------------------------------------------------
card_zombie_master(int player, int card, event_t event)
//----- (4E20E7) --------------------------------------------------------
sub_4E20E7(int player, int card, char a5)
//----- (4E2130) --------------------------------------------------------
sub_4E2130(int player, int card, char event)
//----- (4E2162) --------------------------------------------------------
sub_4E2162(int a1, int a2, char a3)
//----- (4E21B7) --------------------------------------------------------
sub_4E21B7<eax>(int a1<eax>, char a2<dl>)
//----- (4E226C) --------------------------------------------------------
UNKNOWN_card_36(int player, int card, event_t event)
//----- (4E23C6) --------------------------------------------------------
card_blinking_spirit(int player, int card, event_t event)
//----- (4E244F) --------------------------------------------------------
C_card_zuran_orb(int player, int card, event_t event)
//----- (4E2582) --------------------------------------------------------
sub_4E2582(int player, int card, char a5)
//----- (4E25D1) --------------------------------------------------------
sub_4E25D1(int player, int card, char a4)
//----- (4E26E4) --------------------------------------------------------
card_channel_the_suns(int player, int card, event_t event)
//----- (4E279B) --------------------------------------------------------
sub_4E279B(int player, int card, char a3)
//----- (4E28D6) --------------------------------------------------------
sub_4E28D6(int a3, int a4, char a5)
//----- (4E29C7) --------------------------------------------------------
sub_4E29C7(int a3, int a4, char a5)
//----- (4E29F9) --------------------------------------------------------
sub_4E29F9(int player, int bigcard_card, char a3)
//----- (4E2AAB) --------------------------------------------------------
#error "4E2AAB: call analysis failed (funcsize=36)"
//----- (4E2B05) --------------------------------------------------------
sub_4E2B05<eax>(int32_t a1<ebx>, int a2<edi>, int player, int card, char a5)
//----- (4E2BA4) --------------------------------------------------------
sub_4E2BA4<eax>(int32_t a1<ebx>, int player, int card)
//----- (4E2CA7) --------------------------------------------------------
C_card_corrupt(int player, int card, event_t event)
//----- (4E2DCA) --------------------------------------------------------
card_nights_whisper(int player, int card, event_t event)
//----- (4E2E95) --------------------------------------------------------
sub_4E2E95<eax>(int32_t a1<ebx>, int a2<edi>, int a3<esi>, int player, int card, event_t event)
//----- (4E2FB8) --------------------------------------------------------
C_card_kongming_sleeping_dragon(int player, int card, event_t event)
//----- (4E3019) --------------------------------------------------------
C_card_tendrils_of_corruption(int player, int card, event_t event)
//----- (4E30D0) --------------------------------------------------------
UNKNOWN_card_39(int player, int card, event_t event)
//----- (4E32EC) --------------------------------------------------------
C_card_death_grasp(int player, int card, event_t event)
//----- (4E33F7) --------------------------------------------------------
C_card_godsire(int player, int card, event_t event)
//----- (4E3472) --------------------------------------------------------
create_vampiric_tutor_token()
//----- (4E3525) --------------------------------------------------------
C_card_font_of_mythos(int player, int card, event_t event)
//----- (4E3570) --------------------------------------------------------
card_noble_panther(int player, int card, event_t event)
//----- (4E36DB) --------------------------------------------------------
C_card_akromas_memorial(int player, int card, event_t event)
//----- (4E3769) --------------------------------------------------------
C_creatures_you_control_have_haste(int player, int card, event_t event)
//----- (4E37C9) --------------------------------------------------------
card_serras_blessing(int player, int card, event_t event)
//----- (4E382D) --------------------------------------------------------
sub_4E382D(int player, int card, char a3)
//----- (4E38BA) --------------------------------------------------------
C_card_spidersilk_armor(int player, int card, event_t event)
//----- (4E3955) --------------------------------------------------------
C_creatures_you_control_have_firststrike_trample_flying_unslighted_prot_red_black(signed int player, int card, char a3)
//----- (4E39E0) --------------------------------------------------------
C_card_eladamri_lord_of_leaves_nonlegendary(signed int player, int card, event_t event)
//----- (4E3A94) --------------------------------------------------------
card_rain_of_salt(int player, int card, event_t event)
//----- (4E3BCC) --------------------------------------------------------
card_angel_of_despair(int player, int card, event_t event)
//----- (4E3C80) --------------------------------------------------------
card_indrik_stomphowler(int player, int card, event_t event)
//----- (4E3D30) --------------------------------------------------------
UNKNOWN_card_31(int player, int card, event_t event)
//----- (4E3DE0) --------------------------------------------------------
card_goblin_settler(int player, int card, event_t event)
//----- (4E3E90) --------------------------------------------------------
C_card_gravedigger(int player, int card, event_t event)
//----- (4E3EF2) --------------------------------------------------------
C_raise_dead_ability<eax>(int a1<ebx>, int a2<ebp>, int a3<edi>)
//----- (4E3F30) --------------------------------------------------------
C_raise_dead_ability_guts(int player, int card, char a5)
//----- (4E4148) --------------------------------------------------------
UNKNOWN_card_40(int player, int card, event_t event)
//----- (4E41FA) --------------------------------------------------------
C_card_eternal_witness(int player, int card, event_t event)
//----- (4E425C) --------------------------------------------------------
C_regrowth_ability<eax>(int a1<ebx>, int a2<ebp>, int a3<edi>)
//----- (4E429A) --------------------------------------------------------
regrowth_ability_guts(int player, int card, char a5)
//----- (4E4443) --------------------------------------------------------
C_card_farhaven_elf(int player, int card, event_t event)
//----- (4E44A5) --------------------------------------------------------
sub_4E44A5<eax>(int a1<ebp>, int a2<ebx>, int a3<edi>)
//----- (4E44E3) --------------------------------------------------------
UNKNOWN_card_21(int player, int card, event_t event)
//----- (4E46C3) --------------------------------------------------------
C_card_sanctum_gargoyle(int player, int card, event_t event)
//----- (4E4725) --------------------------------------------------------
C_reconstruction_ability<eax>(int a1<ebx>, int a2<ebp>)
//----- (4E4763) --------------------------------------------------------
C_reconstruction_ability_guts(int player, int card, int a4)
//----- (4E4976) --------------------------------------------------------
card_assassinate(int player, int card, event_t event)
//----- (4E4A1A) --------------------------------------------------------
sub_4E4A1A(int a1<eax>, int a2<ebx>, int a3<edi>, card_instance_t *a4<esi>)
//----- (4E4A34) --------------------------------------------------------
C_card_voice_of_all(int player, int card, event_t event)
//----- (4E4B64) --------------------------------------------------------
card_putrefy(int player, int card, event_t event)
//----- (4E4C0A) --------------------------------------------------------
target_available_frontend_74()
//----- (4E4C3F) --------------------------------------------------------
select_target_frontend_72<eax>(const char *a1<edx>)
//----- (4E4C9F) --------------------------------------------------------
validate_target_frontend_50<eax>(int a1<eax>)
//----- (4E4CDF) --------------------------------------------------------
UNKNOWN_card_22(int player, int card, event_t event)
//----- (4E4E95) --------------------------------------------------------
card_windfall(int player, int card, event_t event)
//----- (4E506E) --------------------------------------------------------
sub_4E506E<eax>(card_instance_t *a1<esi>, int player, int card, int a4)
//----- (4E52C9) --------------------------------------------------------
C_card_biorhythm(int player, int card, event_t event)
//----- (4E5367) --------------------------------------------------------
card_ponder(int player, int card, event_t event)
//----- (4E5399) --------------------------------------------------------
ponder_helper(int player, int card, event_t event)
//----- (4E5459) --------------------------------------------------------
sub_4E5459(int a3, int a4, char a5)
//----- (4E5580) --------------------------------------------------------
sub_4E5580(int player, int card, event_t event)
//----- (4E56A6) --------------------------------------------------------
C_card_tolarian_academy(int player, int card, event_t event)
//----- (4E56D7) --------------------------------------------------------
C_card_gaeas_cradle(int player, int card, event_t event)
//----- (4E5708) --------------------------------------------------------
sub_4E5708(int player, int card, event_t event)
//----- (4E582C) --------------------------------------------------------
C_card_fabricate(int player, int card, event_t event)
//----- (4E585D) --------------------------------------------------------
type_tutor(int player, int card, event_t event, type_t type, type_t second_type, char mode_1putontop_2graveyard_3putinplay, int lifecost)
//----- (4E5A14) --------------------------------------------------------
C_card_vampiric_tutor(int player, int card, event_t event)
//----- (4E5A45) --------------------------------------------------------
C_card_grim_tutor(int player, int card, event_t event)
//----- (4E5A76) --------------------------------------------------------
C_card_idyllic_tutor(int player, int card, event_t event)
//----- (4E5AA7) --------------------------------------------------------
C_card_personal_tutor(int player, int card, event_t event)
//----- (4E5AD8) --------------------------------------------------------
card_sylvan_scrying(int player, int card, event_t event)
//----- (4E5B09) --------------------------------------------------------
C_card_worldly_tutor(int player, int card, event_t event)
//----- (4E5B3A) --------------------------------------------------------
card_eladamris_call(int player, int card, event_t event)
//----- (4E5B6B) --------------------------------------------------------
C_card_enlightened_tutor(int player, int card, event_t event)
//----- (4E5B9C) --------------------------------------------------------
C_card_entomb(int player, int card, event_t event)
//----- (4E5BCD) --------------------------------------------------------
C_card_mystical_tutor(int player, int card, event_t event)
//----- (4E5BFE) --------------------------------------------------------
card_buried_alive(int player, int card, event_t event)
//----- (4E5C2F) --------------------------------------------------------
card_exclude(int player, int card, event_t event)
//----- (4E5C60) --------------------------------------------------------
card_draw_a_card(int player, int card, event_t event)
//----- (4E5C99) --------------------------------------------------------
card_lay_of_the_land(int player, int card, event_t event)
//----- (4E5E71) --------------------------------------------------------
C_card_kodamas_reach(int player, int card, event_t event)
//----- (4E5EA2) --------------------------------------------------------
C_reach_through_mists(int player, int card, event_t event)
//----- (4E5EDB) --------------------------------------------------------
C_card_upheaval2(int player, int card, event_t event)
//----- (4E5F14) --------------------------------------------------------
null_spell(int player, int card, event_t event)
//----- (4E5F4D) --------------------------------------------------------
C_card_upheaval(int player, int card, event_t event)
//----- (4E5FDA) --------------------------------------------------------
C_card_rofellos_llanowar_emissary_nonlegendary(int player, int card, event_t event)
//----- (4E6119) --------------------------------------------------------
C_broken_produce_g_for_each_forest(int player, int card, int a3)
//----- (4E618B) --------------------------------------------------------
C_card_rofellos_llanowar_emissary(int player, int card, event_t event)
//----- (4E61C4) --------------------------------------------------------
//----- (4E6303) --------------------------------------------------------
C_produce_u_for_each_artifact(int player, int a2, event_t event, int a4, int a5, int a6, signed int player2)
//----- (4E6397) --------------------------------------------------------
//----- (4E64D6) --------------------------------------------------------
sub_4E64D6(int a1, int a2, int a3, int a4, int a5, int a6, signed int player)
//----- (4E656A) --------------------------------------------------------
card_shrapnel_blast(int player, int card, event_t event)
//----- (4E659B) --------------------------------------------------------
sub_4E659B(int player, int card, int event)
//----- (4E661C) --------------------------------------------------------
sub_4E661C(int preferred_controller, int a4, int a5)
//----- (4E66F0) --------------------------------------------------------
card_flame_rift(int player, int card, event_t event)
//----- (4E6754) --------------------------------------------------------
UNKNOWN_card_24(int player, int card, event_t event)
//----- (4E67EE) --------------------------------------------------------
C_card_test_of_endurance(int player, int card, event_t event)
//----- (4E68FC) --------------------------------------------------------
C_card_silvos_rogue_elemental(int player, int card, event_t event)
//----- (4E6919) --------------------------------------------------------
C_card_rorix_bladewing(int player, int card, event_t event)
//----- (4E6936) --------------------------------------------------------
C_card_visara_the_dreadful(int player, int card, event_t event)
//----- (4E6953) --------------------------------------------------------
C_card_mirri_cat_warrior(int player, int card, event_t event)
//----- (4E6970) --------------------------------------------------------
sub_4E6970(int player, int card, event_t event)
//----- (4E69A0) --------------------------------------------------------
target_available_frontend_75()
//----- (4E69C9) --------------------------------------------------------
C_card_shard_volley(int player, int card, event_t event)
//----- (4E69E6) --------------------------------------------------------
sub_4E69E6(int preferred_controller, int a4, int a5)
//----- (4E6AC2) --------------------------------------------------------
C_bogardan_hellkite_helper3<eax>(int a1<ebx>, int a2<edi>, int a3, int a4, char a5)
//----- (4E6B24) --------------------------------------------------------
C_bogardan_hellkite_helper2(int a1<ebx>, int a2<ebp>, int a3<edi>)
//----- (4E6B62) --------------------------------------------------------
C_bogardan_hellkite_helper1(int player, int card, event_t event)
//----- (4E6E32) --------------------------------------------------------
C_card_bogardan_hellkite(int player, int card, event_t event)
//----- (4E6E4F) --------------------------------------------------------
C_card_punish_ignorance(int player, int card, event_t event)
//----- (4E708A) --------------------------------------------------------
card_mortify(int player, int card, event_t event)
//----- (4E7130) --------------------------------------------------------
target_available_frontend_76()
//----- (4E7165) --------------------------------------------------------
select_target_frontend_73<eax>(const char *prompt<edx>)
//----- (4E71C5) --------------------------------------------------------
validate_target_frontend_51<eax>(int a1<eax>)
//----- (4E7205) --------------------------------------------------------
sub_4E7205(int a3, int a4, int a5)
//----- (4E7222) --------------------------------------------------------
sub_4E7222(int a3, int a4, int a5)
//----- (4E7253) --------------------------------------------------------
sub_4E7253<eax>(int a1<ebx>, int a2<edi>, int player, int card, char a5)
//----- (4E7483) --------------------------------------------------------
rearrange_top_3(int a1, int a2, char a3)
//----- (4E75AA) --------------------------------------------------------
sub_4E75AA(int player, int a2, unsigned __int8 a3)
//----- (4E7648) --------------------------------------------------------
UNKNOWN_card_25(int player, int card, event_t event)
//----- (4E76FB) --------------------------------------------------------
sub_4E76FB(int player, int card, char a5)
//----- (4E77DD) --------------------------------------------------------
check_hand_for_type(int player, int a2, unsigned __int8 a3)
//----- (4E7846) --------------------------------------------------------
sub_4E7846(int bigcard_player, int bigcard_card, int a5)
//----- (4E78CA) --------------------------------------------------------
sub_4E78CA(int player, int a2, int a3, int a4)
//----- (4E7A40) --------------------------------------------------------
sub_4E7A40(int player, int card, char a5)
//----- (4E7AA2) --------------------------------------------------------
sub_4E7AA2(int a3, int a4, char a5)
//----- (4E7B78) --------------------------------------------------------
sub_4E7B78(int player, int card, char event)
//----- (4E7B95) --------------------------------------------------------
mana_producer_ai_very_reluctant_to_activate(int player, int card, event_t event)
//----- (4E7DF3) --------------------------------------------------------
sub_4E7DF3(int a3, int a4, int a5)
//----- (4E7E26) --------------------------------------------------------
target_player(int player, int card, event_t event)
//----- (4E7F0C) --------------------------------------------------------
sub_4E7F0C(int player, int card, char a5)
//----- (4E7FE7) --------------------------------------------------------
sub_4E7FE7(int player, int card, event_t event)
//----- (4E81DF) --------------------------------------------------------
sub_4E81DF(int player, int card, event_t event)
//----- (4E81FE) --------------------------------------------------------
is_tapped(int player, int card)
//----- (4E8224) --------------------------------------------------------
add_one_mana_any_color(int player, int bigcard_card)
//----- (4E8A00) --------------------------------------------------------
C_card_morphling(int player, int card, event_t event)
//----- (4E9240) --------------------------------------------------------
C_card_capsize(int player, int card, event_t event)
//----- (4E9300) --------------------------------------------------------
sub_4E9300(int player, int card, int a3, int a4, int a5, int a6, int a7, int amount)
//----- (4E9460) --------------------------------------------------------
TENTATIVE_buyback(int a1<ebx>, int a2<edi>, card_instance_t *a3<esi>, int player, int card, int event, int a7, int a8, int a9, int a10, int a11, int (__cdecl *a12)(_DWORD, _DWORD, _DWORD, _DWORD, _DWORD, _DWORD, _DWORD))
//----- (4E9564) --------------------------------------------------------
sub_4E9564(int a1)
//----- (1001088) --------------------------------------------------------
sub_1001088()
//----- (10010EC) --------------------------------------------------------
sub_10010EC()
//----- (1001170) --------------------------------------------------------
sub_1001170<eax>(int result<eax>)
//----- (1001178) --------------------------------------------------------
sub_1001178<eax>(int a1<eax>, int a2<edx>)
//----- (10011D4) --------------------------------------------------------
sub_10011D4<eax>(int result<eax>)
//----- (1001208) --------------------------------------------------------
sub_1001208<eax>(int a1<eax>, int a2<edx>, int a3<ecx>)
//----- (1001290) --------------------------------------------------------
sub_1001290<al>(int a1<eax>, int a2<edx>)
//----- (1001354) --------------------------------------------------------
sub_1001354<eax>(signed int a1<eax>, int a2<edx>)
//----- (10013B8) --------------------------------------------------------
sub_10013B8<eax>(int a1<eax>, int a2<edx>, int a3<ecx>)
//----- (1001430) --------------------------------------------------------
sub_1001430<eax>(unsigned int a1<eax>, int a2<edx>, int a3<ecx>)
//----- (10014F8) --------------------------------------------------------
sub_10014F8<eax>(int a1<eax>, int a2<edx>, int a3<ecx>)
//----- (10015B8) --------------------------------------------------------
sub_10015B8<eax>(int a1<eax>, int a2<edx>, int a3<ecx>)
//----- (1001674) --------------------------------------------------------
sub_1001674<eax>(int a1<eax>, int a2<edx>)
//----- (100171C) --------------------------------------------------------
sub_100171C<eax>(int a1<eax>, int a2<edx>, int a3<ecx>)
//----- (1001860) --------------------------------------------------------
sub_1001860<al>(int a1<eax>, int a2<edx>, int a3<ecx>)
//----- (10018F4) --------------------------------------------------------
sub_10018F4(void *this)
//----- (10019CC) --------------------------------------------------------
sub_10019CC()
//----- (1001ACC) --------------------------------------------------------
sub_1001ACC<eax>(int a1<eax>)
//----- (1001B60) --------------------------------------------------------
sub_1001B60<eax>(unsigned int a1<eax>)
//----- (1001BA4) --------------------------------------------------------
sub_1001BA4<eax>(int result<eax>, signed int a2<edx>)
//----- (1001BE0) --------------------------------------------------------
sub_1001BE0<eax>(int a1<eax>, void *a2<ecx>)
//----- (1001C04) --------------------------------------------------------
sub_1001C04<eax>(int a1<eax>, signed int a2<edx>, void *a3<ecx>)
//----- (1001C2C) --------------------------------------------------------
sub_1001C2C<eax>(int a1<eax>)
//----- (1001CA4) --------------------------------------------------------
sub_1001CA4<eax>(int a1<eax>)
//----- (1001CE4) --------------------------------------------------------
sub_1001CE4<eax>(unsigned int a1<eax>, int a2<edx>)
//----- (1001DA0) --------------------------------------------------------
sub_1001DA0<eax>(int result<eax>, int a2<edx>)
//----- (1001E98) --------------------------------------------------------
sub_1001E98(void *this)
//----- (1001EE4) --------------------------------------------------------
sub_1001EE4<al>(int a1<eax>, void *a2<ecx>)
//----- (1001F70) --------------------------------------------------------
sub_1001F70<eax>(int a1<eax>)
//----- (1001F9C) --------------------------------------------------------
sub_1001F9C<eax>(int a1<eax>, int a2<edx>)
//----- (1001FD0) --------------------------------------------------------
sub_1001FD0<eax>(signed int a1<eax>)
//----- (1002008) --------------------------------------------------------
sub_1002008<eax>(signed int a1<eax>)
//----- (1002150) --------------------------------------------------------
sub_1002150<eax>(signed int a1<eax>, void *a2<ecx>)
//----- (10022FC) --------------------------------------------------------
sub_10022FC<eax>(int a1<eax>, void *a2<ecx>)
//----- (10024D4) --------------------------------------------------------
sub_10024D4<al>(int a1<eax>, int a2<edx>, void *a3<ecx>)
//----- (10026D8) --------------------------------------------------------
sub_10026D8<eax>(int a1<eax>, signed int a2<edx>, void *a3<ecx>)
//----- (10027A4) --------------------------------------------------------
sub_10027A4<eax>(int a1<eax>, int a2<ecx>)
//----- (10027D4) --------------------------------------------------------
sub_10027D4<eax>(int a1<eax>)
//----- (10027F4) --------------------------------------------------------
sub_10027F4(int a1<eax>, int a2<edx>, int a3<ecx>)
//----- (1002800) --------------------------------------------------------
sub_1002800(char a1<al>, int a2<edx>, int a3<ecx>)
//----- (100284C) --------------------------------------------------------
sub_100284C(char a1<al>, int a2<ecx>)
//----- (1002858) --------------------------------------------------------
sub_1002858<eax>(int a1<eax>)
//----- (1002868) --------------------------------------------------------
sub_1002868<eax>(const void *a1<eax>, void *a2<edx>, signed int a3<ecx>)
//----- (10028A8) --------------------------------------------------------
sub_10028A8<eax>(int a1<eax>)
//----- (1002900) --------------------------------------------------------
sub_1002900<eax>(int result<eax>, int a2<edx>)
//----- (10029DC) --------------------------------------------------------
sub_10029DC()
//----- (1002A0C) --------------------------------------------------------
sub_1002A0C()
//----- (1002AD0) --------------------------------------------------------
sub_1002AD0()
//----- (1002ADC) --------------------------------------------------------
sub_1002ADC<eax>(int result<eax>)
//----- (1002AE8) --------------------------------------------------------
sub_1002AE8()
//----- (1002B04) --------------------------------------------------------
sub_1002B04<eax>(int result<eax>, int a2<edx>)
//----- (1002B40) --------------------------------------------------------
sub_1002B40<eax>(int result<eax>, int a2<ebx>)
//----- (1002B54) --------------------------------------------------------
sub_1002B54()
//----- (1002B74) --------------------------------------------------------
sub_1002B74<eax>(int result<eax>, int a2<edx>, int a3<ecx>)
//----- (1002B9C) --------------------------------------------------------
sub_1002B9C(int a1, int a2)
//----- (1002BB8) --------------------------------------------------------
sub_1002BB8()
//----- (1002D04) --------------------------------------------------------
sub_1002D04(int a1, int a2)
//----- (1002D3C) --------------------------------------------------------
very_broken__sub_1002D3C(int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8, int a9, int a10, int a11, int a12)
//----- (1002D90) --------------------------------------------------------
very_broken__sub_1002D90(int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8, int a9, int a10, int a11, int a12)
//----- (1002DC0) --------------------------------------------------------
sub_1002DC0(unsigned int a1, int a2, int a3)
//----- (1002DD8) --------------------------------------------------------
sub_1002DD8(int this, int a2)
//----- (1002E78) --------------------------------------------------------
sub_1002E78(int a1, int a2)
//----- (1002F18) --------------------------------------------------------
sub_1002F18<eax>(int a1<ebp>)
//----- (1002F38) --------------------------------------------------------
sub_1002F38()
//----- (1002F60) --------------------------------------------------------
sub_1002F60()
//----- (1002FD0) --------------------------------------------------------
sub_1002FD0()
//----- (1003040) --------------------------------------------------------
sub_1003040<eax>(int a1<eax>, int a2<edx>, int a3<ecx>, int a4<ebx>, int a5<ebp>, int a6<edi>, int a7<esi>, void (__cdecl *a8)(_DWORD))
//----- (1003108) --------------------------------------------------------
sub_1003108(int a1)
//----- (1003164) --------------------------------------------------------
sub_1003164(int a1, int a2, int a3)
//----- (1003194) --------------------------------------------------------
sub_1003194(void *this)
//----- (1003220) --------------------------------------------------------
sub_1003220(int this)
//----- (1003300) --------------------------------------------------------
sub_1003300(int a1<eax>, int a2<ecx>)
//----- (100330C) --------------------------------------------------------
very_broken__sub_100330C(int a1<eax>, int a2<ecx>)
//----- (1003318) --------------------------------------------------------
sub_1003318<eax>(int result<eax>)
//----- (100333C) --------------------------------------------------------
sub_100333C<eax>(int a1<eax>, int a2<ecx>)
//----- (1003368) --------------------------------------------------------
sub_1003368<eax>(int a1<eax>, const void *a2<edx>, int a3<ecx>)
//----- (1003398) --------------------------------------------------------
sub_1003398<eax>(int a1<eax>, const void *a2<edx>, int a3<ecx>)
//----- (10033B0) --------------------------------------------------------
sub_10033B0(int this)
//----- (10033B8) --------------------------------------------------------
sub_10033B8<eax>(int a1<eax>)
//----- (1003414) --------------------------------------------------------
sub_1003414<eax>(int result<eax>)
//----- (1003424) --------------------------------------------------------
sub_1003424<eax>(int a1<eax>)
//----- (10034A8) --------------------------------------------------------
sub_10034A8<eax>(int a1<eax>)
//----- (100351C) --------------------------------------------------------
sub_100351C()
//----- (1003573) --------------------------------------------------------
sub_1003573<eax>(int a1<eax>, int a2<ecx>, int a3<ebp>, int a4<edi>)
//----- (1003674) --------------------------------------------------------
sub_1003674<eax>(int a1<eax>)
//----- (100368C) --------------------------------------------------------
sub_100368C()
//----- (1003694) --------------------------------------------------------
sub_1003694()
//----- (10036E4) --------------------------------------------------------
sub_10036E4()
//----- (1003710) --------------------------------------------------------
sub_1003710()
//----- (1003744) --------------------------------------------------------
sub_1003744()
//----- (1003768) --------------------------------------------------------
sub_1003768()
//----- (10037A8) --------------------------------------------------------
sub_10037A8()
//----- (10037B4) --------------------------------------------------------
sub_10037B4<eax>(int a1<eax>, int a2<ebx>, int a3<ebp>, int a4<edi>, int a5<esi>)
//----- (10037FC) --------------------------------------------------------
sub_10037FC()
//----- (1003834) --------------------------------------------------------
sub_1003834(char a1<cf>, char a2<zf>)
//----- (1003839) --------------------------------------------------------
C_card_civic_wayfinder(int player, int card, event_t event)
//----- (100389B) --------------------------------------------------------
C_civic_wayfinder_tutor_basic_land<eax>(int a1<ebp>)
//----- (10038D9) --------------------------------------------------------
C_tutor_basic_land(int player, int card, event_t event)
//----- (1180000) --------------------------------------------------------
C_card_kami_of_the_crescent_moon(int player, int card, event_t event)
//----- (1180028) --------------------------------------------------------
sub_1180028(int player, int card, int a5)
//----- (1180070) --------------------------------------------------------
C_card_priest_of_gix(int player, int card, event_t event)
//----- (11800D8) --------------------------------------------------------
sub_11800D8(int a1, int a2, int a3)
//----- (118011C) --------------------------------------------------------
sub_118011C(int a2, int a3, int a4)
//----- (1200000) --------------------------------------------------------
C_card_voltaic_key(int player, int card, event_t event)
//----- (1200170) --------------------------------------------------------
C_card_izzet_signet(int player, int card, event_t event)
//----- (1200195) --------------------------------------------------------
C_signet(int player, int card, event_t event, color_t color1, color_t color2)
//----- (12003CB) --------------------------------------------------------
C_card_orzhov_basilica(int player, int card, event_t event)
//----- (12003EF) --------------------------------------------------------
taps_when_comes_into_play(int player, int card, event_t event)
//----- (1200462) --------------------------------------------------------
C_cip_bounce_land(int player, int card, event_t event)
//----- (120058D) --------------------------------------------------------
C_two_mana_land(int player, int card, event_t event, color_t color1, color_t color2)
//----- (12006F1) --------------------------------------------------------
C_two_mana(int player, int card, event_t event, color_t color1, color_t color2)
//----- (120075F) --------------------------------------------------------
C_card_electrolyze(int player, int card, event_t event)
//----- (120077E) --------------------------------------------------------
C_card_fire(int player, int card, event_t event)
//----- (1200A5D) --------------------------------------------------------
C_wildfire_sac_4_lands(int player, int card, int event)
//----- (1200B67) --------------------------------------------------------
C_card_wildfire(int player, int card, event_t event)
//----- (1200BAC) --------------------------------------------------------
C_wildfire_4_damage(int player, int card, event_t event)
//----- (1200C6E) --------------------------------------------------------
C_card_jokulhaups(int player, int card, event_t event)
//----- (1200C91) --------------------------------------------------------
UNKNOWN_card_34(int player, int card, event_t event)
//----- (1200D4A) --------------------------------------------------------
C_card_order_of_leitbur(int player, int card, event_t event)
//----- (1200D69) --------------------------------------------------------
C_white_pump_knight(int player, int card, event_t event)
//----- (1201378) --------------------------------------------------------
C_card_order_of_the_ebon_hand(int player, int card, event_t event)
//----- (1201397) --------------------------------------------------------
C_black_pump_knight(int player, int card, event_t event)
//----- (1201994) --------------------------------------------------------
C_card_soul_warden(int player, int card, event_t event)
//----- (1201A33) --------------------------------------------------------
C_card_meditate(int player, int card, event_t event)
//----- (1201A68) --------------------------------------------------------
C_time_walk_but_dont_bury_self_at_resolution(int player, int card, event_t event)
//----- (1201B52) --------------------------------------------------------
UNKNOWN_card_41(int player, int card, event_t event)
//----- (1201C59) --------------------------------------------------------
C_card_aysen_crusader(int player, int card, event_t event)
//----- (1201C76) --------------------------------------------------------
C_aysen_crusader_soldier(int player, int card, char a3)
//----- (1201CDD) --------------------------------------------------------
C_aysen_crusader_warrior(int player, int card, char a3)
//----- (1201D44) --------------------------------------------------------
sub_1201D44(int player, int card, char a3)
//----- (1201DE5) --------------------------------------------------------
C_card_eron_the_relentless(int player, int card, event_t event)
//----- (1201E26) --------------------------------------------------------
C_regenerate_for_RRR(int a1, int a2, int a3)
//----- (1201E56) --------------------------------------------------------
C_card_soraya_the_falconer(int player, int card, event_t event)
//----- (1201E73) --------------------------------------------------------
C_card_soraya_the_falconer_nonlegendary(int player, int card, event_t event)
//----- (1201F23) --------------------------------------------------------
sub_1201F23(int player, int card, int a5)
//----- (1202040) --------------------------------------------------------
C_card_malach_of_the_dawn(int player, int card, event_t event)
//----- (1202070) --------------------------------------------------------
C_card_wild_aesthir(int player, int card, event_t event)
//----- (12021C5) --------------------------------------------------------
sub_12021C5(int player, int card, char a3)
//----- (120240E) --------------------------------------------------------
C_card_wildfire_emissary(int player, int card, event_t event)
//----- (1202658) --------------------------------------------------------
C_card_zuberi_golden_feather(int player, int card, event_t event)
//----- (1202675) --------------------------------------------------------
C_zuberi_golden_feather_helper(int player, int card, char a3)
//----- (1202718) --------------------------------------------------------
C_card_jeska_warrior_adept(int player, int card, event_t event)
//----- (1202759) --------------------------------------------------------
card_balduvian_war_makers(int player, int card, event_t event)
//----- (1202789) --------------------------------------------------------
sub_1202789(int player, int a4, char a5)
//----- (12027F5) --------------------------------------------------------
sub_12027F5(int player, int card, event_t event)
//----- (1202812) --------------------------------------------------------
C_card_order_of_yawgmoth(int player, int card, event_t event)
//----- (120282F) --------------------------------------------------------
C_card_vitalizing_cascade(int player, int card, event_t event)
//----- (120284C) --------------------------------------------------------
UNKNOWN_card_37(int player, int card, event_t event)
//----- (120294E) --------------------------------------------------------
sub_120294E(int player, int card, char a5)
//----- (1202996) --------------------------------------------------------
C_card_darksteel_ingot(int player, int card, event_t event)
//----- (12029B5) --------------------------------------------------------
C_indestructible(int player, int card, event_t event)
//----- (1202AAC) --------------------------------------------------------
very_broken__sub_1202AAC<eax>(int a1<eax>, int a2<ebp>)
//----- (1202B9A) --------------------------------------------------------
C_no_other_card_in_play_with_same_csvid_bury_this_if_negative_iid(int player, int card)
//----- (1202C2F) --------------------------------------------------------
C_card_kokusho_the_evening_star(int player, int card, event_t event)
//----- (1202C4E) --------------------------------------------------------
C_kokusho_helper(int player, int card, event_t event)
//----- (1202CB8) --------------------------------------------------------
C_card_merfolk_looter(int player, int card, event_t event)
//----- (1202DD6) --------------------------------------------------------
sub_1202DD6(int player, int card, event_t event)
//----- (1202E00) --------------------------------------------------------
C_card_solkanar_the_swamp_king(int player, int card, event_t event)
//----- (1202E1D) --------------------------------------------------------
C_solkanar_the_swamp_king_helper(int a3, int a4, int a5)
//----- (1202E3D) --------------------------------------------------------
sub_1202E3D(int a3, int a4, int a5)
//----- (1202E5D) --------------------------------------------------------
sub_1202E5D(int player, int card, int a5, color_t color)
//----- (1202FF4) --------------------------------------------------------
very_broken__sub_1202FF4<eax>(int a1<eax>, int a2<ebx>)
//----- (12031A8) --------------------------------------------------------
C_card_tahngarth_talruum_hero(int player, int card, event_t event)
//----- (12031E9) --------------------------------------------------------
C_card_tahngarth_talruum_hero_nonlegendary(int player, int card, event_t event)
//----- (1203347) --------------------------------------------------------
sub_1203347(int a1, int a2, int a3)
//----- (1203430) --------------------------------------------------------
UNKNOWN_card_29(int player, int card, event_t event)
//----- (1203454) --------------------------------------------------------
modify_cost_plus_2UB(int player, int card, event_t event)
//----- (1203483) --------------------------------------------------------
UNKNOWN_card_28(int player, int card, event_t event)
//----- (12034A2) --------------------------------------------------------
UNKNOWN_card_27(int player, int card, event_t event)
//----- (1203543) --------------------------------------------------------
UNKNOWN_card_35(int player, int card, event_t event)
//----- (12036C4) --------------------------------------------------------
C_card_arcbound_ravager(int player, int card, event_t event)
//----- (12036E3) --------------------------------------------------------
sub_12036E3(int preferred_controller, int a4, int a5)
//----- (1203847) --------------------------------------------------------
card_pattern_of_rebirth(int player, int card, event_t event)
//----- (12039A8) --------------------------------------------------------
C_card_phyrexian_ghoul(int player, int card, event_t event)
//----- (1203BC9) --------------------------------------------------------
C_card_nicol_bolas(int player, int card, event_t event)
//----- (1203BF0) --------------------------------------------------------
C_nicol_bolas_helper(int player, int card, event_t event)
//----- (1203C79) --------------------------------------------------------
C_nicol_bolas_helper2(int player, int card, int a5)
//----- (1203D80) --------------------------------------------------------
C_rancor_2_0_and_trample(int player, int card, event_t event)
//----- (1203F1F) --------------------------------------------------------
C_card_rancor(int player, int card, event_t event)
//----- (1203F3E) --------------------------------------------------------
C_rancor_bounce_from_gy_and_manaproducer(int player, int card, int event)
//----- (1203F5D) --------------------------------------------------------
C_rancor_bounce_from_gy(int player, int card, event_t event)
//----- (1204026) --------------------------------------------------------
C_card_darksteel_citadel(int player, int card, event_t event)
//----- (1204043) --------------------------------------------------------
C_thunk_mana_producer_sound_on_resolve(int player, int card, event_t event)
//----- (120405C) --------------------------------------------------------
sub_120405C(int player, int card, int a5)
//----- (12041CC) --------------------------------------------------------
was_labelled_card_aladdin2(int player, int card, event_t event)
//----- (120456F) --------------------------------------------------------
labelled_card_animate_artifact55(int player, int card, event_t event)
//----- (12046EE) --------------------------------------------------------
sub_12046EE(int a1<eax>, int a2, int a3, int a4, int a5)
//----- (12047D2) --------------------------------------------------------
sub_12047D2(int a1<ebp>)
//----- (120482B) --------------------------------------------------------
is_creature(int iid)
//----- (12049E4) --------------------------------------------------------
sub_12049E4(int a1<ebp>)
//----- (1204A25) --------------------------------------------------------
sub_1204A25(int a1<eax>, int a2<ecx>)
//----- (1204A52) --------------------------------------------------------
sub_1204A52(char a1<zf>)
//----- (1205001) --------------------------------------------------------
unused_card_somber_hoverguard(int player, int card, event_t event)
//----- (1205055) --------------------------------------------------------
C_card_frozen_aether(int player, int card, event_t event)
//----- (12050F2) --------------------------------------------------------
C_card_thoughtcast(int player, int card, event_t event)
//----- (120517E) --------------------------------------------------------
C_card_ageless_sentinels(int player, int card, event_t event)
//----- (12051E6) --------------------------------------------------------
C_card_fists_of_the_anvil(int player, int card, event_t event)
//----- (1205325) --------------------------------------------------------
sub_1205325(int a1, int player, int card, int a4)
//----- (1205390) --------------------------------------------------------
C_card_charging_troll(int player, int card, event_t event)
//----- (12053FC) --------------------------------------------------------
C_card_ohran_viper(int player, int card, int event)
//----- (12055BD) --------------------------------------------------------
card_arc_lightning(int player, int card, event_t event)
//----- (120588D) --------------------------------------------------------
C_card_jhessian_infiltrator(int player, int card, event_t event)
//----- (12058BF) --------------------------------------------------------
sub_12058BF(int a1, int a2, char a3)
//----- (1270000) --------------------------------------------------------
sub_1270000(int a1<ebp>, int a2, int a3, int a4, int a5)
//----- (1270040) --------------------------------------------------------
sub_1270040(card_instance_t *a1, card_instance_t *a2)
/*
^(//----- \([0-9A-F]+\) -----+
)(//.*
)*(.+
)([a-zA-Z_].+
)((|[^
/].*)
)*

to \1\4
 */
