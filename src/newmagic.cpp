#include "structs.h"
#include "awake.h"
#include "db.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "newmagic.h"
#include "utils.h"
#include "screen.h"
#include "constants.h"
#include "olc.h"
#include <stdlib.h>

#define POWER(name) void (name)(struct char_data *ch, struct char_data *spirit, struct spirit_data *spiritdata, char *arg)
#define SPELLCASTING 0
#define CONJURING 1
#define FAILED_CAST "You fail to bind the mana to your will.\r\n"

extern void die(struct char_data *ch);
extern void damage_equip(struct char_data *ch, struct char_data *vict, int power, int type);
extern void damage_obj(struct char_data *ch, struct obj_data *obj, int power, int type);
extern void check_killer(struct char_data * ch, struct char_data * vict);
struct char_data *find_spirit_by_id(int spiritid, long playerid)
{
  for (struct char_data *ch = character_list; ch; ch = ch->next)
    if (IS_NPC(ch) && (GET_RACE(ch) == CLASS_SPIRIT || GET_RACE(ch) == CLASS_ELEMENTAL) &&
        GET_ACTIVE(ch) == playerid && GET_GRADE(ch) == spiritid)
      return ch;
  return NULL;
}

void mob_magic(struct char_data *ch)
{}

void end_sustained_spell(struct char_data *ch, struct sustain_data *sust)
{
  struct sustain_data *temp, *vsust;
  if (sust->other)
    for (vsust = GET_SUSTAINED(sust->other); vsust; vsust = vsust->next)
      if (sust->caster != vsust->caster && vsust->other == ch && vsust->idnum == sust->idnum)
      {
        if (vsust->spirit) {
          GET_SUSTAINED_FOCI(sust->other)--;
          GET_SUSTAINED_NUM(vsust->spirit--);
        }
        REMOVE_FROM_LIST(vsust, GET_SUSTAINED(sust->other), next);
        delete [] vsust;
        break;
      }
  spell_modify(sust->caster ? sust->other : ch, sust, FALSE);
  REMOVE_FROM_LIST(sust, GET_SUSTAINED(ch), next);
  if (sust->focus)
  {
    GET_SUSTAINED_FOCI(sust->caster ? ch : sust->other)--;
    GET_FOCI(sust->caster ? ch : sust->other)--;
    GET_OBJ_VAL(sust->focus, 4)--;
  }
  if (sust->spirit)
  {
    GET_SUSTAINED_FOCI(ch)--;
    GET_SUSTAINED_NUM(sust->spirit)--;
    GET_SUSTAINED(sust->spirit) = NULL;
  }
  GET_SUSTAINED_NUM(sust->caster ? ch : sust->other)--;
  send_to_char(sust->caster ? ch : sust->other, "You stop sustaining %s.\r\n", spells[sust->spell].name);
  delete [] sust;
}

void totem_bonus(struct char_data *ch, int action, int type, int &target, int &skill)
{
  extern struct time_info_data time_info;
  if (GET_TOTEM(ch) == TOTEM_OWL)
  {
    if (time_info.hours > 6 || time_info.hours < 19)
      target += 2;
    else
      skill += 2;
  } else if (GET_TOTEM(ch) == TOTEM_RAVEN)
  {
    if (ROOM_FLAGGED(ch->in_room, ROOM_INDOORS) || SECT(ch->in_room) == SPIRIT_HEARTH)
      target += 1;
  } else if (GET_TOTEM(ch) == TOTEM_SNAKE && FIGHTING(ch))
    skill--;

  if (action == SPELLCASTING)
  {
    type = spells[type].category;
    switch (GET_TOTEM(ch)) {
    case TOTEM_BEAR:
      if (type == HEALTH)
        skill += 2;
      break;
    case TOTEM_BUFFALO:
      if (type == HEALTH)
        skill += 2;
      else if (type == ILLUSION)
        skill--;
      break;
    case TOTEM_CAT:
      if (type == ILLUSION)
        skill += 2;
      break;
    case TOTEM_DOG:
    case TOTEM_EAGLE:
      if (type == DETECTION)
        skill += 2;
      break;
    case TOTEM_DOLPHIN:
      if (type == DETECTION)
        skill += 2;
      else if (type == COMBAT)
        skill--;
      break;
    case TOTEM_GATOR:
      if (type == COMBAT || type == DETECTION)
        skill += 2;
      else if (type == ILLUSION)
        skill--;
      break;
    case TOTEM_LION:
      if (type == COMBAT)
        skill += 2;
      else if (type == HEALTH)
        skill--;
    case TOTEM_MOUSE:
      if (type == DETECTION || type == HEALTH)
        skill += 2;
      else if (type == COMBAT)
        skill -= 2;
      break;
    case TOTEM_RACCOON:
      if (type == MANIPULATION)
        skill += 2;
      else if (type == COMBAT)
        skill--;
      break;
    case TOTEM_RAT:
      if (type == DETECTION || type == ILLUSION)
        skill += 2;
      else if (type == COMBAT)
        skill--;
    case TOTEM_RAVEN:
      if (type == MANIPULATION)
        skill += 2;
      break;
    case TOTEM_SHARK:
    case TOTEM_WOLF:
      if (type == COMBAT || type == DETECTION)
        skill += 2;
      break;
    case TOTEM_SNAKE:
      if (type == DETECTION || type == HEALTH || type == ILLUSION)
        skill += 2;
      break;

    }
  } else if (action == CONJURING)
  {
    switch (GET_TOTEM(ch)) {
    case TOTEM_BEAR:
      if (type == SPIRIT_FOREST)
        skill += 2;
      break;
    case TOTEM_BUFFALO:
    case TOTEM_LION:
      if (type == SPIRIT_PRAIRIE)
        skill += 2;
      break;
    case TOTEM_CAT:
    case TOTEM_RAT:
    case TOTEM_RACCOON:
      if (type == SPIRIT_CITY)
        skill += 2;
      break;
    case TOTEM_DOG:
    case TOTEM_MOUSE:
      if (type == SPIRIT_FIELD || type == SPIRIT_HEARTH)
        skill += 2;
      break;
    case TOTEM_DOLPHIN:
    case TOTEM_SHARK:
      if (type == SPIRIT_SEA)
        skill += 2;
      break;
    case TOTEM_EAGLE:
    case TOTEM_RAVEN:
      if (type == SPIRIT_MIST || type == SPIRIT_STORM || type == SPIRIT_WIND)
        skill += 2;
      break;
    case TOTEM_GATOR:
    case TOTEM_SNAKE:
    case TOTEM_WOLF:
      if (GET_TOTEMSPIRIT(ch) == type)
        skill += 2;
      break;
    }
  }
}

void end_spirit_existance(struct char_data *ch, bool message)
{
  for (struct descriptor_data *d = descriptor_list; d; d = d->next)
    if (d->character && GET_IDNUM(d->character) == GET_ACTIVE(ch))
    {
      for (struct spirit_data *spirit = GET_SPIRIT(d->character); spirit; spirit = spirit->next)
        if (spirit->id == GET_GRADE(ch)) {
          struct spirit_data *temp;
          REMOVE_FROM_LIST(spirit, GET_SPIRIT(d->character), next);
          delete [] spirit;
          break;
        }
      GET_NUM_SPIRITS(d->character)--;
      if (message)
        send_to_char(d->character, "%s returns to the metaplanes, it's existance in this world finished.\r\n", CAP(GET_NAME(ch)));
      break;
    }
  extract_char(ch);
}

void elemental_fulfilled_services(struct char_data *ch, struct char_data *mob, struct spirit_data *spirit)
{
  if (spirit->services < 1 && !(MOB_FLAGGED(mob, MOB_SPIRITGUARD) || MOB_FLAGGED(mob, MOB_STUDY) || GET_SUSTAINED(mob) || GET_SUSTAINED_NUM(mob) || FIGHTING(mob)))
  {
    send_to_char(ch, "It's services fulfilled, %s departs to the metaplanes.\r\n", CAP(GET_NAME(mob)));
    end_spirit_existance(mob, FALSE);
  }
}

bool conjuring_drain(struct char_data *ch, int force)
{
  int drain = 0;
  if (force <= GET_CHA(ch) / 2)
    drain = LIGHT;
  else if (force <= GET_CHA(ch))
    drain = MODERATE;
  else if (force > GET_CHA(ch) * 1.5)
    drain = DEADLY;
  else
    drain = SERIOUS;
  drain = convert_damage(stage(-success_test(GET_CHA(ch), force), drain));
  if (force > GET_MAG(ch) / 100)
    GET_PHYSICAL(ch) -= drain *100;
  else
  {
    GET_MENTAL(ch) -= drain * 100;
    if (GET_MENTAL(ch) < 0) {
      GET_PHYSICAL(ch) += GET_MENTAL(ch);
      GET_MENTAL(ch) = 0;
    }
  }
  update_pos(ch);
  if ((GET_POS(ch) <= POS_STUNNED) && (GET_POS(ch) > POS_DEAD))
  {
    if (FIGHTING(ch))
      stop_fighting(ch);
    send_to_char("You are unable to resist the drain from conjuring and fall unconcious!\r\n", ch);
    act("$n collapses unconscious from conjuring!", FALSE, ch, 0, 0, TO_ROOM);
  } else if (GET_POS(ch) == POS_DEAD)
  {
    if (FIGHTING(ch))
      stop_fighting(ch);
    send_to_char("The energy from the conjuring ritual overloads your body with energy, killing you...\r\n", ch);
    act("$n suddenly collapases, dead from the energy of the conjuring ritual!", FALSE, ch, 0, 0, TO_ROOM);
    die(ch);
    return TRUE;
  }
  return FALSE;
}

bool spell_drain(struct char_data *ch, int type, int force, int damage)
{
  int target = (int)(force / 2);
  target += spells[type].drainpower;
  if (!damage)
    damage = spells[type].draindamage;
  else
    damage += spells[type].draindamage + 3;
  damage = convert_damage(stage(-success_test(GET_WIL(ch) + GET_DRAIN(ch), target), damage));
  if (force > GET_MAG(ch) / 100 || IS_PROJECT(ch))
    GET_PHYSICAL(ch) -= damage *100;
  else
  {
    GET_MENTAL(ch) -= damage * 100;
    if (GET_MENTAL(ch) < 0) {
      GET_PHYSICAL(ch) += GET_MENTAL(ch);
      GET_MENTAL(ch) = 0;
    }
  }
  update_pos(ch);
  if ((GET_POS(ch) <= POS_STUNNED) && (GET_POS(ch) > POS_DEAD))
  {
    if (FIGHTING(ch))
      stop_fighting(ch);
    send_to_char("You are unable to resist the drain from spell casting and fall unconcious!\r\n", ch);
    act("$n collapses unconscious from the strain of spell casting!", FALSE, ch, 0, 0, TO_ROOM);
  } else if (GET_POS(ch) == POS_DEAD)
  {
    if (FIGHTING(ch))
      stop_fighting(ch);
    send_to_char("The feedback from spell casting floods your body, killing you...\r\n", ch);
    act("$n suddenly collapases, dead from the drain of spell casting!", FALSE, ch, 0, 0, TO_ROOM);
    die(ch);
    return TRUE;
  }
  return FALSE;


}

struct char_data *find_target_at_range(struct char_data *ch, char *name, char *direction)
{
  return NULL;
}
void create_sustained(struct char_data *ch, struct char_data *vict, int spell, int force, int sub, int success, int drain)
{
  struct obj_data *focus = NULL;
  for (int i = 0; !focus && i < NUM_WEARS; i++)
    if (GET_EQ(vict, i) && GET_OBJ_TYPE(GET_EQ(vict, i)) == ITEM_FOCUS && !GET_OBJ_VAL(GET_EQ(vict, i), 4) &&
        GET_OBJ_VAL(GET_EQ(vict, i), 3) == spell && GET_OBJ_VAL(GET_EQ(vict, i), 1) >= force)
      focus = GET_EQ(vict, i);
  GET_SUSTAINED_NUM(ch)++;
  if (focus)
  {
    GET_SUSTAINED_FOCI(ch)++;
    GET_OBJ_VAL(focus, 4)++;
    GET_FOCI(ch)++;
  }
  struct sustain_data *sust = new sustain_data;
  sust->spell = spell;
  sust->subtype = sub;
  sust->force = force;
  sust->success = success;
  sust->other = vict;
  sust->caster = TRUE;
  sust->drain = drain;
  sust->idnum = number(0, 1000);
  sust->next = GET_SUSTAINED(ch);
  sust->focus = focus;
  GET_SUSTAINED(ch) = sust;
  struct sustain_data *vsust = new sustain_data;
  *vsust = *sust;
  vsust->caster = FALSE;
  vsust->other = ch;
  vsust->next = GET_SUSTAINED(vict);
  vsust->focus = focus;
  GET_SUSTAINED(vict) = vsust;
  spell_modify(vict, vsust, TRUE);
}

void spell_bonus(struct char_data *ch, int spell, int &skill, int &target)
{
  int origskill = skill;
  if (GET_TRADITION(ch) == TRAD_SHAMANIC)
    totem_bonus(ch, SPELLCASTING, spell, target, skill);
  else if (GET_TRADITION(ch) == TRAD_HERMETIC && GET_SPIRIT(ch) && spells[spell].category != HEALTH)
  {
    for (struct spirit_data *spirit = GET_SPIRIT(ch); spirit; spirit = spirit->next)
      if (((spells[spell].category == MANIPULATION && spirit->type == ELEM_EARTH) ||
           (spells[spell].category == COMBAT && spirit->type == ELEM_FIRE) ||
           (spells[spell].category == ILLUSION && spirit->type == ELEM_WATER) ||
           (spells[spell].category == DETECTION && spirit->type == ELEM_AIR)) &&
          spirit->called == TRUE) {
        struct char_data *mob = find_spirit_by_id(spirit->id, GET_IDNUM(ch));
        if (mob && MOB_FLAGGED(mob, MOB_AIDSORCERY)) {
          send_to_char(ch, "%s aids your spell casting.\r\n", CAP(GET_NAME(mob)));
          MOB_FLAGS(mob).RemoveBit(MOB_AIDSORCERY);
          skill += GET_LEVEL(mob);
        }
      }
  }
  if (GET_FOCI(ch) > 0)
  {
    for (int i = 0; i < NUM_WEARS && skill == origskill; i++)
      if (GET_EQ(ch, i) && GET_OBJ_TYPE(GET_EQ(ch, i)) == ITEM_FOCUS)
        switch (GET_OBJ_VAL(GET_EQ(ch, i), 0)) {
        case FOCI_EXPENDABLE:
          if (spells[spell].category == GET_OBJ_VAL(GET_EQ(ch, i), 3)) {
            skill += GET_OBJ_VAL(GET_EQ(ch, i), 1);
            extract_obj(GET_EQ(ch, i));
          }
          break;
        case FOCI_SPEC_SPELL:
          if (spell == GET_OBJ_VAL(GET_EQ(ch, i), 3))
            skill += GET_OBJ_VAL(GET_EQ(ch, i), 1);
          break;
        case FOCI_SPELL_CAT:
          if (spells[spell].category == GET_OBJ_VAL(GET_EQ(ch, i), 3))
            skill += GET_OBJ_VAL(GET_EQ(ch, i), 1);
          break;
        }
  }
}

bool find_duplicate_spell(struct char_data *ch, struct char_data *vict, int spell, int sub)
{
  if (spells[spell].duration == INSTANT)
    return FALSE;
  struct sustain_data *sus;
  if (!vict || vict == ch)
    sus = GET_SUSTAINED(ch);
  else
    sus = GET_SUSTAINED(vict);
  for (; sus; sus = sus->next)
    if (!sus->caster && sus->spell == spell && sus->subtype == sub)
    {
      send_to_char(ch, "%s are already affected by that spell.\r\n", (!vict || vict == ch) ? "You" : "They");
      return TRUE;
    }
  return FALSE;
}

bool check_spell_victim(struct char_data *ch, struct char_data *vict, int spell)
{
  if (!vict)
    send_to_char("Cast spell at which target?\r\n", ch);
  else if (((IS_PROJECT(ch) || IS_ASTRAL(ch)) && !(IS_DUAL(vict) || IS_ASTRAL(vict) || IS_PROJECT(vict))) ||
           ((IS_PROJECT(vict) || IS_ASTRAL(vict)) && !(IS_DUAL(ch) || IS_ASTRAL(ch) || IS_PROJECT(ch))))
    send_to_char("They aren't accessible from this plane.\r\n", ch);
  else
    return TRUE;
  return FALSE;
}

int resist_spell(struct char_data *ch, int force, int spell, int sub)
{
  int skill = 0;
  if (sub)
    skill = GET_ATT(ch, sub);
  else if (spell == SPELL_CHAOS)
    skill = GET_INT(ch);
  else if (spells[spell].physical)
    skill = GET_BOD(ch);
  else
    skill = GET_WIL(ch);
  if (GET_TRADITION(ch) == TRAD_ADEPT)
  {
    if (GET_POWER(ch, ADEPT_MAGIC_RESISTANCE))
      skill += GET_POWER(ch, ADEPT_MAGIC_RESISTANCE);
    if (GET_POWER(ch, ADEPT_SPELL_SHROUD) && spells[spell].category == DETECTION)
      skill += GET_POWER(ch, ADEPT_SPELL_SHROUD);
    if (GET_POWER(ch, ADEPT_TRUE_SIGHT) && spells[spell].category == ILLUSION)
      skill += GET_POWER(ch, ADEPT_TRUE_SIGHT);
  }

  return success_test(skill, force);
}
void cast_combat_spell(struct char_data *ch, int spell, int force, char *arg)
{
  struct char_data *vict = NULL;
  two_arguments(arg, buf, buf1);
  int basedamage = 0;
  if (!*buf)
  {
    send_to_char("What damage level do you wish to cast that spell at?\r\n", ch);
    return;
  } else
  {
    for (basedamage = 0; *wound_name[basedamage] != '\n'; basedamage++)
      if (is_abbrev(buf, wound_name[basedamage]))
        break;
    if (basedamage > 4 || basedamage == 0) {
      send_to_char("That is not a valid damage level, please choose between Light, Moderate, Serious and Deadly.\r\n", ch);
      return;
    }
  }
  if (*buf1)
    vict = get_char_room_vis(ch, buf1);
  if (!check_spell_victim(ch, vict, spell))
    return;
  int target = modify_target(ch), skill = GET_SKILL(ch, SKILL_SORCERY) + GET_CASTING(ch), success = 0;
  spell_bonus(ch, spell, skill, target);
  if (skill == -1)
    return;
  check_killer(ch, vict);
  switch (spell)
  {
  case SPELL_MANABOLT:
    if (IS_NPC(vict) && MOB_FLAGGED(vict, MOB_INANIMATE) || (PLR_FLAGGED(ch, PLR_KILLER) && !IS_NPC(vict)))
      success = -1;
    else
      success = success_test(skill, GET_WIL(vict) + target) - resist_spell(vict, spell, force, 0);
    if (success > 0) {
      int dam = convert_damage(stage(success, basedamage));
      if (GET_MENTAL(vict) - (dam * 100) <= 0) {
        act("$n falls to the floor as ^Rblood^n erupts from every pore on $s head.", TRUE, vict, 0, 0, TO_ROOM);
        send_to_char("Your world turns to red as you feel blood flow from every pore on your head.", vict);
      } else if (GET_MENTAL(vict) - (dam * 100) <= 300) {
        act("$n grabs $s head in pain as blood flows from $s ears.", TRUE, vict, 0, 0, TO_ROOM);
        send_to_char("Your head is filled with immense pain as your ears begin to bleed.", vict);
      } else if (GET_MENTAL(vict) - (dam * 100) <= 700) {
        act("$n grabs ahold of $s head in pain.", TRUE, vict, 0, 0, TO_ROOM);
        send_to_char("A sudden flash of pain in your head causes you to reflexivly grab it.", vict);
      } else {
        act("$n seems to flinch slightly as blood trickles from $s nose.", TRUE, vict, 0, 0, TO_ROOM);
        send_to_char("Slight pain fills your mind from an unknown source.", vict);
      }
      damage(ch, vict, dam, TYPE_COMBAT_SPELL, PHYSICAL);
    } else
      send_to_char(FAILED_CAST, ch);
    spell_drain(ch, spell, force, basedamage);
    break;
  case SPELL_STUNBOLT:
    if (IS_NPC(vict) && MOB_FLAGGED(vict, MOB_INANIMATE) || (PLR_FLAGGED(ch, PLR_KILLER) && !IS_NPC(vict)))
      success = -1;
    else
      success = success_test(skill, GET_WIL(vict) + target) - resist_spell(vict, spell, force, 0);
    if (success > 0) {
      int dam = convert_damage(stage(success, basedamage));
      if (GET_MENTAL(vict) - (dam * 100) >= 0) {
        act("$n falls to the floor unconcious.", TRUE, vict, 0, 0, TO_ROOM);
        send_to_char("The world turns black around you as you suddenly fall unconcious.", vict);
      } else if (GET_MENTAL(vict) - (dam * 100) <= 300) {
        act("$n grabs $s head and cringes.", TRUE, vict, 0, 0, TO_ROOM);
        send_to_char("Your head is filled with an unbarable pressure as your vision begins to fade.", vict);
      } else if (GET_MENTAL(vict) - (dam * 100) <= 700) {
        act("$n shakes $s head forcibly as though trying to clear it.", TRUE, vict, 0, 0, TO_ROOM);
        send_to_char("A wave of tiredness comes over you, but seems to clear slightly as you shake your head.", vict);
      } else  {
        act("$n recoils slightly as though hit by an invisble force.", TRUE, vict, 0, 0, TO_ROOM);
        send_to_char("You mind goes slighlty hazy as though you had just been punched.", vict);
      }
      damage(ch, vict, dam, TYPE_COMBAT_SPELL, MENTAL);
    } else
      send_to_char(FAILED_CAST, ch);
    spell_drain(ch, spell, force, basedamage);
    break;
  case SPELL_POWERBOLT:
    if (PLR_FLAGGED(ch, PLR_KILLER) && !IS_NPC(vict))
      success = -1;
    else
      success = success_test(skill, GET_BOD(vict) + target) - resist_spell(vict, spell, force, 0);
    if (success > 0) {
      int dam = convert_damage(stage(success, basedamage));
      if (GET_MENTAL(vict) - (dam * 100) <= 0) {
        act("$n screams loudly as $s body is torn asunder by magic.", FALSE, vict, 0, 0, TO_ROOM);
        send_to_char("You begin to feel yourself tearing apart.", vict);
      } else if (GET_MENTAL(vict) - (dam * 100) <= 300) {
        act("$n cries out in pain as $s begins to bleed from nearly every pore on $s body.", TRUE, vict, 0, 0, TO_ROOM);
        send_to_char("You uncontrollable scream as an immense force tears at your body.", vict);
      } else if (GET_MENTAL(vict) - (dam * 100) <= 700) {
        act("$n grabs $s stomach as a trickle of blood comes from $s mouth.", TRUE, vict, 0, 0, TO_ROOM);
        send_to_char("Your torso is filled with a sharp stabbing pain as you cough up some blood.", vict);
      } else {
        act("$n grimaces, obviously afflicted with mild pain.", TRUE, vict, 0, 0, TO_ROOM);
        send_to_char("You feel a dull throb of pain flow through your body.", vict);
      }
      damage(ch, vict, dam, TYPE_COMBAT_SPELL, PHYSICAL);
    } else
      send_to_char(FAILED_CAST, ch);
    spell_drain(ch, spell, force, basedamage);
    break;
  }
}

void cast_detection_spell(struct char_data *ch, int spell, int force, char *arg)
{
  struct char_data *vict = NULL;
  if (*arg)
    vict = get_char_room_vis(ch, arg);
  if (!check_spell_victim(ch, vict, spell))
    return;
  if (find_duplicate_spell(ch, vict, spell, 0))
    return;
  int target = modify_target(ch), skill = GET_SKILL(ch, SKILL_SORCERY) + GET_CASTING(ch), success = 0;
  spell_bonus(ch, spell, skill, target);
  if (skill == -1)
    return;
  switch (spell)
  {
  case SPELL_COMBATSENSE:
    success = success_test(skill, 4 + target);
    if (success > 0) {
      create_sustained(ch, vict, spell, force, 0, success, spells[spell].draindamage);
      send_to_char("The world seems to slow down around you as your sense of your surroundings becomes clearer.\r\n", vict);
      act("You successfully sustain that spell on $n.", FALSE, ch, 0, vict, TO_CHAR);
    } else
      send_to_char(FAILED_CAST, ch);
    spell_drain(ch, spell, force, 0);
    break;
  }

}

void cast_health_spell(struct char_data *ch, int spell, int sub, int force, char *arg)
{
  struct char_data *vict = NULL;
  if (*arg)
    vict = get_char_room_vis(ch, arg);
  if (!check_spell_victim(ch, vict, spell))
    return;
  if (find_duplicate_spell(ch, vict, spell, sub))
    return;
  int target = modify_target(ch), skill = GET_SKILL(ch, SKILL_SORCERY) + GET_CASTING(ch), success = 0, drain = LIGHT;
  spell_bonus(ch, spell, skill, target);
  if (skill == -1)
    return;
  bool cyber = TRUE;
  switch (spell)
  {
  case SPELL_DETOX:
    if ((GET_DRUG_STAGE(vict) == 1 || GET_DRUG_STAGE(vict) == 2) && GET_DRUG_AFFECT(vict))
      target = drug_types[GET_DRUG_AFFECT(vict)].power;
    if (!target) {
      send_to_char("They aren't affected by any drugs.\r\n", ch);
      return;
    }
    success = success_test(skill, target);
    if (success > 0 && !AFF_FLAGGED(vict, AFF_DETOX)) {
      create_sustained(ch, vict, spell, force, 0, success, spells[SPELL_STABILIZE].draindamage);
      send_to_char("You notice the affects of the drugs suddenly wear off.\r\n", vict);
      act("You successfully sustain that spell on $n.", FALSE, ch, 0, vict, TO_CHAR);
    } else
      send_to_char(FAILED_CAST, ch);
    spell_drain(ch, spell, force, 0);
    break;
  case SPELL_STABILIZE:
    target += 4 + ((GET_LAST_DAMAGETIME(vict) - time(0)) / SECS_PER_MUD_HOUR);
    success = success_test(skill, target);
    if (success > 0 && force >= -(GET_PHYSICAL(vict) / 100)) {
      create_sustained(ch, vict, spell, force, 0, success, spells[SPELL_STABILIZE].draindamage);
      send_to_char("Your condition stabilizes, you manage to grab a thin hold on life.\r\n", vict);
      act("You successfully sustain that spell on $n.", FALSE, ch, 0, vict, TO_CHAR);
    } else
      send_to_char(FAILED_CAST, ch);
    spell_drain(ch, spell, force, 0);
    break;
  case SPELL_RESISTPAIN:
    success = success_test(skill, target + 4);
    if (GET_PHYSICAL(vict) <= 0)
      drain = DEADLY;
    else if (GET_PHYSICAL(vict) <= 300)
      drain = SERIOUS;
    else if (GET_PHYSICAL(vict) <= 700)
      drain = MODERATE;
    if (success > 0 && !AFF_FLAGGED(ch, AFF_RESISTPAIN)) {
      create_sustained(ch, vict, spell, force, 0, success, spells[SPELL_RESISTPAIN].draindamage);
      send_to_char("Your pain begins to fade.\r\n", vict);
      act("You successfully sustain that spell on $n.", FALSE, ch, 0, vict, TO_CHAR);
      vict->points.resistpain = MIN(force, success) * 100;
    } else
      send_to_char(FAILED_CAST, ch);
    spell_drain(ch, spell, force, 0);
    break;
  case SPELL_HEALTHYGLOW:
    success = success_test(skill, 4 + target);
    if (success > 0) {
      create_sustained(ch, vict, spell, force, 0, success, spells[SPELL_HEALTHYGLOW].draindamage);
      send_to_char("You begin to feel healthier and more attractive.\r\n", vict);
      act("You successfully sustain that spell on $n.", FALSE, ch, 0, vict, TO_CHAR);
    } else
      send_to_char(FAILED_CAST, ch);
    spell_drain(ch, spell, force, 0);
    break;
  case SPELL_TREAT:
    if (!AFF_FLAGGED(vict, AFF_DAMAGED)) {
      send_to_char("They are beyond the help of this spell.\r\n", ch);
      return;
    }
  case SPELL_HEAL:
    success = MIN(force, success_test(skill, 10 - (GET_ESS(vict) / 100) + target));
    if (GET_PHYSICAL(vict) <= 0)
      drain = DEADLY;
    else if (GET_PHYSICAL(vict) <= 300)
      drain = SERIOUS;
    else if (GET_PHYSICAL(vict) <= 700)
      drain = MODERATE;
    if (success < 1 || AFF_FLAGGED(vict, AFF_HEALED) || GET_PHYSICAL(vict) == GET_MAX_PHYSICAL(vict)) {
      send_to_char(FAILED_CAST, ch);
    } else {
      AFF_FLAGS(vict).SetBit(AFF_HEALED);
      send_to_char("A warm feeling floods your body.\r\n", vict);
      act("You successfully sustain that spell on $N.", FALSE, ch, 0, vict, TO_CHAR);
      create_sustained(ch, vict, spell, force, 0, success, drain);
      update_pos(vict);
    }
    spell_drain(ch, spell, force, drain);
    break;
  case SPELL_INCREF1:
  case SPELL_INCREF2:
  case SPELL_INCREF3:
    if (GET_REAL_REA(vict) != GET_REA(vict))
      success = -1;
    else
      success = success_test(skill, GET_REA(vict) + target);
    if (success > 0 && GET_INIT_DICE(ch) == 0) {
      create_sustained(ch, vict, spell, force, 0, success, spells[spell].draindamage);
      send_to_char("The world slows down around you.\r\n", ch);
      act("You successfully sustain that spell on $N.", FALSE, ch, 0, vict, TO_CHAR);
    } else
      send_to_char(FAILED_CAST, ch);
    spell_drain(ch, spell, force, 0);
    break;
  case SPELL_INCREA:
    sub = REA;
  case SPELL_DECATTR:
  case SPELL_DECCYATTR:
  case SPELL_INCATTR:
  case SPELL_INCCYATTR:
    if (GET_ATT(vict, sub) != GET_REAL_ATT(vict, sub)) {
      if (GET_TRADITION(vict) == TRAD_ADEPT && sub < CHA) {
        switch (sub) {
        case BOD:
          if (BOOST(vict)[2][0] || GET_POWER(vict, ADEPT_IMPROVED_BOD))
            cyber = false;
          break;
        case QUI:
          if (BOOST(vict)[1][0] || GET_POWER(vict, ADEPT_IMPROVED_QUI))
            cyber = false;
          break;
        case STR:
          if (BOOST(vict)[0][0] || GET_POWER(vict, ADEPT_IMPROVED_STR))
            cyber = false;
          break;
        }
      } else if (GET_SUSTAINED(vict))
        for (struct sustain_data *sus = GET_SUSTAINED(vict); sus; sus = sus->next)
          if (sus->caster == FALSE && (sus->spell == SPELL_INCATTR || sus->spell == SPELL_DECATTR) && sus->subtype == sub) {
            cyber = false;
            break;
          }
      if (cyber && (spell == SPELL_DECATTR || spell == SPELL_INCATTR || spell == SPELL_INCREA)) {
        sprintf(buf, "$N's %s has been modified by technological means and is immune to this spell.\r\n", attributes[sub]);
        act(buf, TRUE, ch, 0, vict, TO_CHAR);
        return;
      } else if (!cyber && (spell == SPELL_DECCYATTR || spell == SPELL_INCCYATTR)) {
        sprintf(buf, "$N's %s has not been modified by technological means and is immune to this spell.\r\n", attributes[sub]);
        act(buf, TRUE, ch, 0, vict, TO_CHAR);
        return;
      }
    }
    if (spell == SPELL_INCREA)
      target += GET_REA(vict);
    else if (spell == SPELL_INCATTR || spell == SPELL_INCCYATTR)
      target += GET_ATT(vict, sub);
    else
      target += 10 - (GET_ESS(vict) / 100);
    success = (int)(success_test(skill, target) -
                    ((spell == SPELL_DECATTR || spell == SPELL_DECCYATTR) ? resist_spell(vict, spell, force, sub) : 0));
    if (success > 0) {
      create_sustained(ch, vict, spell, force, sub, success, spells[spell].draindamage);
      act("You successfully sustain that spell on $N.", FALSE, ch, 0, vict, TO_CHAR);
      send_to_char("You feel your body tingle.\r\n", vict);
    } else
      send_to_char(FAILED_CAST, ch);
    spell_drain(ch, spell, force, 0);
    break;
  }
}

void cast_illusion_spell(struct char_data *ch, int spell, int force, char *arg)
{
  struct char_data *vict = NULL;
  if (*arg)
    vict = get_char_room_vis(ch, arg);
  if (find_duplicate_spell(ch, vict, spell, 0))
    return;
  int target = modify_target(ch), skill = GET_SKILL(ch, SKILL_SORCERY) + GET_CASTING(ch), success = 0;
  spell_bonus(ch, spell, skill, target);
  if (skill == -1)
    return;
  switch (spell)
  {
  case SPELL_CONFUSION:
  case SPELL_CHAOS:
    if (!check_spell_victim(ch, vict, spell))
      return;
    check_killer(ch, vict);
    if (spell == SPELL_CONFUSION)
      target += GET_WIL(vict);
    else
      target += GET_INT(vict);
    if (PLR_FLAGGED(ch, PLR_KILLER) && !IS_NPC(vict))
      success = 1;
    else
      success = success_test(skill, target) - resist_spell(vict, spell, force, 0);
    ;
    if (success > 0) {
      send_to_char("Coherent thought is suddenly a foreign concept.\r\n", vict);
      act("You successfully sustain that spell on $N.", FALSE, ch, 0, vict, TO_CHAR);
      create_sustained(ch, vict, spell, force, 0, success, spells[spell].draindamage);
    } else
      send_to_char(FAILED_CAST, ch);
    spell_drain(ch, spell, force, 0);
    break;
  case SPELL_INVIS:
  case SPELL_IMP_INVIS:
    if (!check_spell_victim(ch, vict, spell))
      return;
    success = success_test(skill, target + 4);
    if (success > 0) {
      act("You blink and suddenly $n is gone!", TRUE, vict, 0, 0, TO_ROOM);
      send_to_char("You feel your body tingle.\r\n", vict);
      create_sustained(ch, vict, spell, force, 0, success, spells[spell].draindamage);
    } else
      send_to_char(FAILED_CAST, ch);
    spell_drain(ch, spell, force, 0);
    break;
  case SPELL_STEALTH:
    if (!check_spell_victim(ch, vict, spell))
      return;
    success = success_test(skill, target + 4);
    if (success > 0) {
      act("You successfully sustain that spell on $n.", FALSE, vict, 0, ch, TO_VICT);
      send_to_char("Your every move becomes silent.", vict);
      create_sustained(ch, vict, spell, force, 0, success, spells[spell].draindamage);
    } else
      send_to_char(FAILED_CAST, ch);
    spell_drain(ch, spell, force, 0);
    break;
  case SPELL_SILENCE:
    success = success_test(skill, target + 4);
    if (success > 0) {
      act("The room falls silent.", FALSE, ch, 0, 0, TO_ROOM);
      create_sustained(ch, ch, spell, force, 0, success, spells[spell].draindamage);
    } else
      send_to_char(FAILED_CAST, ch);
    spell_drain(ch, spell, force, 0);
    break;
  }
}

void cast_manipulation_spell(struct char_data *ch, int spell, int force, char *arg)
{
  struct char_data *vict = NULL;
  int basedamage = 0;
  switch (spell)
  {
  case SPELL_ACIDSTREAM:
  case SPELL_TOXICWAVE:
  case SPELL_FLAMETHROWER:
  case SPELL_FIREBALL:
  case SPELL_LIGHTNINGBOLT:
  case SPELL_BALLLIGHTNING:
  case SPELL_CLOUT:
    two_arguments(arg, buf, buf1);
    if (!*buf) {
      send_to_char("What damage level do you wish to cast that spell at?\r\n", ch);
      return;
    } else {
      for (basedamage = 0; *wound_name[basedamage] != '\n'; basedamage++)
        if (is_abbrev(buf, wound_name[basedamage]))
          break;
      if (basedamage > 4 || basedamage == 0) {
        send_to_char("That is not a valid damage level, please choose between Light, Moderate, Serious and Deadly.\r\n", ch);
        return;
      }
    }
    if (*buf1)
      vict = get_char_room_vis(ch, buf1);
  default:
    if (*arg)
      vict = get_char_room_vis(ch, arg);
  }
  if (find_duplicate_spell(ch, vict, spell, 0))
    return;
  int target = modify_target(ch), skill = IS_ELEMENTAL(ch) || IS_SPIRIT(ch) ? GET_LEVEL(ch) : GET_SKILL(ch, SKILL_SORCERY) + GET_CASTING(ch), success = 0;
  if (!(IS_ELEMENTAL(ch) || IS_SPIRIT(ch)))
    spell_bonus(ch, spell, skill, target);
  if (skill == -1)
    return;
  switch (spell)
  {
  case SPELL_ARMOUR:
    if (!check_spell_victim(ch, vict, spell))
      return;
    success = success_test(skill, target + 6);
    if (success > 0) {
      create_sustained(ch, vict, spell, force, 0, success, spells[spell].draindamage);
      send_to_char("You feel your body tingle.\r\n", vict);
      act("You successfully sustain that spell on $N.", FALSE, ch, 0, vict, TO_CHAR);
    } else
      send_to_char(FAILED_CAST, ch);
    spell_drain(ch, spell, force, 0);
    break;
  case SPELL_POLTERGEIST:
    success = success_test(skill, target + 4);
    if (success > 0) {
      create_sustained(ch, ch, spell, force, 0, success, spells[spell].draindamage);
      act("An invisible wind begins to spin small objects around the area!", FALSE, ch, 0, 0, TO_ROOM);
      act("An invisible wind begins to spin small objects around the area!", FALSE, ch, 0, 0, TO_CHAR);
    } else
      send_to_char(FAILED_CAST, ch);
    spell_drain(ch, spell, force, 0);
    break;
  case SPELL_LIGHT:
    success = success_test(skill, target + 4);
    if (success > 0) {
      create_sustained(ch, ch, spell, force, 0, success, spells[spell].draindamage);
      act("Light radiates from $n, illuminating the area!", FALSE, ch, 0, 0, TO_ROOM);
      act("The area brightens as your light spell takes affect!", FALSE, ch, 0, 0, TO_CHAR);
    } else
      send_to_char(FAILED_CAST, ch);
    spell_drain(ch, spell, force, 0);
    break;
  case SPELL_ICESHEET:
    if (!ch->in_room) {
      send_to_char("You can't create ice in here!\r\n", ch);
      return;
    }
    success = success_test(skill, target + 4);
    if (success > 0) {
      world[ch->in_room].icesheet[0] = (int)(3.14 * ((GET_MAG(ch) * GET_MAG(ch)) / 10000));
      world[ch->in_room].icesheet[1] = force + MIN(force, success / 2);
      act("The floor is suddenly covered in ice!", FALSE, ch, 0, 0, TO_ROOM);
      act("The floor is suddenly covered in ice!", FALSE, ch, 0, 0, TO_CHAR);
    } else
      send_to_char(FAILED_CAST, ch);
    spell_drain(ch, spell, force, 0);
    break;
  case SPELL_IGNITE:
    if (!check_spell_victim(ch, vict, spell))
      return;
    check_killer(ch, vict);
    if (PLR_FLAGGED(ch, PLR_KILLER) && !IS_NPC(vict))
      success = -1;
    else {
      success = success_test(skill, target + 4);
      success -= GET_BOD(vict) / 2;
    }
    if (success > 0) {
      send_to_char("You feel the room temperature sharply rise.\r\n", vict);
      act("You succeed in raising the body temperature of $N to dangerous levels.", FALSE, ch, 0, vict, TO_CHAR);
      create_sustained(ch, vict, spell, force, 0, success, MAX(1, 10 / success));
      if (!FIGHTING(vict) && vict != ch)
        set_fighting(vict, ch);
    } else {
      send_to_char("You feel your body heat slightly then return to normal.\r\n", vict);
      send_to_char("You fail to generate enough heat in your target.\r\n", ch);
    }
    spell_drain(ch, spell, force, 0);
    break;
  case SPELL_SHADOW:
    success = success_test(skill, target + 4);
    if (success > 0) {
      create_sustained(ch, ch, spell, force, 0, success, spells[spell].draindamage);
      act("Dark shadows fall over the area.", FALSE, ch, 0, 0, TO_ROOM);
      act("Dark shadows fall over the area as your spell takes affect.", FALSE, ch, 0, 0, TO_ROOM);
    } else
      send_to_char(FAILED_CAST, ch);
    spell_drain(ch, spell, force, 0);
    break;
  case SPELL_CLOUT:
    if (!check_spell_victim(ch, vict, spell))
      return;
    check_killer(ch, vict);
    if (!AWAKE(vict))
      target -= 2;
    send_to_char("You feel a rush of air head towards you!", vict);
    if (PLR_FLAGGED(ch, PLR_KILLER) && !IS_NPC(vict))
      success = -1;
    else
      success = success_test(skill, 4 + target) - success_test(GET_DEFENSE(vict) + GET_DEFENSE(vict) ? GET_POWER(vict, ADEPT_SIDESTEP) : 0, 4 + damage_modifier(vict, buf));
    if (success <= 0) {
      act("Your clout spell harmlessly disperses as $n dodges it.", FALSE, vict, 0, ch, TO_VICT);
      send_to_char("You easily dodge it!", vict);
      act("$n dodges out of the way of unseen force.", TRUE, vict, 0, ch, TO_NOTVICT);
    } else {
      success -= success_test(GET_BOD(vict) + GET_BODY(vict), force - GET_IMPACT(vict));
      int dam = convert_damage(stage(success, basedamage));
      if (!AWAKE(vict)) {
        act("$n's body recoils as though hit.", TRUE, vict, 0, 0, TO_ROOM);
        send_to_char("You feel a dull thud in the back of your mind.\r\n", ch);
      } else if (GET_MENTAL(vict) - (dam * 100) <= 0) {
        act("$n is thrown to the ground, unconcious, from an immense invisible force.", TRUE, vict, 0, 0, TO_ROOM);
        send_to_char("It slams into you with the force of a freight train, knocking you to the ground, unconcious.\r\n", vict);
      } else if (GET_MENTAL(vict) - (dam * 100) <= 300) {
        act("$n stumbles backwards, almost losing $s footing, as $e is hit by an invisible force.", TRUE, vict, 0, 0, TO_ROOM);
        send_to_char("You stumble backwards, feeling groggy as the air slams into you at full force.\r\n", vict);
      } else if (GET_MENTAL(vict) - (dam * 100) <= 700) {
        act("$n steps back, and shakes $s head to clear it.", TRUE, vict, 0, 0, TO_ROOM);
        send_to_char("You step back as the force hits you, feeling a little worse for wear.\r\n", vict);
      } else if (dam > 0) {
        act("$n recoils, as if from a light punch.", TRUE, vict, 0, 0, TO_ROOM);
        send_to_char("You feel as though a fist strikes you across the face.\r\n", vict);
      } else {
        send_to_char("It rushes past you, not causing any damage.\r\n", vict);
        act("$n doesn't seem hurt by your clout spell.", FALSE, vict, 0, ch, TO_VICT);
      }
      damage(ch, vict, dam, TYPE_MANIPULATION_SPELL, MENTAL);
    }
    spell_drain(ch, spell, force, basedamage);
    break;
  case SPELL_FLAMETHROWER:
    if (!check_spell_victim(ch, vict, spell))
      return;
    check_killer(ch, vict);
    if (!AWAKE(vict))
      target -= 2;
    else
      success -= success_test(GET_DEFENSE(vict) + GET_DEFENSE(vict) ? GET_POWER(vict, ADEPT_SIDESTEP) : 0, 4 + damage_modifier(vict, buf));
    act("$n's hands seem to spontaneously combust as $e directs a stream of flame at $N!", TRUE, ch, 0, vict, TO_ROOM);
    if (PLR_FLAGGED(ch, PLR_KILLER) && !IS_NPC(vict))
      success = -1;
    else
      success += success_test(skill, 4 + target);
    if (success <= 0) {
      act("$n dodges the flames, which disperese as they past $s.", FALSE, vict, 0, 0, TO_ROOM);
      send_to_char("You easily dodge the flames!", vict);
    } else {
      success -= success_test(GET_BOD(vict) + GET_BODY(vict) + GET_POWER(ch, ADEPT_TEMPERATURE_TOLERANCE), force - (GET_IMPACT(vict) / 2));
      int dam = convert_damage(stage(success, basedamage));
      if (!AWAKE(vict)) {
        act("$n spasms as the flames hit $m.", TRUE, vict, 0, 0, TO_ROOM);
        send_to_char("You feel a slight burning sensation in the back of your mind.\r\n", vict);
      } else if (GET_MENTAL(vict) - (dam * 100) <= 0) {
        act("$n is hit full force by the intense flames causing $m to fall to the ground, gurgling.", TRUE, vict, 0, 0, TO_ROOM);
        send_to_char("The flames burn intensely around you, your last memory before falling unconcious is the hideous pain.\r\n", vict);
      } else if (GET_MENTAL(vict) - (dam * 100) <= 300) {
        act("Screams as the flames impact $s body, horribly burning $m.", TRUE, vict, 0, 0, TO_ROOM);
        send_to_char("The flames crash into you, causing you great pain as they horribly burn you.\r\n", vict);
      } else if (GET_MENTAL(vict) - (dam * 100) <= 700) {
        act("$n cringes as the flames hit, patting at the spots where the flame continues to burn.", TRUE, vict, 0, 0, TO_ROOM);
        send_to_char("As the flames hit you quickly pat at the spots that continue to burn, causing searing pain.\r\n", vict);
      } else if (dam > 0) {
        act("The flames burst around $m, causing seemingly little damage.", TRUE, vict, 0, 0, TO_ROOM);
        send_to_char("The flames burst around you causing you slight pain as it burns some of your hair.\r\n", vict);
      } else {
        act("The flames impact $n, but disperse on impact.", FALSE, vict, 0, ch, TO_ROOM);
        send_to_char("The flames rapidly disperse around you, causing only mild discomfort.\r\n", vict);
      }
      damage_equip(ch, vict, force, TYPE_FIRE);
      if (!damage(ch, vict, dam, TYPE_MANIPULATION_SPELL, PHYSICAL) && number(0, 6) <= basedamage + 1) {
        act("^RThe flames continue to burn around $m!^N", TRUE, vict, 0, 0, TO_ROOM);
        send_to_char("^RYou continue to burn!\r\n", vict);
        vict->points.fire[0] = srdice();
        vict->points.fire[1] = 0;
      }
    }
    spell_drain(ch, spell, force, basedamage);
    break;
  case SPELL_ACIDSTREAM:
    if (!check_spell_victim(ch, vict, spell))
      return;
    check_killer(ch, vict);
    if (!AWAKE(vict))
      target -= 2;
    else
      success -= success_test(GET_DEFENSE(vict) + GET_DEFENSE(vict) ? GET_POWER(vict, ADEPT_SIDESTEP) : 0, 4 + damage_modifier(vict, buf));
    act("Dark clouds form around $n moments before it condenses into a dark sludge and flies towards $N!", TRUE, ch, 0, vict, TO_ROOM);
    if (PLR_FLAGGED(ch, PLR_KILLER) && !IS_NPC(vict))
      success = -1;
    else
      success += success_test(skill, 4 + target);
    if (success <= 0) {
      act("$n dodges the acid, which evaporates as it passes $s.", FALSE, vict, 0, 0, TO_ROOM);
      send_to_char("You easily dodge the acid!", vict);
    } else {
      success -= success_test(GET_BOD(vict) + GET_BODY(vict) + GET_POWER(ch, ADEPT_TEMPERATURE_TOLERANCE), force - (GET_IMPACT(vict) / 2));
      int dam = convert_damage(stage(success, basedamage));
      if (!AWAKE(vict)) {
        act("$n spasms as the acid hits $m.", TRUE, vict, 0, 0, TO_ROOM);
        send_to_char("You feel a slight burning sensation in the back of your mind.\r\n", vict);
      } else if (GET_MENTAL(vict) - (dam * 100) <= 0) {
        act("As the acid hits $n $e falls to the ground twitching and screaming as $s body smokes.", TRUE, vict, 0, 0, TO_ROOM);
        send_to_char("The fumes from the acid burning through your body fill your lungs, burning you from the inside out as you fade into unconciousness.\r\n", vict);
      } else if (GET_MENTAL(vict) - (dam * 100) <= 300) {
        act("The acid hits $n, $e cries out in pain as trails of smoke come from $s body.", TRUE, vict, 0, 0, TO_ROOM);
        send_to_char("The acid impacts you with a great force, causing you to step back as it burns through your skin.\r\n", vict);
      } else if (GET_MENTAL(vict) - (dam * 100) <= 700) {
        act("$n cringes as the acid hits.", TRUE, vict, 0, 0, TO_ROOM);
        send_to_char("The acid begins to burn your skin as it hits you, causing a bit of pain.\r\n", vict);
      } else if (dam > 0) {
        act("$n is splashed by the acid, causing nothing but mild irritation.", TRUE, vict, 0, 0, TO_ROOM);
        send_to_char("The acid splashes against you causing a mild burning sensation.\r\n", vict);
      } else {
        act("The acid splashes on $n, but $e doesn't seem to flinch.", FALSE, vict, 0, ch, TO_ROOM);
        send_to_char("You are splashed by the acid, but it causes nothing more than a moments irritation.\r\n", vict);
      }
      damage_equip(ch, vict, force, TYPE_ACID);
      AFF_FLAGS(vict).SetBit(AFF_ACID);
      damage(ch, vict, dam, TYPE_MANIPULATION_SPELL, PHYSICAL);
    }
    spell_drain(ch, spell, force, basedamage);
    break;
  case SPELL_LIGHTNINGBOLT:
    if (!check_spell_victim(ch, vict, spell))
      return;
    check_killer(ch, vict);
    if (!AWAKE(vict))
      target -= 2;
    else
      success -= success_test(GET_DEFENSE(vict), 4 + damage_modifier(vict, buf));
    act("Lightning bursts forth from $n and heads directly towards $N!", TRUE, ch, 0, vict, TO_ROOM);
    if (PLR_FLAGGED(ch, PLR_KILLER) && !IS_NPC(vict))
      success = -1;
    else
      success += success_test(skill, 4 + target);
    if (success <= 0) {
      act("$n easily dodges it, and it vanishes to nothing.", FALSE, vict, 0, 0, TO_ROOM);
      send_to_char("You easily dodge it!", vict);
    } else {
      success -= success_test(GET_BOD(vict) + GET_BODY(vict) + GET_POWER(ch, ADEPT_TEMPERATURE_TOLERANCE), force - (GET_IMPACT(vict) / 2));
      int dam = convert_damage(stage(success, basedamage));
      if (!AWAKE(vict)) {
        act("$n's body goes into convulsions as the lightning flows through it.", TRUE, vict, 0, 0, TO_ROOM);
        send_to_char("You feel a slight burning sensation in the back of your mind.\r\n", vict);
      } else if (GET_MENTAL(vict) - (dam * 100) <= 0) {
        act("$n is propelled backwards by the force of the lightning bolt, $s body smoking as it lands, not a sign of life from it.", TRUE, vict, 0, 0, TO_ROOM);
        send_to_char("You feel your body begin to spasm as the huge charge of electricity fries your nervous system.\r\n", vict);
      } else if (GET_MENTAL(vict) - (dam * 100) <= 300) {
        act("$n is almost lifted in the air by the lightning, spasms filling $s body, as a thin trail of smoke rise from $m.", TRUE, vict, 0, 0, TO_ROOM);
        send_to_char("Your body is filled with pain as the lightning hits you, your limbs going into an uncontrolable seizure.\r\n", vict);
      } else if (GET_MENTAL(vict) - (dam * 100) <= 700) {
        act("$n spasms as the lightning hits $m, his body wracked with spasms as the lightning disipates.", TRUE, vict, 0, 0, TO_ROOM);
        send_to_char("Pain flashes through your body as the lightning hits, your body wracked with serveral serious spasms.\r\n", vict);
      } else if (dam > 0) {
        act("$n visibly recoils as the lightning hits, but otherwise seems fine.", TRUE, vict, 0, 0, TO_ROOM);
        send_to_char("You recoil as the lightning hits you, your mind going fuzzy for a moment.\r\n", vict);
      } else {
        act("The lightning hits $n, but seems to be easily absorbed.", FALSE, vict, 0, ch, TO_ROOM);
        send_to_char("Your body absorbs the lightning without harm.\r\n", vict);
      }
      for (int i = 0; i < NUM_WEARS; i++)
        if (GET_EQ(ch, i) && (GET_OBJ_MATERIAL(GET_EQ(ch, i)) == 10 || GET_OBJ_MATERIAL(GET_EQ(ch, i)) == 11))
          damage_obj(ch, GET_EQ(ch, i), force, DAMOBJ_LIGHTNING);
      damage(ch, vict, dam, TYPE_MANIPULATION_SPELL, PHYSICAL);
    }
    spell_drain(ch, spell, force, basedamage);
    break;
  }
}

void cast_spell(struct char_data *ch, int spell, int sub, int force, char *arg)
{
  if (spells[spell].duration == SUSTAINED)
  {
    if (GET_SUSTAINED_NUM(ch) >= GET_SKILL(ch, SKILL_SORCERY)) {
      send_to_char("You cannot sustain anymore spells.\r\n", ch);
      return;
    }
  }
  switch (spells[spell].category)
  {
  case COMBAT:
    cast_combat_spell(ch, spell, force, arg);
    break;
  case DETECTION:
    cast_detection_spell(ch, spell, force, arg);
    break;
  case HEALTH:
    cast_health_spell(ch, spell, sub, force, arg);
    break;
  case ILLUSION:
    cast_illusion_spell(ch, spell, force, arg);
    break;
  case MANIPULATION:
    cast_manipulation_spell(ch, spell, force, arg);
    break;
  }
}

bool check_spirit_sector(rnum_t room, int spirit)
{
  if ((spirit == SPIRIT_WIND || spirit == SPIRIT_MIST || spirit == SPIRIT_STORM)) {
    if (SECT(room) == SPIRIT_HEARTH || SECT(room) == SPIRIT_FOREST || ROOM_FLAGGED(room, ROOM_INDOORS))
      return FALSE;
  } else if (SECT(room) != spirit)
    return FALSE;
  return TRUE;
}

void circle_build(struct char_data *ch, char *type, int force)
{
  if (IS_WORKING(ch))
  {
    send_to_char(ch, "You're too busy.\r\n");
    return;
  }
  if (GET_TRADITION(ch) != TRAD_HERMETIC || GET_ASPECT(ch) == ASPECT_SORCERER)
  {
    send_to_char("Only hermetic mages need to construct a hermetic circle.\r\n", ch);
    return;
  }
  if (GET_NUYEN(ch) < force * force)
  {
    send_to_char(ch, "You need %d nuyen for the materials needed to construct that circle.\r\n", force * force);
    return;
  }
  int element = 0;
  for (;element < NUM_ELEMENTS; element++)
    if (is_abbrev(type, elements[element].name))
      break;
  if (element == NUM_ELEMENTS)
  {
    send_to_char("What element do you wish to dedicate this circle to?\r\n", ch);
    return;
  }
  GET_NUYEN(ch) -= force * force;
  struct obj_data *obj = read_object(115, VIRTUAL);
  GET_OBJ_VAL(obj, 1) = force;
  GET_OBJ_VAL(obj, 2) = element;
  GET_OBJ_VAL(obj, 3) = GET_IDNUM(ch);
  GET_OBJ_VAL(obj, 9) = force * 60;
  AFF_FLAGS(ch).SetBit(AFF_CIRCLE);
  GET_BUILDING(ch) = obj;
  obj_to_room(obj, ch->in_room);
  send_to_char("You begin to draw a heremetic circle.\r\n", ch);
  act("$n begins to draw a hermetic circle.\r\n", FALSE, ch, 0, 0, TO_ROOM);
}

void lodge_build(struct char_data *ch, int force)
{
  if (IS_WORKING(ch))
  {
    send_to_char(ch, "You're too busy.\r\n");
    return;
  }
  if (GET_TRADITION(ch) != TRAD_SHAMANIC)
  {
    send_to_char("Only shamans need to build a lodge.\r\n", ch);
    return;
  }
  if (GET_NUYEN(ch) < force * 500)
  {
    send_to_char(ch, "You need %d nuyen worth of materials to construct that lodge.\r\n", force * 500);
    return;
  }
  GET_NUYEN(ch) -= force * 500;
  struct obj_data *obj = read_object(114, VIRTUAL);
  GET_OBJ_VAL(obj, 1) = force;
  GET_OBJ_VAL(obj, 2) = GET_TOTEM(ch);
  GET_OBJ_VAL(obj, 3) = GET_IDNUM(ch);
  GET_OBJ_VAL(obj, 9) = force * 60 * 5;
  AFF_FLAGS(ch).SetBit(AFF_LODGE);
  GET_BUILDING(ch) = obj;
  obj_to_room(obj, ch->in_room);
  send_to_char(ch, "You begin to build a lodge to %s.\r\n", totem_types[GET_TOTEM(ch)]);
  act("$n begins to build a lodge.\r\n", FALSE, ch, 0, 0, TO_ROOM);
}

struct char_data *create_elemental(struct char_data *ch, int type, int force, int idnum, int trad)
{
  struct char_data *mob;
  if (trad == TRAD_HERMETIC)
    mob = read_mobile(elements[type].vnum, VIRTUAL);
  else
    mob = read_mobile(spirits[type].vnum, VIRTUAL);
  GET_REAL_BOD(mob) = force;
  GET_REAL_QUI(mob) = force;
  GET_REAL_STR(mob) = force;
  GET_REAL_CHA(mob) = force;
  GET_REAL_INT(mob) = force;
  GET_REAL_WIL(mob) = force;
  GET_ESS(mob) = force * 100;
  GET_LEVEL(mob) = force;
  GET_ACTIVE(mob) = GET_IDNUM(ch);
  GET_SPARE1(mob) = type;
  GET_SPARE2(mob) = force;
  GET_GRADE(mob) = idnum;
  if (trad == TRAD_HERMETIC)
    switch (type)
    {
    case ELEM_EARTH:
      GET_REAL_BOD(mob) += 4;
      GET_REAL_QUI(mob) -= 2;
      GET_REAL_STR(mob) += 4;
      break;
    case ELEM_FIRE:
      GET_REAL_BOD(mob)++;
      GET_REAL_QUI(mob) += 2;
      GET_REAL_STR(mob) -= 2;
      break;
    case ELEM_AIR:
      GET_REAL_BOD(mob) -= 2;
      GET_REAL_QUI(mob) += 3;
      GET_REAL_STR(mob) -= 3;
      break;
    case ELEM_WATER:
      GET_REAL_BOD(mob) += 2;
      break;
    }
  else
    switch (type)
    {
    case SPIRIT_CITY:
    case SPIRIT_FIELD:
    case SPIRIT_HEARTH:
      GET_REAL_BOD(mob)++;
      GET_REAL_QUI(mob) += 2;
      GET_REAL_STR(mob) -= 2;
      break;
    case SPIRIT_DESERT:
    case SPIRIT_FOREST:
    case SPIRIT_MOUNTAIN:
    case SPIRIT_PRAIRIE:
      GET_REAL_BOD(mob) += 4;
      GET_REAL_QUI(mob) -= 2;
      GET_REAL_STR(mob) += 4;
      break;
    case SPIRIT_MIST:
    case SPIRIT_STORM:
    case SPIRIT_WIND:
      GET_REAL_BOD(mob) -= 2;
      GET_REAL_QUI(mob) += 3;
      GET_REAL_STR(mob) -= 3;
      break;
    case SPIRIT_LAKE:
    case SPIRIT_RIVER:
    case SPIRIT_SEA:
    case SPIRIT_SWAMP:
      GET_REAL_BOD(mob) += 2;
      break;
    }
  if (ch->in_veh)
    char_to_room(mob, ch->in_veh->in_room);
  else
    char_to_room(mob, ch->in_room);
  add_follower(mob, ch);
  affect_total(mob);
  return mob;
}

ACMD(do_contest)
{
  if (GET_ASPECT(ch) == ASPECT_SORCERER || GET_TRADITION(ch) == TRAD_ADEPT || GET_TRADITION(ch) == TRAD_MUNDANE || !GET_SKILL(ch, SKILL_CONJURING)) {
    send_to_char("You don't have the ability to do that.\r\n", ch);
    return;
  }
  struct char_data *mob, *caster = NULL;
  skip_spaces(&argument);
  if (!(mob = get_char_room_vis(ch, argument))) {
    send_to_char(ch, "Contest the binding of which %s?\r\n", GET_TRADITION(ch) ? "nature spirit" : "elemental");
    return;
  }
  if (GET_CHA(ch) <= GET_NUM_SPIRITS(ch)) {
    send_to_char(ch, "You cannot have any more %ss bound to you.\r\n", GET_TRADITION(ch) ? "nature spirit" : "elemental");
    return;
  }
  if (GET_TRADITION(ch) == TRAD_SHAMANIC) {
    send_to_char("You can only contest the binding of a nature spirit.\r\n", ch);
  } else if (GET_TRADITION(ch) == TRAD_HERMETIC && (GET_MOB_VNUM(mob) < 25 || GET_MOB_VNUM(mob) > 28)) {
    send_to_char("You can only contest the binding of an elemental.\r\n", ch);
    return;
  }
  if (GET_ACTIVE(mob) == GET_IDNUM(ch)) {
    send_to_char(ch, "You already have that %s bound to you!\r\n", GET_TRADITION(ch) ? "nature spirit" : "elemental");
    return;
  }
  if (!GET_ACTIVE(mob)) {
    send_to_char("You can't bind a free spirit!\r\n", ch);
    return;
  }
  for (struct descriptor_data *d = descriptor_list; d; d = d->next)
    if (d->character && GET_IDNUM(d->character) == GET_ACTIVE(mob)) {
      caster = d->character;
      break;
    }
  if (!caster) {
    send_to_char("You fain to gain control!\r\n", ch);
    return;
  }
  int chskill = GET_SKILL(ch, SKILL_CONJURING), caskill = GET_SKILL(ch, SKILL_CONJURING) + GET_CHA(ch);
  for (int i = 0; i < NUM_WEARS; i++)
    if (GET_EQ(ch, i) && GET_OBJ_TYPE(GET_EQ(ch, i)) == ITEM_FOCUS && GET_OBJ_VAL(GET_EQ(ch, i), 0) == FOCI_SPIRIT
        && GET_OBJ_VAL(GET_EQ(ch, i), 2) == GET_IDNUM(ch) && GET_OBJ_VAL(GET_EQ(ch, i), 3) == GET_SPARE1(mob) &&
        GET_OBJ_VAL(GET_EQ(ch, i), 4)) {
      chskill += GET_OBJ_VAL(GET_EQ(ch, i), 1);
      break;
    }
  for (int i = 0; i < NUM_WEARS; i++)
    if (GET_EQ(caster, i) && GET_OBJ_TYPE(GET_EQ(caster, i)) == ITEM_FOCUS && GET_OBJ_VAL(GET_EQ(caster, i), 0) == FOCI_SPIRIT
        && GET_OBJ_VAL(GET_EQ(caster, i), 2) == GET_IDNUM(ch) && GET_OBJ_VAL(GET_EQ(caster, i), 3) == GET_SPARE1(mob) &&
        GET_OBJ_VAL(GET_EQ(caster, i), 4)) {
      chskill += GET_OBJ_VAL(GET_EQ(ch, i), 1);
      break;
    }

  int chsuc = success_test(chskill, GET_LEVEL(mob)), casuc = success_test(caskill, GET_LEVEL(mob));
  struct spirit_data *temp;
  if (chsuc < 1 && casuc < 1) {
    for (struct spirit_data *sdata = GET_SPIRIT(caster); sdata; sdata = sdata->next)
      if (sdata->id == GET_GRADE(mob)) {
        REMOVE_FROM_LIST(sdata, GET_SPIRIT(caster), next);
        delete [] sdata;
        break;
      }
    if (GET_MOB_VNUM(mob) < 25 || GET_MOB_VNUM(mob) > 28) {
      act("$n senses an opportunity and vanishes!", TRUE, mob, 0, 0, TO_ROOM);
      extract_char(mob);
    } else {
      MOB_FLAGS(mob).SetBit(MOB_AGGRESSIVE);
      act("$n becomes uncontrolled!", TRUE, mob, 0, 0, TO_ROOM);
      GET_ACTIVE(mob) = 0;
    }
    conjuring_drain(caster, GET_LEVEL(mob));
    conjuring_drain(ch, GET_LEVEL(mob));
  } else if (chsuc > casuc) {
    send_to_char(ch, "You steal control of %s!\r\n", GET_NAME(mob));
    sprintf(buf, "$n steals control of %s!", GET_NAME(mob));
    act(buf, FALSE, ch, 0, caster, TO_VICT);
    for (struct spirit_data *sdata = GET_SPIRIT(caster); sdata; sdata = sdata->next)
      if (sdata->id == GET_GRADE(mob)) {
        REMOVE_FROM_LIST(sdata, GET_SPIRIT(caster), next);
        sdata->services = chsuc - casuc;
        sdata->next = GET_SPIRIT(ch);
        GET_SPIRIT(ch) = sdata->next;
        GET_NUM_SPIRITS(ch)++;
        GET_NUM_SPIRITS(caster)--;
        break;
      }
    GET_ACTIVE(mob) = GET_IDNUM(ch);
    conjuring_drain(caster, GET_LEVEL(mob));
    conjuring_drain(ch, GET_LEVEL(mob));
  } else {
    send_to_char("You fail to gain control!\r\n", ch);
    sprintf(buf, "$n tries to steal control of %s.", GET_NAME(mob));
    act(buf, FALSE, ch, 0, caster, TO_VICT);
    conjuring_drain(ch, GET_LEVEL(mob));
  }
}

ACMD(do_bond)
{
  if (!*argument) {
    send_to_char("What do you want to bond?\r\n", ch);
    return;
  }
  half_chop(argument, buf1, buf2);
  struct obj_data *obj;
  int karma = 0, spirit = 0;
  struct spell_data *spell = GET_SPELLS(ch);

  for (obj = ch->carrying; obj; obj = obj->next_content)
    if (isname(buf1, obj->text.keywords) || isname(buf2, GET_OBJ_NAME(obj)))
      break;
  if (!obj)
    for (int i = 0; i < NUM_WEARS && !obj; i++)
      if (GET_EQ(ch, i) && (isname(buf2, GET_EQ(ch, i)->text.keywords) || isname(buf2, GET_OBJ_NAME(GET_EQ(ch, i)))))
        obj = GET_EQ(ch, i);
  if (!obj) {
    send_to_char("You don't have that item.\r\n", ch);
    return;
  }
  if (GET_OBJ_TYPE(obj) == ITEM_DOCWAGON) {
    if (GET_OBJ_VAL(obj, 1)) {
      act("$p has already been activated.", FALSE, ch, obj, 0, TO_CHAR);
      return;
    }
    GET_OBJ_VAL(obj, 1) = GET_IDNUM(ch);
    act("$p's lights begin to subtly flash in a rhythmic sequence.", FALSE,
        ch, obj, 0, TO_CHAR);
    return;
  }
  if (GET_OBJ_TYPE(obj) == ITEM_FOCUS) {
    if (GET_TRADITION(ch) == TRAD_MUNDANE)
      send_to_char(ch, "You can't bond foci.\r\n");
    else if (GET_TRADITION(ch) == TRAD_ADEPT && GET_OBJ_VAL(obj, 0) != FOCI_WEAPON)
      send_to_char("Adepts can only bond weapon foci.\r\n", ch);
    else if (IS_WORKING(ch))
      send_to_char("You're too busy to bond a focus.\r\n", ch);
    else if (GET_POS(ch) > POS_SITTING)
      send_to_char("You must be sitting to perform a bonding ritual.\r\n", ch);
    else if (GET_OBJ_VAL(obj, 2) == GET_IDNUM(ch)) {
      if (GET_OBJ_VAL(obj, 9)) {
        GET_OBJ_VAL(obj, 9) = GET_OBJ_VAL(obj, 1) * 60;
        send_to_char(ch, "You restart the ritual to bond %s.\r\n", GET_OBJ_NAME(obj));
        act("$n begins a ritual to bond $o.", TRUE, ch, obj, 0, TO_ROOM);
        AFF_FLAGS(ch).SetBit(AFF_BONDING);
        ch->char_specials.programming = obj;
      } else
        send_to_char("You have already bonded this focus.\r\n", ch);
    } else {
      switch (GET_OBJ_VAL(obj, 0)) {
      case FOCI_SPEC_SPELL:
        karma = GET_OBJ_VAL(obj, 1);
        break;
      case FOCI_EXPENDABLE:
      case FOCI_SPELL_CAT:
        if (!*buf2) {
          send_to_char("Bond which spell category?\r\n", ch);
          return;
        }
        for (; spirit <= MANIPULATION; spirit++)
          if (is_abbrev(buf2, spell_category[spirit]))
            break;
        if (spirit > MANIPULATION) {
          send_to_char("That is not a valid category.\r\n", ch);
          return;
        }
        if (GET_OBJ_VAL(obj, 0) == FOCI_SPELL_CAT)
          karma = GET_OBJ_VAL(obj, 1) * 3;
        break;
      case FOCI_SPIRIT:
        if (!*buf2) {
          send_to_char("Bond which spirit type?\r\n", ch);
          return;
        }
        if (GET_TRADITION(ch) == TRAD_HERMETIC) {
          for (; spirit < NUM_ELEMENTS; spirit++)
            if (is_abbrev(buf2, elements[spirit].name))
              break;
        } else
          for (; spirit < NUM_SPIRITS; spirit++)
            if (is_abbrev(buf2, spirits[spirit].name))
              break;
        if (GET_TRADITION(ch) == TRAD_HERMETIC ? spirit == NUM_ELEMENTS : spirit == NUM_SPIRITS) {
          send_to_char(ch, "That is not a valid %s.\r\n", GET_TRADITION(ch) == TRAD_HERMETIC ? "elemental" : "spirit");
          return;
        }
        karma = GET_OBJ_VAL(obj, 1) * 2;
        break;
      case FOCI_POWER:
        karma = GET_OBJ_VAL(obj, 1) * 5;
        break;
      case FOCI_SUSTAINED:
        karma = GET_OBJ_VAL(obj, 1);
        break;
      }
      if (GET_OBJ_VAL(obj, 0) == FOCI_SUSTAINED || GET_OBJ_VAL(obj, 0) == FOCI_SPEC_SPELL) {
        for (;spell; spell = spell->next)
          if (is_abbrev(buf2, spell->name)) {
            spirit = spell->type;
            break;
          }
        if (!spell) {
          send_to_char("You don't know that spell to bond.\r\n", ch);
          return;
        }
        if (GET_OBJ_VAL(obj, 0) == FOCI_SUSTAINED && spells[spirit].duration != SUSTAINED) {
          send_to_char("You don't need to bond a sustaining foci for this spell.\r\n", ch);
          return;
        }
      }
      if (PLR_FLAGGED(ch, PLR_AUTH)) {
        if (GET_FORCE_POINTS(ch) < karma) {
          send_to_char(ch, "You don't have enough force points to bond that (Need %d).\r\n", karma);
          return;
        }
        GET_FORCE_POINTS(ch) -= karma;
        GET_OBJ_VAL(obj, 9) = 1;
      } else {
        if (GET_KARMA(ch) < karma * 100) {
          send_to_char(ch, "You don't have enough karma to bond that (Need %d).\r\n", karma);
          return;
        }
        GET_KARMA(ch) -= karma * 100;
        GET_OBJ_VAL(obj, 9) = GET_OBJ_VAL(obj, 1) * 60;
      }
      GET_OBJ_VAL(obj, 2) = GET_IDNUM(ch);
      GET_OBJ_VAL(obj, 3) = spirit;
      GET_OBJ_VAL(obj, 5) = GET_TRADITION(ch) == TRAD_HERMETIC ? 1 : 0;
      send_to_char(ch, "You begin the ritual to bond %s.\r\n", GET_OBJ_NAME(obj));
      act("$n begins a ritual to bond $o.", TRUE, ch, 0, obj, TO_ROOM);
      AFF_FLAGS(ch).SetBit(AFF_BONDING);
      ch->char_specials.programming = obj;
      return;
    }
  } else
    send_to_char("You cannot bond that item.\r\n", ch);
}

ACMD(do_release)
{
  if (GET_TRADITION(ch) == TRAD_ADEPT || GET_TRADITION(ch) == TRAD_MUNDANE) {
    send_to_char("You don't have the ability to do that.\r\n", ch);
    return;
  }
  two_arguments(argument, buf, buf1);
  int i = 0;
  if (is_abbrev(buf, "spirit") || is_abbrev(buf, "elemental")) {
    if (GET_ASPECT(ch) == ASPECT_SORCERER || !GET_SKILL(ch, SKILL_CONJURING)) {
      send_to_char("You don't have the ability to do that.\r\n", ch);
    }
    if (!GET_SPIRIT(ch)) {
      send_to_char("You don't have any elementals bound to you.\r\n", ch);
      return;
    }
    int i;
    if (!(i = atoi(buf1)) || i > GET_NUM_SPIRITS(ch)) {
      send_to_char("Which spirit do you wish to release from your services?\r\n", ch);
      return;
    }
    for (struct spirit_data *spirit = GET_SPIRIT(ch); spirit; spirit = spirit->next)
      if (--i == 0) {
        struct spirit_data *temp;
        if (GET_TRADITION(ch) == TRAD_HERMETIC)
          send_to_char(ch, "You release %s from its obligations and it departs to the metaplanes.\r\n", GET_NAME(&mob_proto[real_mobile(elements[spirit->type].vnum)]));
        else
          send_to_char(ch, "You release %s from its obligations and it departs to the metaplanes.\r\n", GET_NAME(&mob_proto[real_mobile(spirits[spirit->type].vnum)]));
        if (spirit->called)
          for (struct char_data *mob = character_list; mob; mob = mob->next)
            if (IS_NPC(mob) && (GET_RACE(mob) == CLASS_SPIRIT || GET_RACE(mob) == CLASS_ELEMENTAL) && GET_ACTIVE(mob) ==
                GET_IDNUM(ch) && GET_GRADE(mob) == spirit->id) {
              act("Freed from its services, $n returns to the metaplanes", TRUE, mob, 0, ch, TO_NOTVICT);
              extract_char(mob);
              break;
            }
        REMOVE_FROM_LIST(spirit, GET_SPIRIT(ch), next);
        delete [] spirit;
        GET_NUM_SPIRITS(ch)--;
        return;
      }
  } else if ((i = atoi(buf)) > 0) {
    if (i > GET_SUSTAINED_NUM(ch))
      send_to_char("You don't have that many spells sustained.\r\n", ch);
    else {
      struct sustain_data *sust;
      for (sust = GET_SUSTAINED(ch); sust; sust = sust->next)
        if (sust->caster && --i == 0)
          break;
      end_sustained_spell(ch, sust);
    }
    return;
  }
  send_to_char("Release what?\r\n", ch);
}

ACMD(do_cast)
{
  if (GET_ASPECT(ch) == ASPECT_CONJURER || GET_TRADITION(ch) == TRAD_ADEPT || GET_TRADITION(ch) == TRAD_MUNDANE || !GET_SKILL(ch, SKILL_SORCERY)) {
    send_to_char("You don't have the ability to do that.\r\n", ch);
    return;
  }

  int force = 0;
  char spell_name[120], tokens[MAX_STRING_LENGTH], *s;
  struct spell_data *spell = GET_SPELLS(ch);
  if (!*argument) {
    send_to_char("Cast what spell?\r\n", ch);
    return;
  }
  strcpy(tokens, argument);
  if (strtok(tokens, "\"") && (s = strtok(NULL, "\""))) {
    strcpy(spell_name, s);
    if (s = strtok(NULL, "\0")) {
      skip_spaces(&s);
      strcpy(buf1, s);
    } else
      *buf1 = '\0';
    one_argument(argument, buf);
    force = atoi(buf);
  } else {
    half_chop(argument, buf, buf1);
    if (!(force = atoi(buf))) {
      strcpy(spell_name, buf);
    } else {
      half_chop(buf1, buf2, buf1);
      strcpy(spell_name, buf2);
    }
  }
  for (;spell; spell = spell->next)
    if (is_abbrev(spell_name, spell->name))
      break;
  if (!spell) {
    send_to_char("You don't know that spell.\r\n", ch);
    return;
  }
  if (!force)
    force = spell->force;
  else if (force > spell->force) {
    send_to_char("You don't know that spell at that high a force.\r\n", ch);
    return;
  }
  if (spells[spell->type].physical && IS_PROJECT(ch)) {
    send_to_char("You can only cast mana spells on the astral plane.\r\n", ch);
    return;
  }
  cast_spell(ch, spell->type, spell->subtype, force, buf1);
}

ACMD(do_conjure)
{
  if (GET_ASPECT(ch) == ASPECT_SORCERER || GET_TRADITION(ch) == TRAD_ADEPT || GET_TRADITION(ch) == TRAD_MUNDANE || !GET_SKILL(ch, SKILL_CONJURING)) {
    send_to_char("You don't have the ability to do that.\r\n", ch);
    return;
  }
  if (IS_WORKING(ch)) {
    send_to_char("You're too busy to conjure right now.\r\n", ch);
    return;
  }
  if (ch->in_veh) {
    send_to_char("There is not enough room to conjure in here.\r\n", ch);
    return;
  }
  int force, spirit = 0;
  two_arguments(argument, buf, buf1);
  if (!(force = atoi(buf))) {
    send_to_char("What force do you wish to conjure at?\r\n", ch);
    return;
  }
  if (force > (GET_MAG(ch) / 100) * 2) {
    send_to_char("You can't conjure a spirit of force more than twice your magic rating.\r\n", ch);
    return;
  }
  if (GET_TRADITION(ch) == TRAD_HERMETIC) {
    if (GET_NUM_SPIRITS(ch) >= GET_CHA(ch)) {
      send_to_char("You have too many spirit summoned.\r\n", ch);
      return;
    }
    for (; spirit < NUM_ELEMENTS; spirit++)
      if (is_abbrev(buf1, elements[spirit].name))
        break;
    if (spirit == NUM_ELEMENTS) {
      send_to_char("What elemental do you wish to conjure?\r\n", ch);
      return;
    }
    if ((GET_ASPECT(ch) == ASPECT_ELEMFIRE && spirit != ELEM_FIRE) ||
        (GET_ASPECT(ch) == ASPECT_ELEMWATER && spirit != ELEM_WATER) ||
        (GET_ASPECT(ch) == ASPECT_ELEMAIR && spirit != ELEM_AIR) ||
        (GET_ASPECT(ch) == ASPECT_ELEMEARTH && spirit != ELEM_EARTH)) {
      send_to_char("You cannot summon elementals of that type.\r\n", ch);
      return;
    }
    bool library = FALSE, circle = FALSE;
    struct obj_data *obj;
    for (struct obj_data *obj = world[ch->in_room].contents; obj; obj = obj->next_content)
      if (GET_OBJ_TYPE(obj) == ITEM_MAGIC_TOOL)
        if (GET_OBJ_VAL(obj, 0) == TYPE_LIBRARY_CONJURE) {
          if (GET_OBJ_VAL(obj, 1) < force) {
            send_to_char("Your library isn't of a high enough rating to conjure that elemental.\r\n", ch);
            return;
          }
          library = TRUE;
        } else if (GET_OBJ_VAL(obj, 0) == TYPE_CIRCLE && !GET_OBJ_VAL(obj, 9)) {
          if (GET_OBJ_VAL(obj, 1) < force) {
            send_to_char("Your hermetic circle isn't of a high enough rating to conjure that elemental.\r\n", ch);
            return;
          } else if (GET_OBJ_VAL(obj, 2) != spirit) {
            send_to_char("That circle is for a different type of elemental.\r\n", ch);
            return;
          }
          circle = TRUE;
        }
    if (!circle || !library) {
      send_to_char("You need a conjuring library and a hermetic circle to conjure spirits.\r\n", ch);
      return;
    }
    for (obj = ch->carrying; obj; obj = obj->next_content)
      if (GET_OBJ_TYPE(obj) == ITEM_MAGIC_TOOL && GET_OBJ_VAL(obj, 0) == TYPE_SUMMONING)
        if (GET_OBJ_COST(obj) < force * 1000) {
          send_to_char("You don't have enough materials to summon at that high a force.\r\n", ch);
          return;
        } else
          break;
    if (!obj) {
      send_to_char("You need conjuring materials to conjure an elemental.\r\n", ch);
      return;
    }
    AFF_FLAGS(ch).SetBit(AFF_CONJURE);
    ch->char_specials.conjure[0] = spirit;
    ch->char_specials.conjure[1] = force;
    ch->char_specials.conjure[2] = force * 30;
    ch->char_specials.programming = obj;
    send_to_char(ch, "You begin to conjure a %s elemental.\r\n", elements[spirit].name);
  } else {
    for (; spirit < NUM_SPIRITS; spirit++)
      if (is_abbrev(buf1, spirits[spirit].name))
        break;
    if (spirit == NUM_SPIRITS) {
      send_to_char("Which spirit do you wish to conjure?\r\n", ch);
      return;
    }
    if (GET_DOMAIN(ch) != ((spirit == SPIRIT_MIST || spirit == SPIRIT_STORM || spirit == SPIRIT_WIND) ? SPIRIT_SKY : spirit)) {
      send_to_char("You aren't in the correct domain to conjure that spirit.\r\n", ch);
      return;
    }
    int skill = GET_SKILL(ch, SKILL_CONJURING), target = force;
    totem_bonus(ch, CONJURING, spirit, target, skill);
    if (GET_ASPECT(ch) == ASPECT_SHAMANIST && skill == GET_SKILL(ch, SKILL_CONJURING)) {
      send_to_char("Your totem will not let you conjure that spirit!\r\n", ch);
      return;
    }
    for (int i = 0; i < NUM_WEARS; i++)
      if (GET_EQ(ch, i) && GET_OBJ_TYPE(GET_EQ(ch, i)) == ITEM_FOCUS && GET_OBJ_VAL(GET_EQ(ch, i), 0) == FOCI_SPIRIT
          && GET_OBJ_VAL(GET_EQ(ch, i), 2) == GET_IDNUM(ch) && GET_OBJ_VAL(GET_EQ(ch, i), 3) == ch->char_specials.conjure[0]
          && GET_OBJ_VAL(GET_EQ(ch, i), 4)) {
        skill += GET_OBJ_VAL(GET_EQ(ch, i), 1);
        break;
      }
    int success = success_test(skill, target);
    if (!conjuring_drain(ch, force) && AWAKE(ch)) {
      if (success < 1)
        send_to_char("You fail to conjure forth that spirit.\r\n", ch);
      else {
        struct spirit_data *spirdata = new spirit_data;
        spirdata->type = spirit;
        spirdata->force = force;
        spirdata->id = number(0, 1000);
        spirdata->services = success;
        spirdata->called = TRUE;
        spirdata->next = GET_SPIRIT(ch);
        GET_SPIRIT(ch) = spirdata;
        GET_NUM_SPIRITS(ch)++;
        struct char_data *mob = create_elemental(ch, spirit, force, spirdata->id, TRAD_SHAMANIC);
        send_to_char("The spirit hears your call and comes forth from the metaplanes.\r\n", ch);
        act("$n fades into existance on the astral plane.", TRUE, mob, 0, 0, TO_ROOM);
      }
    }
  }
}

ACMD(do_spells)
{
  if (GET_ASPECT(ch) == ASPECT_CONJURER || GET_TRADITION(ch) == TRAD_ADEPT || GET_TRADITION(ch) == TRAD_MUNDANE || !GET_SKILL(ch, SKILL_SORCERY)) {
    send_to_char("You don't have the ability to do that.\r\n", ch);
    return;
  }
  if (!GET_SPELLS(ch)) {
    send_to_char("You don't know any spells.\r\n", ch);
    return;
  }
  send_to_char("You know the following spells:\r\n", ch);
  for (struct spell_data *spell = GET_SPELLS(ch); spell; spell = spell->next)
    send_to_char(ch, "%-50s Type: %10s Force: %d\r\n", spell->name, spells[spell->type].name, spell->force);
}

ACMD(do_learn)
{
  if (GET_ASPECT(ch) == ASPECT_CONJURER || GET_TRADITION(ch) == TRAD_ADEPT || GET_TRADITION(ch) == TRAD_MUNDANE || !GET_SKILL(ch, SKILL_SORCERY)) {
    send_to_char("You don't have the ability to do that.\r\n", ch);
    return;
  }
  two_arguments(argument, buf, buf1);
  struct obj_data *obj = ch->carrying;
  int force;
  if (!*buf || !(obj = get_obj_in_list_vis(ch, buf, ch->carrying))) {
    send_to_char("Learn which spell?\r\n", ch);
    return;
  }
  if (GET_OBJ_TYPE(obj) != ITEM_SPELL_FORMULA) {
    send_to_char("You can't learn anything from that.\r\n", ch);
    return;
  }
  if ((GET_TRADITION(ch) == TRAD_HERMETIC && GET_OBJ_VAL(obj, 2)) || (GET_TRADITION(ch) == TRAD_SHAMANIC && !GET_OBJ_VAL(obj, 2))) {
    send_to_char("You don't understand this formula.\r\n", ch);
    return;
  }
  if (!*buf2 || atoi(buf1) == 0)
    force = GET_OBJ_VAL(obj, 0);
  else
    force = MIN(GET_OBJ_VAL(obj, 0), atoi(buf1));
  if (GET_KARMA(ch) < force * 100) {
    send_to_char(ch, "You don't have enough karma to learn this spell at that force! (You need %d)\r\n", force);
    return;
  }
  if ((GET_ASPECT(ch) == ASPECT_ELEMFIRE && spells[GET_OBJ_VAL(obj, 1)].category != COMBAT) ||
      (GET_ASPECT(ch) == ASPECT_ELEMEARTH && spells[GET_OBJ_VAL(obj, 1)].category != MANIPULATION) ||
      (GET_ASPECT(ch) == ASPECT_ELEMWATER && spells[GET_OBJ_VAL(obj, 1)].category != ILLUSION) ||
      (GET_ASPECT(ch) == ASPECT_ELEMAIR && spells[GET_OBJ_VAL(obj, 1)].category != DETECTION)) {
    send_to_char("Glancing over the formula you realise you can't bind mana in that fashion.\r\n", ch);
  }
  if (GET_ASPECT(ch) == ASPECT_SHAMANIST) {
    int skill = 0, target = 0;
    totem_bonus(ch, 0, GET_OBJ_VAL(obj, 1), target, skill);
    if (skill < 1) {
      send_to_char(ch, "%s forbids you from learning this spell.\r\n", totem_types[GET_TOTEM(ch)]);
    }
  }
  struct obj_data *library = world[ch->in_room].contents;
  for (;library; library = library->next_content)
    if (GET_OBJ_TYPE(library) == ITEM_MAGIC_TOOL && GET_OBJ_VAL(library, 1) >= force &&
        ((GET_TRADITION(ch) == TRAD_SHAMANIC
          && GET_OBJ_VAL(library, 0) == TYPE_LODGE && GET_OBJ_VAL(library, 3) == GET_IDNUM(ch)) ||
         (GET_TRADITION(ch) == TRAD_HERMETIC && GET_OBJ_VAL(library, 0) == TYPE_LIBRARY_SPELL)))
      break;
  if (!library) {
    send_to_char("You don't have the right tools here to learn that spell.\r\n", ch);
    return;
  }
  for (struct spell_data *spell = GET_SPELLS(ch); spell; spell = spell->next)
    if (spell->type == GET_OBJ_VAL(obj, 1) && spell->subtype == GET_OBJ_VAL(obj, 3)) {
      if (spell->force >= force) {
        send_to_char("You already know this spell at an equal or higher force.\r\n", ch);
        return;
      } else {
        struct spell_data *temp;
        REMOVE_FROM_LIST(spell, GET_SPELLS(ch), next);
        delete [] spell;
        break;
      }
    }
  int skill = GET_SKILL(ch, SKILL_SORCERY);
  if (GET_TRADITION(ch) == TRAD_HERMETIC && GET_SPIRIT(ch)) {
    for (struct spirit_data *spir = GET_SPIRIT(ch); spir && skill == GET_SKILL(ch, SKILL_SORCERY); spir = spir->next)
      if (spir->called) {
        struct char_data *spirit = find_spirit_by_id(spir->id, GET_IDNUM(ch));
        if (MOB_FLAGS(spirit).IsSet(MOB_STUDY)) {
          switch(spir->type) {
          case ELEM_FIRE:
            if (spells[GET_OBJ_VAL(obj, 1)].category == COMBAT) {
              skill += spir->force;
              MOB_FLAGS(spirit).RemoveBit(MOB_STUDY);
            }
            break;
          case ELEM_WATER:
            if (spells[GET_OBJ_VAL(obj, 1)].category == ILLUSION) {
              skill += spir->force;
              MOB_FLAGS(spirit).RemoveBit(MOB_STUDY);
            }
            break;
          case ELEM_AIR:
            if (spells[GET_OBJ_VAL(obj, 1)].category == DETECTION) {
              skill += spir->force;
              MOB_FLAGS(spirit).RemoveBit(MOB_STUDY);
            }
            break;
          case ELEM_EARTH:
            if (spells[GET_OBJ_VAL(obj, 1)].category == MANIPULATION) {
              skill += spir->force;
              MOB_FLAGS(spirit).RemoveBit(MOB_STUDY);
            }
            break;
          }
          elemental_fulfilled_services(ch, spirit, spir);
          break;
        }
      }
  }
  if (success_test(skill, force * 2) < 1) {
    send_to_char("You can't get your head around how to cast that spell.\r\n", ch);
    return;
  }
  GET_KARMA(ch) -= force * 100;
  struct spell_data *spell = new spell_data;
  if (GET_OBJ_VAL(obj, 1) == SPELL_INCATTR || GET_OBJ_VAL(obj, 1) == SPELL_INCCYATTR ||
      GET_OBJ_VAL(obj, 1) == SPELL_DECATTR || GET_OBJ_VAL(obj, 1) == SPELL_DECCYATTR) {
    strcpy(buf, spells[GET_OBJ_VAL(obj, 1)].name);
    sprintf(ENDOF(buf), " (%s)", attributes[GET_OBJ_VAL(obj, 3)]);
    spell->name = str_dup(buf);
  } else
    spell->name = str_dup(spells[GET_OBJ_VAL(obj, 1)].name);
  spell->type = GET_OBJ_VAL(obj, 1);
  spell->subtype = GET_OBJ_VAL(obj, 3);
  spell->force = force;
  spell->next = GET_SPELLS(ch);
  GET_SPELLS(ch) = spell;
  send_to_char(ch, "You spend %d karma and learn %s.\r\n", force, spell->name);
  extract_obj(obj);
}

ACMD(do_elemental)
{
  if (GET_ASPECT(ch) == ASPECT_SORCERER || GET_TRADITION(ch) == TRAD_MUNDANE || GET_TRADITION(ch) == TRAD_ADEPT || !GET_SKILL(ch, SKILL_CONJURING)) {
    send_to_char("You don't have the ability to do that.\r\n", ch);
    return;
  }
  if (!GET_NUM_SPIRITS(ch)) {
    send_to_char("You don't have any elementals bound to you.\r\n", ch);
    return;
  }
  int i = 1;
  strcpy(buf, "You currently have the following elementals bound:\r\n");
  for (struct spirit_data *elem = GET_SPIRIT(ch); elem; elem = elem->next, i++) {
    if (GET_TRADITION(ch) == TRAD_SHAMANIC)
      sprintf(ENDOF(buf), "%d) %-30s (Force %d) Services %d\r\n", i, GET_NAME(&mob_proto[real_mobile(spirits[elem->type].vnum)]), elem->force, elem->services);
    else
      sprintf(ENDOF(buf), "%d) %-30s (Force %d) Services: %d%10s\r\n", i, GET_NAME(&mob_proto[real_mobile(elements[elem->type].vnum)]),
              elem->force, elem->services, elem->called ? " Present" : " ");
  }
  send_to_char(buf, ch);
}

ACMD(do_banish)
{
  if (GET_ASPECT(ch) == ASPECT_SORCERER || GET_TRADITION(ch) == TRAD_ADEPT || GET_TRADITION(ch) == TRAD_MUNDANE || !GET_SKILL(ch, SKILL_CONJURING)) {
    send_to_char("You don't have the ability to do that.\r\n", ch);
    return;
  }
  if (AFF_FLAGGED(ch, AFF_BANISH)) {
    if (GET_EXTRA(ch)) {
      act("You give up trying to banish $n.", FALSE, FIGHTING(ch), 0, ch, TO_VICT);
      AFF_FLAGS(FIGHTING(ch)).RemoveBit(AFF_BANISH);
      stop_fighting(ch);
      AFF_FLAGS(ch).RemoveBit(AFF_BANISH);
    } else
      send_to_char(ch, "Try as you might, you can't wrest your mind away!\r\n");
    return;
  }
  struct char_data *mob;
  skip_spaces(&argument);
  if (!(mob = get_char_room_vis(ch, argument))) {
    send_to_char("Attempt to banish which spirit?\r\n", ch);
    return;
  }
  if (GET_RACE(mob) != CLASS_SPIRIT && GET_RACE(mob) != CLASS_ELEMENTAL) {
    send_to_char("You can only banish a nature spirit or elemental.\r\n", ch);
    return;
  }
  AFF_FLAGS(ch).SetBit(AFF_BANISH);
  AFF_FLAGS(mob).SetBit(AFF_BANISH);
  stop_fighting(ch);
  stop_fighting(mob);
  set_fighting(ch, mob);
  set_fighting(mob, ch);
  send_to_char(ch, "You begin attempting to banish %s.\r\n", GET_NAME(mob));
  act("$n begins attempting to banish $N.", FALSE, ch, 0, mob, TO_ROOM);
  GET_EXTRA(ch) = 1;
  GET_EXTRA(mob) = 0;
}

bool spirit_can_perform(int type, int order, int tradition)
{
  if (order == SERV_MATERIALIZE || order == SERV_DEMATERIAL || order == SERV_LEAVE || order == SERV_ATTACK)
    return TRUE;
  if (tradition == TRAD_HERMETIC) {
    if (order == SERV_ENGULF || order == SERV_APPEAR || order == SERV_SORCERY || order == SERV_STUDY || order == SERV_SUSTAIN)
      return TRUE;
    switch (type) {
    case ELEM_FIRE:
      if (order == SERV_FLAMEAURA || order == SERV_GUARD || order == SERV_FLAMETHROWER)
        return TRUE;
      break;
    case ELEM_AIR:
      if (order == SERV_MOVEMENT || order == SERV_BREATH || order == SERV_PSYCHOKINESIS)
        return TRUE;
      break;
    case ELEM_WATER:
    case ELEM_EARTH:
      if (order == SERV_MOVEMENT)
        return TRUE;
      break;
    }
  } else {
    if ((type != SPIRIT_STORM && type != SPIRIT_DESERT) && order == SERV_ACCIDENT)
      return TRUE;
    if ((type != SPIRIT_LAKE && type != SPIRIT_WIND) && order == SERV_CONCEAL)
      return TRUE;
    if (type != SPIRIT_STORM && order == SERV_GUARD)
      return TRUE;
    if ((type != SPIRIT_FOREST && type != SPIRIT_STORM && type != SPIRIT_MIST) && order == SERV_SEARCH)
      return TRUE;
    switch(type) {
    case SPIRIT_CITY:
      if (order == SERV_CONFUSION || order == SERV_FEAR)
        return TRUE;
      break;
    case SPIRIT_FIELD:
      break;
    case SPIRIT_HEARTH:
      if (order == SERV_CONFUSION)
        return TRUE;
      break;
    case SPIRIT_DESERT:
      if (order == SERV_MOVEMENT)
        return TRUE;
      break;
    case SPIRIT_FOREST:
      if (order == SERV_CONFUSION || order == SERV_FEAR)
        return TRUE;
      break;
    case SPIRIT_MOUNTAIN:
    case SPIRIT_PRAIRIE:
      if (order == SERV_MOVEMENT)
        return TRUE;
      break;
    case SPIRIT_MIST:
      if (order == SERV_CONFUSION || order == SERV_MOVEMENT)
        return TRUE;
      break;
    case SPIRIT_STORM:
      if (order == SERV_CONFUSION || order == SERV_FEAR)
        return TRUE;
      break;
    case SPIRIT_WIND:
      if (order == SERV_CONFUSION || order == SERV_MOVEMENT)
        return TRUE;
      break;
    case SPIRIT_LAKE:
      if (order == SERV_ENGULF || order == SERV_FEAR || order == SERV_MOVEMENT)
        return TRUE;
      break;
    case SPIRIT_RIVER:
      if (order == SERV_ENGULF || order == SERV_FEAR || order == SERV_MOVEMENT)
        return TRUE;
      break;
    case SPIRIT_SEA:
      if (order == SERV_CONFUSION || order == SERV_ENGULF || order == SERV_MOVEMENT)
        return TRUE;
      break;
    case SPIRIT_SWAMP:
      if (order == SERV_BINDING || order == SERV_CONFUSION || order == SERV_ENGULF || order == SERV_FEAR ||
          order == SERV_MOVEMENT)
        return TRUE;
      break;
    }
  }
  return FALSE;
}

void make_spirit_power(struct char_data *spirit, struct char_data *tch, int type)
{
  struct spirit_sustained *ssust = new spirit_sustained;
  ssust->type = type;
  ssust->caster = TRUE;
  ssust->target = tch;
  ssust->next = SPIRIT_SUST(spirit);
  SPIRIT_SUST(spirit) = ssust;
  ssust = new spirit_sustained;
  ssust->type = type;
  ssust->caster = FALSE;
  ssust->target = spirit;
  ssust->next = SPIRIT_SUST(tch);
  SPIRIT_SUST(tch) = ssust;
}

void stop_spirit_power(struct char_data *spirit, int type)
{
  struct spirit_sustained *temp;
  for (struct spirit_sustained *ssust = SPIRIT_SUST(spirit); ssust; ssust = ssust->next)
    if (ssust->type == type && ssust->caster == TRUE)
    {
      for (struct spirit_sustained *tsust = SPIRIT_SUST(ssust->target); tsust; tsust = tsust->next)
        if (tsust->type == type && tsust->target == spirit) {
          if (type == ENGULF) {
            if (IS_SPIRIT(spirit) || (IS_ELEMENTAL(spirit) && GET_SPARE1(spirit) == ELEM_WATER)) {
              act("The water surrounding $n falls away and soaks into the ground almost instantly.", TRUE, ssust->target, 0, 0, TO_ROOM);
              send_to_char("The water surrounding you suddenly vanishes allowing you to gasp for breath!", ssust->target);
            } else {
              switch (GET_SPARE1(spirit)) {
              case ELEM_FIRE:
                act("The fire engulfing $n suddenly goes out.", TRUE, ssust->target, 0, 0, TO_ROOM);
                send_to_char("The water fire surrounding you suddenly vanishes!", ssust->target);
                break;
              case ELEM_EARTH:
                act("The dirt and rock engulfing $n suddenly bursts apart, covering everyone close by.", TRUE, ssust->target, 0, 0, TO_ROOM);
                send_to_char("The earth surrounding you suddenly bursts open, allowing you to gasp for air!", ssust->target);
                break;
              case ELEM_AIR:
                act("$n begins to gasp for breath as though $s was just chocked.", TRUE, ssust->target, 0, 0, TO_ROOM);
                send_to_char("The mysterious force oppressing your lungs is suddenly gone!", ssust->target);
                break;
              }
            }
          } else if (type == CONFUSION)
            send_to_char("Your mind clears.\r\n", ssust->target);
          else if (type == CONCEAL)
            send_to_char("You suddenly feel very exposed.\r\n", ssust->target);
          else if (type == MOVEMENTUP || type == MOVEMENTDOWN)
            send_to_char("The force pushing you forward seems to lessen.\r\n", ssust->target);
          REMOVE_FROM_LIST(tsust, SPIRIT_SUST(ssust->target), next);
          delete [] tsust;
          break;
        }
      REMOVE_FROM_LIST(ssust, SPIRIT_SUST(spirit), next);
      delete [] ssust;
      break;
    }
}

POWER(spirit_appear)
{
  spiritdata->called = TRUE;
  spirit = create_elemental(ch, spiritdata->type, spiritdata->force, spiritdata->id, GET_TRADITION(ch));
  act("$n appears in the astral plane.", TRUE, spirit, 0, ch, TO_NOTVICT);
  send_to_char(ch, "%s heeds your call and arrives from the metaplanes ready to do your bidding.\r\n", CAP(GET_NAME(spirit)));
}

POWER(spirit_sorcery)
{
  if (!MOB_FLAGS(spirit).IsSet(MOB_AIDSORCERY)) {
    spiritdata->services--;
    MOB_FLAGS(spirit).SetBit(MOB_AIDSORCERY);
    send_to_char(ch, "%s will now asist you in casting a spell next time it is able.\r\n", CAP(GET_NAME(spirit)));
  } else
    send_to_char(ch, "%s will already asist in spell casting next time it is able.\r\n", CAP(GET_NAME(spirit)));

}

POWER(spirit_study)
{
  if (!MOB_FLAGS(spirit).IsSet(MOB_STUDY)) {
    spiritdata->services--;
    MOB_FLAGS(spirit).SetBit(MOB_STUDY);
    send_to_char(ch, "%s will now asist you in studying a spell next time it is able.\r\n", CAP(GET_NAME(spirit)));
  } else
    send_to_char(ch, "%s will already asist you the next time it is able.\r\n", CAP(GET_NAME(spirit)));
}

POWER(spirit_sustain)
{
  struct sustain_data *sust;
  if (GET_SUSTAINED_NUM(spirit))
    send_to_char("That spirit is already sustaining a spell.\r\n", ch);
  else {
    int i = atoi(arg);
    if (i <= 0 || i > GET_SUSTAINED_NUM(ch)) {
      send_to_char("You aren't sustaining that many spells.\r\n", ch);
      return;
    }
    for (sust = GET_SUSTAINED(ch); sust; sust = sust->next)
      if (sust->caster && --i == 0)
        break;
    if (sust->focus || sust->spirit) {
      send_to_char("You aren't sustaining that spell yourself.\r\n", ch);
      return;
    }
    if ((spells[sust->spell].category == MANIPULATION && spiritdata->type == ELEM_EARTH) ||
        (spells[sust->spell].category == COMBAT && spiritdata->type == ELEM_FIRE) ||
        (spells[sust->spell].category == ILLUSION && spiritdata->type == ELEM_WATER) ||
        (spells[sust->spell].category == DETECTION && spiritdata->type == ELEM_AIR)) {
      sust->spirit = spirit;
      GET_SUSTAINED_FOCI(ch)++;
      GET_SUSTAINED_NUM(spirit)++;
      spiritdata->services--;
      GET_SUSTAINED(spirit) = sust;
      send_to_char(ch, "%s sustains %s for you.\r\n", CAP(GET_NAME(spirit)), spells[sust->spell].name);
    } else
      send_to_char("That spirit can't sustain that type of spell.\r\n", ch);
  }
}

POWER(spirit_accident)
{
  struct char_data *tch = get_char_room_vis(spirit, arg);
  if (!tch)
    send_to_char("Use accident against which target?\r\n", ch);
  else if (tch == ch)
    send_to_char("You cannot target yourself with that power.\r\n", ch);
  else if (tch == spirit)
    send_to_char("The spirit refuses to perform that service.\r\n", ch);
  else {
    int success = success_test(MAX(GET_INT(tch), GET_QUI(tch)), GET_SPARE2(spirit));
    for (struct char_data *mob = world[spirit->in_room].people; mob; mob = mob->next)
      if (IS_NPC(mob) && (GET_RACE(mob) == CLASS_SPIRIT || GET_RACE(mob) == CLASS_ELEMENTAL) &&
          MOB_FLAGGED(mob, MOB_SPIRITGUARD)) {
        success = -1;
        break;
      }
    if (success < 1) {
      act("$n trips and stumbles.", TRUE, tch, 0, 0, TO_ROOM);
      send_to_char(tch, "You trip and stumble!");
      GET_INIT_ROLL(tch) -= 10;
    } else {
      sprintf(buf, "%s fails to cause $N to have an accident.", CAP(GET_NAME(spirit)));
      act(buf, TRUE, ch, 0, tch, TO_CHAR);
    }
    spiritdata->services--;
  }
}

POWER(spirit_binding)
{
  struct char_data *tch = get_char_room_vis(spirit, arg);
  if (!tch)
    send_to_char("Use binding against which target?\r\n", ch);
  else if (tch == spirit || tch == ch)
    send_to_char("The spirit refuses to perform that service.\r\n", ch);
  else {
    act("$N suddenly becomes incapable of movement!", FALSE, spirit, 0, ch, TO_VICT);
    send_to_char("You suddenly notice you are stuck fast to the ground!", tch);
    AFF_FLAGS(tch).SetBit(AFF_BINDING);
    tch->points.binding = GET_LEVEL(spirit) * 2;
    spiritdata->services--;
  }
}

POWER(spirit_conceal)
{
  struct char_data *tch = get_char_room_vis(spirit, arg);
  if (affected_by_power(spirit, CONCEAL)) {
    act("$N stops providing that services.", FALSE, ch, 0, spirit, TO_CHAR);
    stop_spirit_power(spirit, CONCEAL);
    return;
  }
  if (!tch)
    send_to_char("Use conceal against which target?\r\n", ch);
  else if (tch == spirit || affected_by_power(tch, CONCEAL))
    send_to_char("The spirit refuses to perform that service.\r\n", ch);
  else {
    act("$n vanishes from sight.", FALSE, spirit, 0, ch, TO_VICT);
    send_to_char("The terrain seems to cover your tracks.", tch);
    make_spirit_power(spirit, tch, CONCEAL);
    spiritdata->services--;
  }
}

POWER(spirit_confusion)
{
  struct char_data *tch = get_char_room_vis(spirit, arg);
  if (affected_by_power(spirit, CONFUSION)) {
    act("$N stops providing that services.", FALSE, ch, 0, spirit, TO_CHAR);
    stop_spirit_power(spirit, CONFUSION);
    return;
  }
  if (!tch)
    send_to_char("Use confusion against which target?\r\n", ch);
  else if (tch == spirit || tch == ch || affected_by_power(tch, CONFUSION))
    send_to_char("The spirit refuses to perform that service.\r\n", ch);
  else {
    act("$n vanishes from sight.", FALSE, spirit, 0, ch, TO_VICT);
    send_to_char("The terrain seems to cover your tracks.", tch);
    make_spirit_power(spirit, tch, CONFUSION);
    spiritdata->services--;
  }
}

POWER(spirit_engulf)
{
  struct char_data *tch = get_char_room_vis(spirit, arg);
  if (affected_by_power(spirit, ENGULF)) {
    act("$N stops providing that services.", FALSE, ch, 0, spirit, TO_CHAR);
    stop_spirit_power(spirit, ENGULF);
    return;
  }
  if (!tch)
    send_to_char("Use movement against which target?\r\n", ch);
  else if (tch == spirit || tch == ch || affected_by_power(tch, ENGULF))
    send_to_char("The spirit refuses to perform that service.\r\n", ch);
  else {
    act("$n rushes towards $N and attempts to engulf them!", FALSE, spirit, 0, tch, TO_ROOM);
    int target = GET_QUI(spirit), targskill = get_skill(tch, SKILL_UNARMED_COMBAT, target);
    int success = success_test(GET_QUI(spirit), targskill) - success_test(targskill, GET_QUI(spirit));
    if (success < 1) {
      act("$n successfully dodges the attack!", TRUE, ch, 0, 0, TO_ROOM);
      send_to_char("You successfully dodge!\r\n", tch);
      return;
    }
    if (IS_SPIRIT(spirit) || (IS_ELEMENTAL(spirit) && GET_SPARE1(spirit) == ELEM_WATER)) {
      act("The water in the air surrounding $n seems to quickly condense and engulf $s!", TRUE, tch, 0, 0, TO_ROOM);
      send_to_char("The water in the air around you seems to condense and swallow you up!", tch);
    } else {
      switch (GET_SPARE1(spirit)) {
      case ELEM_FIRE:
        act("A strange, unnatural fire suddenly engulfs $n!", TRUE, tch, 0, 0, TO_ROOM);
        send_to_char("Fire suddenly engulfs your entire body!", tch);
        break;
      case ELEM_EARTH:
        act("The ground seems to rise up from below and encase $n!", TRUE, tch, 0, 0, TO_ROOM);
        send_to_char("The earth surrounding you suddenly bursts open, allowing you to gasp for air!", tch);
        break;
      case ELEM_AIR:
        act("$n suddenly falls to $s knees and begins to wretch!", TRUE, tch, 0, 0, TO_ROOM);
        send_to_char("A huge pressure falls on your lungs and you find it impossible to breath!", tch);
        break;
      }
    }
    make_spirit_power(spirit, tch, ENGULF);
    set_fighting(spirit, tch);
    set_fighting(tch, spirit);
    GET_EXTRA(tch) = 0;
    spiritdata->services--;
  }
}

POWER(spirit_fear)
{
  struct char_data *tch = get_char_room_vis(spirit, arg);
  if (!tch)
    send_to_char("Use fear against which target?\r\n", ch);
  else if (tch == ch)
    send_to_char("You cannot target yourself with that power.\r\n", ch);
  else if (tch == spirit)
    send_to_char("The spirit refuses to perform that service.\r\n", ch);
  else {
    int success = success_test(GET_SPARE2(spirit), GET_WIL(tch)) - success_test(GET_WIL(tch), GET_SPARE2(spirit));
    if (success < 1) {
      send_to_char("A dark shadow passes over your mind, but is quickly gone.\r\n", tch);
      sprintf(buf, "%s fails to instill fear in $N.", CAP(GET_NAME(spirit)));
      act(buf, FALSE, ch, 0, tch, TO_CHAR);
    } else {
      AFF_FLAGS(tch).SetBit(AFF_FEAR);
      send_to_char("An all consuming terror overcomes you!\r\n", tch);
      sprintf(buf, "%s succeeds in instilling fear in $N.", CAP(GET_NAME(spirit)));
      act(buf, FALSE, ch, 0, tch, TO_CHAR);
      extern ACMD(do_flee);
      do_flee(tch, "", 0, 0);
    }
    spiritdata->services--;
  }
}

POWER(spirit_flameaura)
{
  if (!MOB_FLAGGED(spirit, MOB_FLAMEAURA))
    act("$n's body erupts in flames.", TRUE, spirit, 0, 0, TO_ROOM);
  else
    act("The flames surrounding $n subsides.", TRUE, spirit, 0, 0, TO_ROOM);
  MOB_FLAGS(spirit).ToggleBit(MOB_FLAMEAURA);
  spiritdata->services--;
}

POWER(spirit_flamethrower)
{
  struct char_data *tch = get_char_room_vis(spirit, arg);
  if (!tch)
    send_to_char("Use flamethrower against which target?\r\n", ch);
  else if (tch == ch)
    send_to_char("You cannot target yourself with that power.\r\n", ch);
  else if (tch == spirit)
    send_to_char("The spirit refuses to perform that service.\r\n", ch);
  else {
    sprintf(buf, "moderate %s", arg);
    cast_spell(spirit, SPELL_FLAMETHROWER, 0, GET_LEVEL(spirit), buf);
  }
}

POWER(spirit_guard)
{
  if (!MOB_FLAGGED(spirit, MOB_SPIRITGUARD))
    act("$n begins to guard the area from accidents.", FALSE, spirit, 0, ch, TO_VICT);
  else
    act("$n stops guarding the area from accidents.", FALSE, spirit, 0, ch, TO_VICT);
  MOB_FLAGS(spirit).ToggleBit(MOB_SPIRITGUARD);
  spiritdata->services--;
}

POWER(spirit_leave)
{
  act("$n vanishes back into the metaplanes.", TRUE, spirit, 0, ch, TO_NOTVICT);
  send_to_char(ch, "%s returns to the metaplanes to await further orders.\r\n", CAP(GET_NAME(spirit)));
  spiritdata->called = FALSE;
  extract_char(spirit);
}

POWER(spirit_dematerialize)
{
  act("$n fades from the physical realm.\r\n", TRUE, spirit, 0, ch, TO_ROOM);
  MOB_FLAGS(spirit).RemoveBits(MOB_DUAL_NATURE, MOB_FLAMEAURA, ENDBIT);
  MOB_FLAGS(spirit).SetBit(MOB_ASTRAL);
  spiritdata->services--;
}

POWER(spirit_materialize)
{
  if (IS_DUAL(spirit)) {
    send_to_char(ch, "%s is already materialized.\r\n", CAP(GET_NAME(spirit)));
    return;
  }
  MOB_FLAGS(spirit).SetBit(MOB_DUAL_NATURE);
  MOB_FLAGS(spirit).RemoveBit(MOB_ASTRAL);
  spiritdata->services--;
  act("$n takes on a physical form.\r\n", TRUE, spirit, 0, ch, TO_ROOM);
}

POWER(spirit_movement)
{
  int increase = 0;
  if (affected_by_power(spirit, MOVEMENTUP)) {
    act("$N stops providing that services.", FALSE, ch, 0, spirit, TO_CHAR);
    stop_spirit_power(spirit, MOVEMENTUP);
    return;
  }
  if (affected_by_power(spirit, MOVEMENTDOWN)) {
    act("$N stops providing that services.", FALSE, ch, 0, spirit, TO_CHAR);
    stop_spirit_power(spirit, MOVEMENTDOWN);
    return;
  }
  two_arguments(arg, buf, buf1);
  if (!(*buf1 || *buf)) {
    send_to_char("Do you want to increase or decrease the movement of the target?\r\n", ch);
    return;
  }
  if (is_abbrev(buf1, "increase"))
    increase = 1;
  else if (is_abbrev(buf1, "decrease"))
    increase = -1;
  if (!increase) {
    send_to_char("You must specify increase or decrease.\r\n", ch);
    return;
  }
  struct char_data *tch = get_char_room_vis(spirit, buf);
  if (!tch)
    send_to_char("Use movement against which target?\r\n", ch);
  else if (tch == spirit || affected_by_power(tch, increase > 0 ? MOVEMENTUP : MOVEMENTDOWN))
    send_to_char("The spirit refuses to perform that service.\r\n", ch);
  else {
    act("$n performs that service for you.", FALSE, spirit, 0, ch, TO_VICT);
    if (increase > 0)
      send_to_char("You feel something pushing you forward faster.\r\n", tch);
    else
      send_to_char("You face heavy resistance as you try and move forward.\r\n", ch);
    make_spirit_power(spirit, tch, increase > 0 ? MOVEMENTUP : MOVEMENTDOWN);
    spiritdata->services--;
  }
}

POWER(spirit_breath)
{
  struct char_data *tch = get_char_room_vis(spirit, arg);
  if (!tch)
    send_to_char("Use noxious breath against which target?\r\n", ch);
  else if (tch == ch)
    send_to_char("You cannot target yourself with that power.\r\n", ch);
  else if (tch == spirit)
    send_to_char("The spirit refuses to perform that service.\r\n", ch);
  else {
    act("$n turns towards $N as a cloud of noxious fumes forms around $S.", TRUE, spirit, 0, tch, TO_NOTVICT);
    act("$n lets forth a stream of noxious fumes in your direction.", FALSE, spirit, 0, tch, TO_VICT);
    int dam = convert_damage(stage(-success_test(MAX(GET_WIL(tch), GET_BOD(tch)), GET_SPARE2(spirit)), SERIOUS));
    if (dam > 0) {
      act("$n chokes and coughs.", TRUE, tch, 0, 0, TO_ROOM);
      send_to_char("The noxious fumes fill your throat and lungs, causing your vision to swim.\r\n", tch);
      damage(spirit, tch, dam, TYPE_FUMES, MENTAL);
    } else {
      act("$n waves $s hand infront of $s face, dispersing the fumes.", TRUE, tch, 0, 0, TO_ROOM);
      send_to_char("You easily resist the noxious fumes.\r\n", tch);
    }
    spiritdata->services--;
  }
}

POWER(spirit_attack)
{
  struct char_data *tch = get_char_room_vis(spirit, arg);
  if (!tch)
    send_to_char("Use attack which target?\r\n", ch);
  else if (tch == ch)
    send_to_char("Ordering your own spirit to attack you is not a good idea.\r\n", ch);
  else if (tch == spirit)
    send_to_char("The spirit refuses to perform that service.\r\n", ch);
  else {
    check_killer(ch, tch);
    set_fighting(spirit, tch);
    spiritdata->services--;
  }
}

POWER(spirit_psychokinesis)
{}

POWER(spirit_search)
{}

struct order_data services[] =
  {
    {"Appear", spirit_appear, 0
    },
    {"Sorcery", spirit_sorcery, 0},
    {"Study", spirit_study, 0},
    {"Sustain", spirit_sustain, 0},
    {"Accident", spirit_accident, 1},
    {"Binding", spirit_binding, 1},
    {"Concealment", spirit_conceal, 1},
    {"Confusion", spirit_confusion, 0},
    {"Dematerialize", spirit_dematerialize, 1},
    {"Engulf", spirit_engulf, 1},
    {"Fear", spirit_fear, 0},
    {"Flame Aura", spirit_flameaura, 1},
    {"Flamethrower", spirit_flamethrower, 1},
    {"Guard", spirit_guard, 1},
    {"Leave", spirit_leave, 0},
    {"Materialize", spirit_materialize, 0},
    {"Movement", spirit_movement, 1},
    {"Breath", spirit_breath, 1},
    {"Psychokinesis", spirit_psychokinesis, 1},
    {"Search", spirit_search, 1},
    {"Attack", spirit_attack, 1}
  };

ACMD(do_order)
{
  if (GET_ASPECT(ch) == ASPECT_SORCERER || GET_TRADITION(ch) == TRAD_ADEPT || GET_TRADITION(ch) == TRAD_MUNDANE || !GET_SKILL(ch, SKILL_CONJURING)) {
    send_to_char("You don't have the ability to do that.\r\n", ch);
    return;
  }
  half_chop(argument, buf, buf1);
  struct spirit_data *spirit;
  int i, order = 0;
  if (!(i = atoi(buf)) || i > GET_NUM_SPIRITS(ch)) {
    send_to_char("Which spirit do you wish to give an order to?\r\n", ch);
    return;
  }
  for (spirit = GET_SPIRIT(ch); spirit; spirit = spirit->next)
    if (--i == 0)
      break;
  if (!*buf1) {
    send_to_char("Available Services: \r\n", ch);
    if (GET_TRADITION(ch) == TRAD_HERMETIC) {
      if (!spirit->called)
        send_to_char("  ^WAppear^n\r\n", ch);
      else {
        send_to_char("  Aid ^WSorcery^n\r\n"
                     "  Aid ^WStudy^n\r\n"
                     "  ^WSustain^n Spell\r\n"
                     "  ^WMaterialize^n\r\n"
                     "  ^WDematerialize^n\r\n"
                     "  ^WLeave^n\r\n"
                     "  ^WEngulf^n\r\n", ch);
        switch(spirit->type) {
        case ELEM_FIRE:
          send_to_char("  ^WFlame Aura^n\r\n"
                       "  ^WFlamethrower^n\r\n", ch);
          break;
        case ELEM_WATER:
        case ELEM_EARTH:
          send_to_char("  ^WMovement^n\r\n", ch);
          break;
        case ELEM_AIR:
          send_to_char("  ^WMovement^n\r\n"
                       "  Noxious ^WBreath^n\r\n"
                       "  ^WPsychokinesis^n\r\n", ch);
          break;
        }
      }
    } else {
      send_to_char("  ^WMaterialize^n\r\n"
                   "  ^WDematerialize^n\r\n", ch);
      if (spirit_can_perform(spirit->type, SERV_ACCIDENT, TRAD_SHAMANIC))
        send_to_char("  ^WAccident^n\r\n", ch);
      if (spirit_can_perform(spirit->type, SERV_BINDING, TRAD_SHAMANIC))
        send_to_char("  ^WBinding^n\r\n", ch);
      if (spirit_can_perform(spirit->type, SERV_CONCEAL, TRAD_SHAMANIC))
        send_to_char("  ^WConcealment^n\r\n", ch);
      if (spirit_can_perform(spirit->type, SERV_CONFUSION, TRAD_SHAMANIC))
        send_to_char("  ^WConfusion^n\r\n", ch);
      if (spirit_can_perform(spirit->type, SERV_ENGULF, TRAD_SHAMANIC))
        send_to_char("  ^WEngulf^n\r\n", ch);
      if (spirit_can_perform(spirit->type, SERV_FEAR, TRAD_SHAMANIC))
        send_to_char("  ^WFear^n\r\n", ch);
      if (spirit_can_perform(spirit->type, SERV_GUARD, TRAD_SHAMANIC))
        send_to_char("  ^WGuard^n\r\n", ch);
      if (spirit_can_perform(spirit->type, SERV_MOVEMENT, TRAD_SHAMANIC))
        send_to_char("  ^WMovement^n\r\n", ch);
      if (spirit_can_perform(spirit->type, SERV_BREATH, TRAD_SHAMANIC))
        send_to_char("  Noxious ^WBreath^n\r\n", ch);
      if (spirit_can_perform(spirit->type, SERV_PSYCHOKINESIS, TRAD_SHAMANIC))
        send_to_char("  ^WPsychokinesis^n\r\n", ch);
      if (spirit_can_perform(spirit->type, SERV_SEARCH, TRAD_SHAMANIC))
        send_to_char("  ^WSearch^n\r\n", ch);
    }
    send_to_char("  Attack\r\n", ch);
  } else {
    half_chop(buf1, buf, buf2);
    for (;order < NUM_SERVICES; order++)
      if (is_abbrev(buf, services[order].name) && spirit_can_perform(spirit->type, order, GET_TRADITION(ch)))
        break;
    if (order == NUM_SERVICES) {
      send_to_char("Which service do you wish to order the elemental to perform?\r\n", ch);
      return;
    }
    if (GET_TRADITION(ch) == TRAD_HERMETIC) {
      if (order == SERV_APPEAR && spirit->called) {
        send_to_char("That elemental is already here!\r\n", ch);
        return;
      } else if (!spirit->called && order != SERV_APPEAR) {
        send_to_char("That elemental is waiting on the metaplanes.\r\n", ch);
        return;
      }
    }
    struct char_data *mob = find_spirit_by_id(spirit->id, GET_IDNUM(ch));
    if (services[order].type == 1 && MOB_FLAGGED(mob, MOB_ASTRAL)) {
      send_to_char("That elemental must materialize before it can use that power.\r\n", ch);
      return;
    }
    if (spirit->services < 1 && order != SERV_LEAVE) {
      send_to_char("The spirit no longer listens to you.\r\n", ch);
      return;
    }
    ((*services[order].func) (ch, mob, spirit, buf2));
    if (order != SERV_LEAVE)
      elemental_fulfilled_services(ch, mob, spirit);
  }
}

ACMD(do_domain)
{
  if (GET_TRADITION(ch) != TRAD_SHAMANIC) {
    send_to_char("You have no sense of domain.\r\n", ch);
    return;
  }
  if (!*argument) {
    send_to_char(ch, "You are currently in the %s domain.\r\n", spirits[GET_DOMAIN(ch)].name);
    if (SECT(ch->in_room) != SPIRIT_FOREST && SECT(ch->in_room) != SPIRIT_HEARTH && !ROOM_FLAGGED(ch->in_room, ROOM_INDOORS)) {
      send_to_char(ch, "You can switch to the following domains:\r\n  %s\r\n  Sky Spirit\r\n", spirits[SECT(ch->in_room)].name);
    }
  } else {
    struct spirit_data *next;
    skip_spaces(&argument);
    int newdomain = -1;
    if (SECT(ch->in_room) != SPIRIT_FOREST && SECT(ch->in_room) != SPIRIT_HEARTH && !ROOM_FLAGGED(ch->in_room, ROOM_INDOORS)) {
      if (is_abbrev(argument, spirits[SECT(ch->in_room)].name))
        newdomain = SECT(ch->in_room);
      else if (is_abbrev(argument, "sky spirit"))
        newdomain = SPIRIT_SKY;
    }
    if (newdomain == GET_DOMAIN(ch))
      send_to_char("You are already focusing on that domain.\r\n", ch);
    else if (newdomain == -1)
      send_to_char("Which domain do you wish to focus on?\r\n", ch);
    else {
      GET_DOMAIN(ch) = newdomain;
      send_to_char(ch, "You switch your focus to the %s domain.\r\n", GET_DOMAIN(ch) == SPIRIT_SKY ? "Sky spirit" : spirits[GET_DOMAIN(ch)].name);
      for (struct spirit_data *spirit = GET_SPIRIT(ch); spirit; spirit = next) {
        next = spirit->next;
        if (GET_DOMAIN(ch) != spirit->type) {
          struct spirit_data *temp;
          struct char_data *tch = find_spirit_by_id(spirit->id, GET_IDNUM(ch));
          if (tch) {
            send_to_char(ch, "You release %s from the rest of its services.\r\n", GET_NAME(tch));
            act("$n returns to the metaplanes.", TRUE, tch, 0, ch, TO_NOTVICT);
            extract_char(tch);
          }
          REMOVE_FROM_LIST(spirit, GET_SPIRIT(ch), next);
          GET_NUM_SPIRITS(ch)--;
          delete [] spirit;
        }
      }
    }
  }
}

ACMD(do_drain)
{
  if (!GET_MAGIC(ch)) {
    send_to_char("You don't have any spell pool dice to allocate.\r\n", ch);
    return;
  }
  skip_spaces(&argument);
  int i = atoi(argument);
  if (i > GET_MAGIC(ch))
    send_to_char(ch, "You may only allocate up to %d dice for drain tests.\r\n", GET_MAGIC(ch));
  else {
    GET_DRAIN(ch) = ch->real_abils.drain_pool = i;
    GET_CASTING(ch) = GET_MAGIC(ch) - i;
    send_to_char(ch, "You are now allocating %d dice to drain resistance and %d dice to casting.\r\n", GET_DRAIN(ch), GET_CASTING(ch));
  }
}
void sedit_parse(struct descriptor_data *d, char *arg)
{}

ACMD(do_deactivate)
{
  skip_spaces(&argument);
  struct obj_data *obj;
  int i;
  if (!GET_FOCI(ch)) {
    send_to_char("You have no foci activated!\r\n", ch);
    return;
  }
  if (!(obj = get_object_in_equip_vis(ch, argument, ch->equipment, &i)) &&
      !(obj = get_obj_in_list_vis(ch, argument, ch->carrying))) {
    send_to_char("Deactive which focus?\r\n", ch);
    return;
  }
  if (GET_OBJ_VAL(obj, 4) < 1) {
    send_to_char("That focus isn't activated.\r\n", ch);
    return;
  }
  GET_OBJ_VAL(obj, 4) = 0;
  GET_FOCI(ch)--;
  affect_total(ch);
  send_to_char(ch, "You deactivate %s.\r\n", GET_OBJ_NAME(obj));
}

ACMD(do_destroy)
{
  if (!*argument) {
    send_to_char("Destroy which magic construct?\r\n", ch);
    return;
  }
  skip_spaces(&argument);
  struct obj_data *obj;
  if (!(obj = get_obj_in_list_vis(ch, argument, world[ch->in_room].contents))) {
    send_to_char("That object isn't here.\r\n", ch);
    return;
  }
  if (GET_OBJ_TYPE(obj) == ITEM_MAGIC_TOOL && (GET_OBJ_VAL(obj, 0) == TYPE_LODGE || GET_OBJ_VAL(obj, 0) == TYPE_CIRCLE)) {
    if (GET_OBJ_VAL(obj, 0) == TYPE_LODGE) {
      send_to_char(ch, "You kick at the supports and smash the talismans.\r\n");
      act("$n roughly pulls down $o.", TRUE, ch, obj, 0, TO_ROOM);
    } else {
      send_to_char(ch, "You rub your feet along the lines making up the hermetic circle, erasing them.\r\n");
      act("$n uses $s feet to rub out $o.", TRUE, ch, obj, 0, TO_ROOM);
    }
    extract_obj(obj);
  } else
    send_to_char("You can't destroy that object.\r\n", ch);
}

ACMD(do_track)
{
  if (!IS_PROJECT(ch)) {
    send_to_char("You have to be projecting to astrally track.\r\n", ch);
    return;
  }
  if (AFF_FLAGGED(ch->desc->original, AFF_TRACKING)) {
    AFF_FLAGS(ch->desc->original).RemoveBit(AFF_TRACKING);
    send_to_char("You stop searching the astral plane.\r\n", ch);
    return;
  }
  int success;
  skip_spaces(&argument);
  if (!*argument) {
    if (HUNTING(ch->desc->original)) {
      success = success_test(MAX(GET_INT(ch), GET_MAG(ch)/100), HOURS_SINCE_TRACK(ch->desc->original));
      if (success > 0) {
        AFF_FLAGS(ch->desc->original).SetBit(AFF_TRACKING);
        send_to_char("You pick up the trail and continue tracking the astral signature.\r\n", ch);
      } else {
        HUNTING(ch->desc->original) = NULL;
        send_to_char("You seem to have lost the trail.\r\n", ch);
      }
    } else
      send_to_char("What do you wish to track the owner of?\r\n", ch);
    return;
  }
  struct char_data *vict;
  struct obj_data *obj;
  two_arguments(argument, buf, buf1);
  if (!generic_find(buf,  FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_OBJ_EQUIP |
                    FIND_CHAR_ROOM, ch, &vict, &obj)) {
    send_to_char("You don't see that here.\r\n", ch);
    return;
  }
  if (vict) {
    if (*buf1) {
      obj = NULL;
      two_arguments(buf1, buf2, argument);
      int i = 0;
      if (!str_cmp(buf1, "spell")) {
        int spell = 0;
        if (*buf2)
          spell = atoi(buf2);
        for (struct sustain_data *sust = GET_SUSTAINED(vict); sust; sust = sust->next)
          if (!sust->caster && !spell--) {
            vict = sust->other;
            break;
          }
      } else if (!(obj = get_object_in_equip_vis(vict, buf1, vict->equipment, &i))) {
        send_to_char("They aren't wearing that.\r\n", ch);
        return;
      }
    } else if (IS_NPC(vict) && (GET_RACE(vict) == CLASS_SPIRIT || GET_RACE(vict) == CLASS_ELEMENTAL) && GET_ACTIVE(vict)) {
      for (struct descriptor_data *desc = descriptor_list; desc; desc = desc->next)
        if (desc->original && GET_IDNUM(desc->original) == GET_ACTIVE(vict)) {
          vict = desc->original;
          break;
        } else if (desc->character && GET_IDNUM(desc->character) == GET_ACTIVE(vict)) {
          vict = desc->character;
          break;
        }
    } else {
      send_to_char("They are right here!\r\n", ch);
      return;
    }
  }
  if (obj) {
    vict = NULL;
    if (GET_OBJ_TYPE(obj) != ITEM_FOCUS && !(GET_OBJ_TYPE(obj) == ITEM_MAGIC_TOOL && (GET_OBJ_VAL(obj, 0) == TYPE_CIRCLE || GET_OBJ_VAL(obj, 0) == TYPE_LODGE))) {
      send_to_char("There is no astral signature present on that item.\r\n", ch);
      return;
    }
    int num = 3;
    if (GET_OBJ_TYPE(obj) == ITEM_FOCUS)
      num = 2;
    if (!GET_OBJ_VAL(obj, num)) {
      send_to_char("There is no astral signature present on that item.\r\n", ch);
      return;
    }
    for (struct descriptor_data *desc = descriptor_list; desc; desc = desc->next)
      if (desc->original && GET_IDNUM(desc->original) == GET_OBJ_VAL(obj, num)) {
        vict = desc->original;
        break;
      } else if (desc->character && GET_IDNUM(desc->character) == GET_OBJ_VAL(obj, num)) {
        vict = desc->character;
        break;
      }
  }
  if (vict) {
    if (vict->in_room == ch->in_room) {
      send_to_char("They're right here!\r\n", ch);
      return;
    }
    HUNTING(ch->desc->original) = vict;
    AFF_FLAGS(ch->desc->original).SetBit(AFF_TRACKING);
    HOURS_SINCE_TRACK(ch->desc->original) = 0;
    HOURS_LEFT_TRACK(ch->desc->original) = MAX(1, 4 / success_test(GET_INT(ch), 4));
    AFF_FLAGS(vict).SetBit(AFF_TRACKED);
    send_to_char("You begin to trace that astral signature back to its owner.\r\n", ch);
  } else {
    send_to_char("You can't seem to track them down.\r\n", ch);
    return;
  }
}

ACMD(do_manifest)
{
  if (!IS_PROJECT(ch)) {
    send_to_char("You can't manifest when you're not astrally projecting.\r\n", ch);
    return;
  }
  AFF_FLAGS(ch).ToggleBit(AFF_MANIFEST);
  if (AFF_FLAGGED(ch, AFF_MANIFEST)) {
    act("The ghostly image of $n fades slowly into view.", TRUE, ch, 0, 0, TO_ROOM);
    send_to_char("You manifest your presence on the physical plane.\r\n", ch);
  } else {
    act("$n fades from the physical plane.", TRUE, ch, 0, 0, TO_ROOM);
    send_to_char("You vanish back into the astral plane.\r\n", ch);
  }
}

ACMD(do_dispell)
{
  if (GET_ASPECT(ch) == ASPECT_CONJURER || GET_TRADITION(ch) == TRAD_ADEPT || GET_TRADITION(ch) == TRAD_MUNDANE || !GET_SKILL(ch, SKILL_SORCERY)) {
    send_to_char("You don't have the ability to do that.\r\n", ch);
    return;
  }
  if (!*argument) {
    send_to_char("What spell do you wish to dispell?\r\n", ch);
    return;
  }
  skip_spaces(&argument);
  struct char_data *vict;
  two_arguments(argument, buf, buf2);
  if (!(vict = get_char_room_vis(ch, buf))) {
    send_to_char("You don't see them here.\r\n", ch);
    return;
  }
  int x = atoi(buf2);
  if (x <= 0) {
    send_to_char("Dispell which spell?\r\n", ch);
    return;
  }
  struct sustain_data *sust = GET_SUSTAINED(vict);
  for (;sust; sust = sust->next)
    if (!sust->caster && !--x)
      break;
  if (!sust) {
    send_to_char("They don't have that many spells cast on them.\r\n", ch);
    return;
  }
  int success = success_test(GET_SKILL(ch, SKILL_SORCERY) + GET_CASTING(ch), sust->force);
  int type = sust->spell, force = sust->force, drain = sust->drain;
  if (success > 0) {
    spell_modify(vict, sust, FALSE);
    sust->success -= success;
    if (sust->success < 1) {
      send_to_char("You succeed in completly dispelling that spell.\r\n", ch);
      sust->success += success;
      spell_modify(vict, sust, TRUE);
      end_sustained_spell(vict, sust);
    } else {
      spell_modify(vict, sust, TRUE);
      send_to_char("You succeed in weakening that spell.\r\n", ch);
    }
  } else
    send_to_char("You fail to weaken that spell.\r\n", ch);
  spell_drain(ch, type, force, drain);
}

ACMD(do_heal)
{
  if (GET_TRADITION(ch) != TRAD_ADEPT || !GET_POWER(ch, ADEPT_EMPATHICHEAL)) {
    send_to_char("You don't have the ability to do that.\r\n", ch);
    return;
  }
  struct char_data *vict;
  skip_spaces(&argument);
  if (!*argument) {
    send_to_char("Who do you wish to heal?\r\n", ch);
    return;
  }
  two_arguments(argument, arg, buf);
  if (!*buf)
    send_to_char("What damage level do you wish to heal them at?\r\n", ch);
  else if (GET_POS(ch) == POS_FIGHTING)
    send_to_char("You are too busy to do that!\r\n", ch);
  else if (!(vict = get_char_room_vis(ch, arg)))
    send_to_char("You don't see them here.\r\n", ch);
  else if (GET_PHYSICAL(ch) <= 100)
    send_to_char("Succeeding in that task would surely kill you.\r\n", ch);
  else if (GET_PHYSICAL(vict) == GET_MAX_PHYSICAL(vict))
    send_to_char("They don't need your help.\r\n", ch);
  else if (GET_POS(vict) > POS_LYING)
    send_to_char("They have to be lying down to receive your help.\r\n", ch);
  else {
    int basedamage = 0;
    for (; *wound_name[basedamage] != '\n'; basedamage++)
      if (is_abbrev(buf, wound_name[basedamage]))
        break;
    if (basedamage > 4 || basedamage == 0) {
      send_to_char("That is not a valid damage level, please choose between Light, Moderate, Serious and Deadly.\r\n", ch);
      return;
    }
    int success = success_test(GET_MAG(ch) / 100, 10 - (GET_ESS(vict) / 100));
    if (success < 1) {
      send_to_char("You fail to channel your energy into that pursuit.\r\n", ch);
      return;
    }
    success = MIN(damage_array[basedamage], MIN((GET_PHYSICAL(ch) / 100) - 1, success)) * 100;
    success = MIN(GET_MAX_PHYSICAL(vict) - GET_PHYSICAL(vict), success);
    GET_PHYSICAL(vict) += success;
    GET_PHYSICAL(ch) -= success;
    WAIT_STATE(ch, 3 RL_SEC);
    act("You feel $n place $s hands on you, $s minstrations seem to cause your wounds to fade!", TRUE, ch, 0, vict, TO_VICT);
    act("You place your hands on $N, you feel $S pain and suffering transfered to your body!", TRUE, ch, 0, vict, TO_CHAR);
    act("$n places his hands on $N seemingly transfering the wound to $mself!", TRUE, ch, 0, vict, TO_NOTVICT);
  }
}

ACMD(do_relieve)
{
  if (GET_TRADITION(ch) != TRAD_ADEPT || !GET_POWER(ch, ADEPT_PAINRELIEF)) {
    send_to_char("You don't have the ability to do that.\r\n", ch);
    return;
  }
  struct char_data *vict;
  skip_spaces(&argument);
  if (!*argument) {
    send_to_char("Who do you wish to heal?\r\n", ch);
    return;
  }
  if (GET_POS(ch) == POS_FIGHTING)
    send_to_char("You are too busy to do that!\r\n", ch);
  else if (!(vict = get_char_room_vis(ch, argument)))
    send_to_char("You don't see them here.\r\n", ch);
  else if (GET_MENTAL(vict) == GET_MAX_MENTAL(vict))
    send_to_char("They don't need your help.\r\n", ch);
  else if (GET_POS(vict) > POS_LYING)
    send_to_char("They have to be lying down to receive your help.\r\n", ch);
  else {
    int success = success_test(GET_MAG(ch) / 100, 10 - (GET_ESS(vict) / 100));
    if (success < 1) {
      send_to_char("You fail to channel your energy into that pursuit.\r\n", ch);
      return;
    }
    if (GET_MENTAL(vict) <= 0)
      GET_MENTAL(vict) = (int)(GET_MAX_MENTAL(vict) / 0.4);
    else if (GET_MENTAL(vict) <= GET_MAX_MENTAL(vict) / 0.4)
      GET_MENTAL(vict) = (int)(GET_MAX_MENTAL(vict) / 0.7);
    else if (GET_MENTAL(vict) <= GET_MAX_MENTAL(vict) / 0.7)
      GET_MENTAL(vict) = (int)(GET_MAX_MENTAL(vict) / 0.9);
    else
      GET_MENTAL(vict) = GET_MAX_MENTAL(vict);
    act("You feel $n place $s hands on you, a tingling feeling feels your body as your pain begins to subside", TRUE, ch, 0, vict, TO_VICT);
    act("You hands warm slightly at the energy being channeled through them into $N's chakra points.", TRUE, ch, 0, vict, TO_CHAR);
    act("$n places his hands on $N. $N appears to feel better.", TRUE, ch, 0, vict, TO_NOTVICT);
    WAIT_STATE(ch, 5 RL_SEC);
  }
}


ACMD(do_nervestrike)
{
  if (GET_TRADITION(ch) != TRAD_ADEPT || !GET_POWER(ch, ADEPT_NERVE_STRIKE)) {
    send_to_char("You don't have the ability to do that.\r\n", ch);
    return;
  }
  if (IS_NERVE(ch)) {
    IS_NERVE(ch) = FALSE;
    send_to_char("You will no longer use nerve strike.\r\n", ch);
  } else {
    IS_NERVE(ch) = TRUE;
    send_to_char("You will now use nerve strike.\r\n", ch);
  }
}
