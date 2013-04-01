// Sorry, but *snicker* - Che

#include "structs.h"
#include "awake.h"
#include "db.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "utils.h"
#include "screen.h"
#include "constants.h"
#include "olc.h"
#include "newmagic.h"

#define CH d->character
#define PEDIT_MENU 0
#define PEDIT_TYPE 1
#define PEDIT_NAME 2
#define PEDIT_RATING 3
#define PEDIT_WOUND 4

extern void part_design(struct char_data *ch, struct obj_data *design);


void pedit_disp_menu(struct descriptor_data *d)
{
  CLS(CH);
  send_to_char(CH, "1) Name: ^c%s^n\r\n", d->edit_obj->restring);
  send_to_char(CH, "2) Type: ^c%s^n\r\n", programs[GET_OBJ_VAL(d->edit_obj, 0)].name);
  send_to_char(CH, "3) Rating: ^c%d^n\r\n", GET_OBJ_VAL(d->edit_obj, 1));
  if (GET_OBJ_VAL(d->edit_obj, 0) == 5)
  {
    send_to_char(CH, "4) Damage: ^c%s^n\r\n", wound_name[GET_OBJ_VAL(d->edit_obj, 2)]);
    send_to_char(CH, "Size: ^c%d^n\r\n", (GET_OBJ_VAL(d->edit_obj, 1) * GET_OBJ_VAL(d->edit_obj, 1)) * attack_multiplier[GET_OBJ_VAL(d->edit_obj, 2)]);
  } else
    send_to_char(CH, "Size: ^c%d^n\r\n", (GET_OBJ_VAL(d->edit_obj, 1) * GET_OBJ_VAL(d->edit_obj, 1)) * programs[GET_OBJ_VAL(d->edit_obj, 0)].multiplier);
  send_to_char(CH, "q) Quit\r\nEnter your choice: ");
  d->edit_mode = PEDIT_MENU;
}

void pedit_disp_program_menu(struct descriptor_data *d)
{
  CLS(CH);
  for (int counter = 1; counter < NUM_PROGRAMS; counter += 3)
  {
    send_to_char(CH, "  %2d) %-15s %2d) %-15s %2d) %-15s\r\n",
                 counter, programs[counter].name,
                 counter + 1, counter + 1 < NUM_PROGRAMS ?
                 programs[counter + 1].name : "", counter + 2, counter + 2 < NUM_PROGRAMS ?
                 programs[counter + 2].name : "");
  }
  send_to_char("Program type: ", d->character);
  d->edit_mode = PEDIT_TYPE;
}
void pedit_parse(struct descriptor_data *d, char *arg)
{
  int number = atoi(arg);
  switch(d->edit_mode)
  {
  case PEDIT_MENU:
    switch (*arg) {
    case '1':
      send_to_char(CH, "What do you want to call this program: ");
      d->edit_mode = PEDIT_NAME;
      break;
    case '2':
      pedit_disp_program_menu(d);
      break;
    case '3':
      if (!GET_OBJ_VAL(d->edit_obj, 0))
        send_to_char(CH, "Choose a program type first!\r\n");
      else {
        send_to_char(CH,"Enter Rating: ");
        d->edit_mode = PEDIT_RATING;
      }
      break;
    case '4':
      if (GET_OBJ_VAL(d->edit_obj, 0) != 5)
        send_to_char(CH, "Invalid option!\r\n");
      else {
        CLS(CH);
        send_to_char(CH, "1) Light\r\n2) Moderate\r\n3) Serious\r\n4) Deadly\r\nEnter your choice: ");
        d->edit_mode = PEDIT_WOUND;
      }
      break;
    case 'q':
      send_to_char(CH, "Design saved!\r\n");
      if (GET_OBJ_VAL(d->edit_obj, 0) == 5) {
        GET_OBJ_VAL(d->edit_obj, 4) = GET_OBJ_VAL(d->edit_obj, 1) * attack_multiplier[GET_OBJ_VAL(d->edit_obj, 2)];
        GET_OBJ_VAL(d->edit_obj, 6) = (GET_OBJ_VAL(d->edit_obj, 1) * GET_OBJ_VAL(d->edit_obj, 1)) * attack_multiplier[GET_OBJ_VAL(d->edit_obj, 2)];
      } else if (GET_OBJ_VAL(d->edit_obj, 0) == SOFT_RESPONSE) {
        GET_OBJ_VAL(d->edit_obj, 4) = GET_OBJ_VAL(d->edit_obj, 1) * ((GET_OBJ_VAL(d->edit_obj, 1) / 3) * (GET_OBJ_VAL(d->edit_obj, 1) / 3));
        GET_OBJ_VAL(d->edit_obj, 6) = GET_OBJ_VAL(d->edit_obj, 1) * GET_OBJ_VAL(d->edit_obj, 1) * ((GET_OBJ_VAL(d->edit_obj, 1) / 3) * (GET_OBJ_VAL(d->edit_obj, 1) / 3));
      } else {
        GET_OBJ_VAL(d->edit_obj, 4) = GET_OBJ_VAL(d->edit_obj, 1) * programs[GET_OBJ_VAL(d->edit_obj, 0)].multiplier;
        GET_OBJ_VAL(d->edit_obj, 6) = (GET_OBJ_VAL(d->edit_obj, 1) * GET_OBJ_VAL(d->edit_obj, 1)) * programs[GET_OBJ_VAL(d->edit_obj, 0)].multiplier;
      }
      GET_OBJ_VAL(d->edit_obj, 4)  *= 20;
      GET_OBJ_TIMER(d->edit_obj) = GET_OBJ_VAL(d->edit_obj, 4);
      GET_OBJ_VAL(d->edit_obj, 9) = GET_IDNUM(CH);
      obj_to_char(d->edit_obj, CH);
      STATE(d) = CON_PLAYING;
      d->edit_obj = NULL;
      break;
    default:
      send_to_char(CH, "Invalid option!\r\n");
      break;
    }
    break;
  case PEDIT_RATING:
    if (GET_OBJ_VAL(d->edit_obj, 0) <= SOFT_SENSOR && number > GET_SKILL(CH, SKILL_COMPUTER) * 1.5)
      send_to_char(CH, "You can't create a persona program of a higher rating than your computer skill times one and a half.\r\n"
                   "Enter Rating: ");
    else if (GET_OBJ_VAL(d->edit_obj, 0) == SOFT_EVALUATE && number > GET_SKILL(CH, SKILL_DATA_BROKERAGE))
      send_to_char(CH, "You can't create an evaluate program of a higher rating than your data brokerage skill.\r\n"
                   "Enter Rating: ");
    else if (GET_OBJ_VAL(d->edit_obj, 0) > SOFT_SENSOR && number > GET_SKILL(CH, SKILL_COMPUTER))
      send_to_char(CH, "You can't create a program of a higher rating than your computer skill.\r\n"
                   "Enter Rating: ");
    else if (GET_OBJ_VAL(d->edit_obj, 0) == SOFT_RESPONSE && number > 3)
      send_to_char("You can't create a response increase of a rating higher than 3.\r\nEnter Rating: ", CH);
    else {
      GET_OBJ_VAL(d->edit_obj, 1) = number;
      pedit_disp_menu(d);
    }
    break;
  case PEDIT_NAME:
    if (strlen(arg) >= LINE_LENGTH) {
      pedit_disp_menu(d);
      return;
    }
    delete [] d->edit_obj->restring;
    d->edit_obj->restring = str_dup(arg);
    pedit_disp_menu(d);
    break;
  case PEDIT_WOUND:
    if (number < 1 || number > 4)
      send_to_char(CH, "Not a valid option!\r\nEnter your choice: ");
    else {
      GET_OBJ_VAL(d->edit_obj, 2) = number;
      pedit_disp_menu(d);
    }
    break;
  case PEDIT_TYPE:
    if (number < 1 || number >= NUM_PROGRAMS)
      send_to_char(CH, "Not a valid option!\r\nEnter your choice: ");
    else {
      GET_OBJ_VAL(d->edit_obj, 0) = number;
      GET_OBJ_VAL(d->edit_obj, 1) = 0;
      pedit_disp_menu(d);
    }
    break;
  }
}

void create_program(struct char_data *ch)
{
  struct obj_data *design = read_object(107, VIRTUAL);
  STATE(ch->desc) = CON_PRO_CREATE;
  design->restring = str_dup("A blank program");
  ch->desc->edit_obj = design;
  pedit_disp_menu(ch->desc);
}


ACMD(do_design)
{
  struct obj_data *comp, *prog;
  if (!*argument) {
    if (AFF_FLAGGED(ch, AFF_PART_DESIGN)) {
      send_to_char(ch, "You stop working on %s.\r\n", GET_OBJ_NAME(GET_BUILDING(ch)));
      STOP_WORKING(ch);
    }
    if (AFF_FLAGGED(ch, AFF_DESIGN)) {
      send_to_char(ch, "You stop working on %s.\r\n", GET_OBJ_NAME(GET_BUILDING(ch)));
      STOP_WORKING(ch);
    } else
      send_to_char(ch, "Design what?\r\n");
    return;
  }
  if (GET_POS(ch) > POS_SITTING) {
    send_to_char(ch, "You have to be sitting to do that.\r\n");
    return;
  }
  if (IS_WORKING(ch)) {
    send_to_char(ch, "You are already working on something.\r\n");
    return;
  }
  skip_spaces(&argument);
  prog = get_obj_in_list_vis(ch, argument, ch->carrying);
  if (prog && GET_OBJ_TYPE(prog) == ITEM_PART) {
    part_design(ch, prog);
    return;
  }
  for (comp = world[ch->in_room].contents; comp; comp = comp->next_content)
    if (GET_OBJ_TYPE(comp) == ITEM_DECK_ACCESSORY && GET_OBJ_VAL(comp, 0) == 2)
      break;
  if (!comp) {
    send_to_char(ch, "There is no computer here for you to make a program design on.\r\n");
    return;
  }
  for (struct char_data *vict = world[ch->in_room].people; vict; vict = vict->next_in_room)
    if (AFF_FLAGS(vict).AreAnySet(AFF_PROGRAM, AFF_DESIGN, ENDBIT)) {
      send_to_char(ch, "Someone is already using the computer.\r\n");
      return;
    }
  for (prog = comp->contains; prog; prog = prog->next_content)
    if ((isname(argument, prog->text.keywords) || isname(argument, prog->restring)) && GET_OBJ_TYPE(prog) == ITEM_DESIGN)
      break;
  if (!prog) {
    send_to_char(ch, "The program design isn't on that computer.\r\n");
    return;
  }
  if (GET_OBJ_VAL(prog, 3) || GET_OBJ_VAL(prog, 5)) {
    send_to_char(ch, "You can no longer make a design on that project.\r\n");
    return;
  }
  if (GET_OBJ_VAL(prog, 9) && GET_OBJ_VAL(prog, 9) != GET_IDNUM(ch)) {
    send_to_char(ch, "Someone else has already started on this program.\r\n");
    return;
  }
  int skill = 0, target = 0;
  switch (GET_OBJ_VAL(prog,0)) {
  case SOFT_BOD:
  case SOFT_EVASION:
  case SOFT_MASKING:
  case SOFT_SENSOR:
  case SOFT_ASIST_COLD:
  case SOFT_ASIST_HOT:
  case SOFT_HARDENING:
  case SOFT_ICCM:
  case SOFT_ICON:
  case SOFT_MPCP:
  case SOFT_REALITY:
  case SOFT_RESPONSE:
    skill = get_skill(ch, SKILL_PROGRAM_CYBERTERM, target);
    break;
  case SOFT_ATTACK:
  case SOFT_SLOW:
    skill = get_skill(ch, SKILL_PROGRAM_COMBAT, target);
    break;
  case SOFT_CLOAK:
  case SOFT_LOCKON:
  case SOFT_MEDIC:
  case SOFT_ARMOUR:
    skill = get_skill(ch, SKILL_PROGRAM_DEFENSIVE, target);
    break;
  case SOFT_BATTLETEC:
  case SOFT_COMPRESSOR:
  case SOFT_SLEAZE:
  case SOFT_TRACK:
  case SOFT_SUITE:
    skill = get_skill(ch, SKILL_PROGRAM_SPECIAL, target);
    break;
  case SOFT_CAMO:
  case SOFT_CRASH:
  case SOFT_DEFUSE:
  case SOFT_EVALUATE:
  case SOFT_VALIDATE:
  case SOFT_SWERVE:
  case SOFT_SNOOPER:
  case SOFT_ANALYZE:
  case SOFT_DECRYPT:
  case SOFT_DECEPTION:
  case SOFT_RELOCATE:
  case SOFT_SCANNER:
  case SOFT_BROWSE:
  case SOFT_READ:
  case SOFT_COMMLINK:
    skill = get_skill(ch, SKILL_PROGRAM_OPERATIONAL, target);
    break;
  }
  if (!skill) {
    send_to_char(ch, "You have no idea how to go about creating a program design for that.\r\n");
    return;
  }
  if (GET_OBJ_TIMER(prog) == GET_OBJ_VAL(prog, 4)) {
    send_to_char(ch, "You begin designing %s.\r\n", prog->restring);
    GET_OBJ_VAL(prog, 8) = skill;
  } else
    send_to_char(ch, "You continue to design %s.\r\n", prog->restring);
  AFF_FLAGS(ch).SetBit(AFF_DESIGN);
  GET_BUILDING(ch) = prog;
}

ACMD(do_program)
{
  struct obj_data *comp, *prog;
  if (!*argument) {
    if (AFF_FLAGGED(ch, AFF_PROGRAM)) {
      AFF_FLAGS(ch).RemoveBit(AFF_PROGRAM);
      send_to_char(ch, "You stop working on %s.\r\n", ch->char_specials.programming->restring);
      ch->char_specials.programming = NULL;
    } else
      send_to_char(ch, "Program What?\r\n");
    return;
  }
  if (GET_POS(ch) > POS_SITTING) {
    send_to_char(ch, "You have to be sitting to do that.\r\n");
    return;
  }
  if (IS_WORKING(ch)) {
    send_to_char(ch, "You are already working on something.\r\n");
    return;
  }
  skip_spaces(&argument);
  for (struct char_data *vict = world[ch->in_room].people; vict; vict = vict->next_in_room)
    if (AFF_FLAGS(vict).AreAnySet(AFF_PROGRAM, AFF_DESIGN, ENDBIT)) {
      send_to_char(ch, "Someone is already using the computer.\r\n");
      return;
    }
  for (comp = world[ch->in_room].contents; comp; comp = comp->next_content)
    if (GET_OBJ_TYPE(comp) == ITEM_DECK_ACCESSORY && GET_OBJ_VAL(comp, 0) == 2)
      break;
  if (!comp) {
    send_to_char(ch, "There is no computer here for you to program on.\r\n");
    return;
  }
  for (prog = comp->contains; prog; prog = prog->next_content)
    if ((isname(argument, prog->text.keywords) || isname(argument, prog->restring)) && GET_OBJ_TYPE(prog) == ITEM_DESIGN)
      break;
  if (!prog) {
    send_to_char(ch, "The program design isn't on that computer.\r\n");
    return;
  }
  if (GET_OBJ_VAL(prog, 9) && GET_OBJ_VAL(prog, 9) != GET_IDNUM(ch)) {
    send_to_char(ch, "Someone else has already started on this program.\r\n");
    return;
  }
  if (!GET_OBJ_VAL(prog, 5)) {
    send_to_char(ch, "You begin to program %s.\r\n", prog->restring);
    int target = GET_OBJ_VAL(prog, 1);
    if (GET_OBJ_VAL(comp, 1) >= GET_OBJ_VAL(prog, 6) * 2)
      target -= 2;
    if (GET_OBJ_VAL(prog, 8) != -1)
      target += 2;
    else
      target -= GET_OBJ_VAL(prog, 3);
    int skill = get_skill(ch, SKILL_COMPUTER, target);
    int success = success_test(skill, target);
    for (struct obj_data *suite = comp->contains; suite; suite = suite->next_content)
      if (GET_OBJ_TYPE(suite) == ITEM_PROGRAM && GET_OBJ_VAL(suite, 0) == SOFT_SUITE) {
        skill += MIN(GET_SKILL(ch, SKILL_COMPUTER), GET_OBJ_VAL(suite, 1));
        break;
      }
    success += (int)(success_test(skill, target) / 2);
    if (success) {
      GET_OBJ_VAL(prog, 5) = GET_OBJ_VAL(prog, 6) / success;
      GET_OBJ_VAL(prog, 5) *= 60;
      GET_OBJ_TIMER(prog) = GET_OBJ_VAL(prog, 5);
    } else {
      GET_OBJ_VAL(prog, 5) = number(1, 6) + number(1, 6);
      GET_OBJ_TIMER(prog) = (GET_OBJ_VAL(prog, 6) * 60) / number(1, 3);
      GET_OBJ_VAL(prog, 7) = 1;
    }
  } else
    send_to_char(ch, "You continue to work on %s.\r\n", prog->restring);
  AFF_FLAGS(ch).SetBit(AFF_PROGRAM);
  GET_BUILDING(ch) = prog;
}
#undef CH
#define CH desc->character
#define PROG GET_BUILDING(CH)
void update_programs(void)
{
  struct descriptor_data *next;
  for (struct descriptor_data *desc = descriptor_list; desc; desc = next) {
    next = desc->next;
    if (CH && STATE(desc) == CON_PLAYING)
      if (AFF_FLAGGED(desc->character, AFF_PART_BUILD)) {
        if (--GET_OBJ_VAL(PROG, 4) < 1) {
          if (GET_OBJ_TIMER(PROG) < 0) {
            send_to_char(desc->character, "You realise you have botched installing %s.\r\n", GET_OBJ_NAME(PROG));
            extract_obj(PROG);
          } else {
            send_to_char(desc->character, "You finish installing %s into %s.\r\n", GET_OBJ_NAME(PROG), GET_OBJ_NAME(PROG->contains));
            obj_from_char(PROG);
            obj_to_obj(PROG, PROG->contains);
            PROG->contains = NULL;
            GET_OBJ_VAL(PROG, 4) = -2;
            if (!GET_OBJ_VAL(PROG->in_obj, 0))
              GET_OBJ_VAL(PROG->in_obj, 0) = GET_OBJ_VAL(PROG, 2);
            switch(GET_OBJ_VAL(PROG, 0)) {
            case PART_MPCP:
              GET_OBJ_VAL(PROG->in_obj, 0) = GET_OBJ_VAL(PROG, 2);
              break;
            case PART_RESPONSE:
              GET_OBJ_VAL(PROG->in_obj, 6) = GET_OBJ_VAL(PROG, 1);
              break;
            case PART_IO:
              GET_OBJ_VAL(PROG->in_obj, 4) = GET_OBJ_VAL(PROG, 1);
              break;
            case PART_STORAGE:
              GET_OBJ_VAL(PROG->in_obj, 3) = GET_OBJ_VAL(PROG, 1);
              break;
            case PART_HARDENING:
              GET_OBJ_VAL(PROG->in_obj, 1) = GET_OBJ_VAL(PROG, 1);
              break;
            case PART_ACTIVE:
              GET_OBJ_VAL(PROG->in_obj, 2) = GET_OBJ_VAL(PROG, 1);
              break;
            }
            if (GET_OBJ_VAL(PROG->in_obj, 9)) {
              int x = 0;
              for (struct obj_data *obj = PROG->in_obj->contains; obj; obj = obj->next_content)
                if (GET_OBJ_TYPE(obj) == ITEM_PART)
                  switch (GET_OBJ_VAL(obj, 0)) {
                  case PART_MPCP:
                  case PART_ASIST_HOT:
                  case PART_ASIST_COLD:
                  case PART_ACTIVE:
                  case PART_BOD:
                  case PART_SENSOR:
                  case PART_IO:
                  case PART_MATRIX_INTERFACE:
                    x++;
                  }
              if (x == 7)
                GET_OBJ_VAL(PROG->in_obj, 9) = 0;
            }
          }
          STOP_WORKING(desc->character);
        }
      } else if (AFF_FLAGGED(desc->character, AFF_PART_DESIGN)) {
        if (--GET_OBJ_VAL(PROG, 3) < 1) {
          send_to_char(desc->character, "You complete the design plan for %s.\r\n", GET_OBJ_NAME(PROG));
          AFF_FLAGS(desc->character).RemoveBit(AFF_PART_DESIGN);
          PROG = NULL;
        }
      } else if (AFF_FLAGGED(desc->character, AFF_DESIGN)) {
        if (--GET_OBJ_VAL(PROG, 4) < 1) {
          send_to_char(desc->character, "You complete the design plan for %s.\r\n", GET_OBJ_NAME(PROG));
          int target = 4;
          if (GET_OBJ_VAL(PROG, 1) < 5)
            target--;
          else if (GET_OBJ_VAL(PROG, 1) > 9)
            target++;
          GET_OBJ_VAL(PROG, 3) = success_test(GET_OBJ_VAL(PROG, 8), target);
          GET_OBJ_VAL(PROG, 8) = -1;
          PROG = NULL;
          AFF_FLAGS(desc->character).RemoveBit(AFF_DESIGN);
        }
        if (ROOM_FLAGGED(desc->character->in_room, ROOM_HOUSE))
          ROOM_FLAGS(desc->character->in_room).SetBit(ROOM_HOUSE_CRASH);
      } else if (AFF_FLAGGED(desc->character, AFF_PROGRAM)) {
        if (--GET_OBJ_VAL(PROG, 5) < 1) {
          if (GET_OBJ_VAL(PROG, 7))
            send_to_char(desc->character, "You realise programming %s is a lost cause.\r\n", GET_OBJ_NAME(PROG));
          else {
            send_to_char(desc->character, "You complete programming %s.\r\n", GET_OBJ_NAME(PROG));
            struct obj_data *newp = read_object(108, VIRTUAL);
            newp->restring = str_dup(GET_OBJ_NAME(PROG));
            GET_OBJ_VAL(newp, 0) = GET_OBJ_VAL(PROG, 0);
            GET_OBJ_VAL(newp, 1) = GET_OBJ_VAL(PROG, 1);
            GET_OBJ_VAL(newp, 2) = GET_OBJ_VAL(PROG, 6);
            GET_OBJ_VAL(newp, 3) = GET_OBJ_VAL(PROG, 2);
            obj_to_char(newp, desc->character);
          }
          GET_OBJ_VAL(PROG->in_obj, 3) -= GET_OBJ_VAL(PROG, 6) + (GET_OBJ_VAL(PROG, 6) / 10);
          extract_obj(PROG);
          PROG = NULL;
          AFF_FLAGS(desc->character).RemoveBit(AFF_PROGRAM);
        }
        if (ROOM_FLAGGED(desc->character->in_room, ROOM_HOUSE))
          ROOM_FLAGS(desc->character->in_room).SetBit(ROOM_HOUSE_CRASH);
      } else if (AFF_FLAGGED(CH, AFF_BONDING) && --GET_OBJ_VAL(PROG, 9) < 1) {
        send_to_char(CH, "You complete the bonding ritual for %s.\r\n", GET_OBJ_NAME(PROG));
        STOP_WORKING(CH);
      } else if (AFF_FLAGGED(CH, AFF_CONJURE) && --CH->char_specials.conjure[2] < 1) {
        if ((GET_OBJ_COST(PROG) -= CH->char_specials.conjure[1] * 1000) <= 0)
          extract_obj(PROG);
        STOP_WORKING(CH);
        int skill = GET_SKILL(CH, SKILL_CONJURING);
        for (int i = 0; i < NUM_WEARS; i++)
          if (GET_EQ(CH, i) && GET_OBJ_TYPE(GET_EQ(CH, i)) == ITEM_FOCUS && GET_OBJ_VAL(GET_EQ(CH, i), 0) == FOCI_SPIRIT
              && GET_OBJ_VAL(GET_EQ(CH, i), 2) == GET_IDNUM(CH) && GET_OBJ_VAL(GET_EQ(CH, i), 3) == CH->char_specials.conjure[0]
              && GET_OBJ_VAL(GET_EQ(CH, i), 4)) {
            skill += GET_OBJ_VAL(GET_EQ(CH, i), 1);
            break;
          }
        int success = success_test(skill, CH->char_specials.conjure[1]);
        if (success < 1) {
          send_to_char(CH, "You fail to conjure the %s elemental.\r\n", elements[CH->char_specials.conjure[0]].name);
          continue;
        }
        int idnum = number(0, 1000);
        send_to_char(CH, "You summon a %s elemental.\r\n", elements[CH->char_specials.conjure[0]].name);
        struct char_data *mob = create_elemental(CH, CH->char_specials.conjure[0], CH->char_specials.conjure[1], idnum, TRAD_HERMETIC);
        act("$n appears just outside of the hermetic circle.", FALSE, mob, 0, 0, TO_ROOM);
        if (conjuring_drain(CH, CH->char_specials.conjure[1]) || !AWAKE(CH)) {
          if (success_test(GET_LEVEL(mob), 6) > 0)
            extract_char(mob);
          else {
            GET_ACTIVE(mob) = 0;
            MOB_FLAGS(mob).SetBit(MOB_AGGRESSIVE);
            stop_follower(mob);
          }
          continue;
        }
        struct spirit_data *spirit = new spirit_data;
        spirit->type = CH->char_specials.conjure[0];
        spirit->force = CH->char_specials.conjure[1];
        spirit->services = success;
        spirit->id = idnum;
        spirit->next = GET_SPIRIT(CH);
        spirit->called = TRUE;
        GET_SPIRIT(CH) = spirit;
        GET_NUM_SPIRITS(CH)++;
        GET_SPARE2(mob) = spirit->id;
      } else if (AFF_FLAGGED(CH, AFF_CIRCLE) && --GET_OBJ_VAL(PROG, 9) < 1) {
        send_to_char("You complete drawing the circle.\r\n", CH);
        act("$n finishes drawing the heremetic circle.\r\n", FALSE, CH, 0, 0, TO_ROOM);
        STOP_WORKING(CH);
      } else if (AFF_FLAGGED(CH, AFF_LODGE) && --GET_OBJ_VAL(PROG, 9) < 1) {
        send_to_char("You finish building the lodge.\r\n", CH);
        act("$n finishes building the lodge.\r\n", FALSE, CH, 0, 0, TO_ROOM);
        STOP_WORKING(CH);
      }
  }

}
