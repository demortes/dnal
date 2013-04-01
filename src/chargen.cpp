#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include "structs.h"
#include "db.h"
#include "dblist.h"
#include "newdb.h"
#include "utils.h"
#include "interpreter.h"
#include "comm.h"
#include "handler.h"
#include "newmagic.h"
#include "awake.h"
#include "constants.h"

#define CH d->character

extern class helpList Help;

const char *pc_race_types[] =
  {
    "Undef",
    "Undefined",
    "Human",
    "Dwarf",
    "Elf",
    "Ork",
    "Troll",
    "Cyclops",
    "Koborokuru",
    "Fomori",
    "Menehune",
    "Hobgoblin",
    "Giant",
    "Gnome",
    "Oni",
    "Wakyambi",
    "Ogre",
    "Minotaur",
    "Satyr",
    "Night-One",
    "Dryad",
    "Dragon",
    "\n"
  };

const char *race_menu =
  "\r\nSelect a race:\r\n"
  "  [1] Human\r\n"
  "  [2] Dwarf\r\n"
  "  [3] Elf\r\n"
  "  [4] Ork\r\n"
  "  [5] Troll\r\n"
  "  [6] Cyclops\r\n"
  "  [7] Koborokuru\r\n"
  "  [8] Fomori\r\n"
  "  [9] Menehune\r\n"
  "  [A] Hobgoblin\r\n"
  "  [B] Giant\r\n"
  "  [C] Gnome\r\n"
  "  [D] Oni\r\n"
  "  [E] Wakyambi\r\n"
  "  [F] Ogre\r\n"
  "  [G] Minotaur\r\n"
  "  [H] Satyr\r\n"
  "  [I] Night-One\r\n"
  "  ?# (for help on a particular race)\r\n";

const char *assign_menu =
  "\r\nSelect a priority to assign:\r\n"
  "  [1] Attributes\r\n"
  "  [2] Magic\r\n"
  "  [3] Resources\r\n"
  "  [4] Skills\r\n";

int attrib_vals[5] = { 30, 27, 24, 21, 18 };
int skill_vals[5] = { 50, 40, 34, 30, 27 };
int nuyen_vals[5] = { 1000000, 400000, 90000, 20000, 5000 };
int force_vals[5] = { 25, 25, 25, 25, 25 };

void set_attributes(struct char_data *ch, int magic)
{
  GET_REAL_BOD(ch) = 1;
  GET_REAL_QUI(ch) = 1;
  GET_REAL_STR(ch) = 1;
  GET_REAL_CHA(ch) = 1;
  GET_REAL_INT(ch) = 1;
  GET_REAL_WIL(ch) = 1;
  ch->real_abils.bod_index = 100;
  GET_REAL_REA(ch) = 1;

  if (magic)
  {
    ch->real_abils.mag = 600;
    ch->real_abils.rmag = 600;
  } else
    ch->real_abils.mag = 0;

  switch (GET_RACE(ch))
  {
  case RACE_DWARF:
    GET_REAL_BOD(ch)++;
    GET_REAL_STR(ch) += 2;
    GET_REAL_WIL(ch)++;
    ch->real_abils.bod_index += 100;
    GET_ATT_POINTS(ch) -= 6;
    send_to_char("You spend 6 attribute points to raise your attributes to their minimums.\r\n", ch);
    break;
  case RACE_ELF:
    GET_REAL_QUI(ch)++;
    GET_REAL_CHA(ch) += 2;
    GET_ATT_POINTS(ch) -= 6;
    send_to_char("You spend 6 attribute points to raise your attributes to their minimums.\r\n", ch);
    break;
  case RACE_ORK:
    GET_REAL_BOD(ch) += 3;
    GET_REAL_STR(ch) += 2;
    ch->real_abils.bod_index += 300;
    GET_ATT_POINTS(ch) -= 8;
    send_to_char("You spend 8 attribute points to raise your attributes to their minimums.\r\n", ch);
    break;
  case RACE_TROLL:
    GET_REAL_BOD(ch) += 5;
    GET_REAL_STR(ch) += 4;
    ch->real_abils.bod_index += 500;
    GET_ATT_POINTS(ch) -= 11;
    send_to_char("You spend 11 attribute points to raise your attributes to their minimums.\r\n", ch);
    break;
  case RACE_CYCLOPS:
    GET_REAL_BOD(ch) += 5;
    GET_REAL_STR(ch) += 6;
    ch->real_abils.bod_index += 500;
    GET_ATT_POINTS(ch) -= 11;
    send_to_char("You spend 11 attribute points to raise your attributes to their mininums.\r\n", ch);
    break;
  case RACE_KOBOROKURU:
    GET_REAL_BOD(ch)++;
    GET_REAL_STR(ch) += 2;
    GET_REAL_WIL(ch)++;
    ch->real_abils.bod_index += 100;
    GET_ATT_POINTS(ch) -= 6;
    send_to_char("You spend 6 attribute points to raise your attributes to their minimums.\r\n", ch);
    break;
  case RACE_FOMORI:
    GET_REAL_BOD(ch) += 4;
    GET_REAL_STR(ch) += 3;
    ch->real_abils.bod_index += 400;
    GET_ATT_POINTS(ch) -= 9;
    send_to_char("You spend 9 attribute points to raise your attributes to their minimums.\r\n", ch);
    break;
  case RACE_MENEHUNE:
    GET_REAL_BOD(ch) += 2;
    GET_REAL_STR(ch)++;
    GET_REAL_WIL(ch)++;
    ch->real_abils.bod_index += 200;
    GET_ATT_POINTS(ch) -= 6;
    send_to_char("You spend 6 attribute points to raise your attributes to their minimums.\r\n", ch);
    break;
  case RACE_HOBGOBLIN:
    GET_REAL_BOD(ch) += 2;
    GET_REAL_STR(ch) += 2;
    ch->real_abils.bod_index += 200;
    GET_ATT_POINTS(ch) -= 7;
    send_to_char("You spend 7 attribute points to raise your attributes to their minimums.\r\n", ch);
    break;
  case RACE_GIANT:
    GET_REAL_BOD(ch) += 5;
    GET_REAL_STR(ch) += 5;
    ch->real_abils.bod_index += 500;
    GET_ATT_POINTS(ch) -= 11;
    send_to_char("You spend 11 attribute points to raise your attributes to their minimums.\r\n", ch);
    break;
  case RACE_GNOME:
    GET_REAL_BOD(ch)++;
    GET_REAL_STR(ch)++;
    GET_REAL_WIL(ch) += 2;
    ch->real_abils.bod_index += 100;
    GET_ATT_POINTS(ch) -= 6;
    send_to_char("You spend 6 attribute points to raise your attributes to their minimums.\r\n", ch);
    break;
  case RACE_ONI:
    GET_REAL_BOD(ch) += 2;
    GET_REAL_STR(ch) += 2;
    GET_REAL_WIL(ch)++;
    ch->real_abils.bod_index += 200;
    GET_ATT_POINTS(ch) -= 8;
    send_to_char("You spend 8 attribute points to raise your attributes to their minimums.\r\n", ch);
    break;
  case RACE_WAKYAMBI:
    GET_REAL_CHA(ch) += 2;
    GET_REAL_WIL(ch)++;
    GET_ATT_POINTS(ch) -= 6;
    send_to_char("You spend 6 attribute points to raise your attributes to their minimums.\r\n", ch);
    break;
  case RACE_OGRE:
    GET_REAL_BOD(ch) += 3;
    GET_REAL_STR(ch) += 2;
    ch->real_abils.bod_index += 300;
    GET_ATT_POINTS(ch) -= 7;
    send_to_char("You spend 7 attribute points to raise your attributes to their minimums.\r\n", ch);
    break;
  case RACE_MINOTAUR:
    GET_REAL_BOD(ch) += 4;
    GET_REAL_STR(ch) += 3;
    ch->real_abils.bod_index += 400;
    GET_ATT_POINTS(ch) -= 9;
    send_to_char("You spend 9 attribute points to raise your attributes to their minimums.\r\n", ch);
    break;
  case RACE_SATYR:
    GET_REAL_BOD(ch) += 3;
    GET_REAL_STR(ch) += 2;
    GET_REAL_WIL(ch)++;
    ch->real_abils.bod_index += 300;
    GET_ATT_POINTS(ch) -= 9;
    send_to_char("You spend 9 attribute points to raise your attributes to their minimums.\r\n", ch);
    break;
  case RACE_NIGHTONE:
    GET_REAL_QUI(ch) += 2;
    GET_REAL_CHA(ch) += 2;
    GET_ATT_POINTS(ch) -= 6;
    send_to_char("You spend 6 attribute points to raise your attributes to their minimums.\r\n", ch);
    break;
  case RACE_DRYAD:
    GET_REAL_QUI(ch)++;
    GET_REAL_CHA(ch) += 3;
    GET_ATT_POINTS(ch) -= 10;
    send_to_char("You spend 10 attribute points to raise your attributes to their minimums.\r\n", ch);
    break;
  case RACE_HUMAN:
    GET_ATT_POINTS(ch) -= 6;
    send_to_char("You spend 6 attribute points to raise your attributes to their minimums.\r\n", ch);
    break;
  }

  ch->aff_abils = ch->real_abils;
}

void init_create_vars(struct descriptor_data *d)
{
  int i;

  d->ccr.mode = CCR_SEX;
  for (i = 0; i < 5; i++)
    d->ccr.pr[i] = PR_NONE;
  d->ccr.force_points = 0;
  d->ccr.temp = 0;
}

int parse_race(struct descriptor_data *d, char *arg)
{
  switch (*arg)
  {
  case '1':
    return RACE_HUMAN;
  case '2':
    return RACE_DWARF;
  case '3':
    return RACE_ELF;
  case '4':
    return RACE_ORK;
  case '5':
    return RACE_TROLL;
  case '6':
    return RACE_CYCLOPS;
  case '7':
    return RACE_KOBOROKURU;
  case '8':
    return RACE_FOMORI;
  case '9':
    return RACE_MENEHUNE;
  case 'a':
    return RACE_HOBGOBLIN;
  case 'b':
    return RACE_GIANT;
  case 'c':
    return RACE_GNOME;
  case 'd':
    return RACE_ONI;
  case 'e':
    return RACE_WAKYAMBI;
  case 'f':
    return RACE_OGRE;
  case 'g':
    return RACE_MINOTAUR;
  case 'h':
    return RACE_SATYR;
  case 'i':
    return RACE_NIGHTONE;
  case '?':
    switch (*(arg+1)) {
    case '1':
      Help.FindTopic(buf2, "human");
      break;
    case '2':
      Help.FindTopic(buf2, "dwarf");
      break;
    case '3':
      Help.FindTopic(buf2, "elf");
      break;
    case '4':
      Help.FindTopic(buf2, "ork");
      break;
    case '5':
      Help.FindTopic(buf2, "troll");
      break;
    case '6':
      Help.FindTopic(buf2, "cyclops");
      break;
    case '7':
      Help.FindTopic(buf2, "koborokuru");
      break;
    case '8':
      Help.FindTopic(buf2, "fomori");
      break;
    case '9':
      Help.FindTopic(buf2, "menehune");
      break;
    case 'a':
      Help.FindTopic(buf2, "hobgoblin");
      break;
    case 'b':
      Help.FindTopic(buf2, "giant");
      break;
    case 'c':
      Help.FindTopic(buf2, "gnome");
      break;
    case 'd':
      Help.FindTopic(buf2, "oni");
      break;
    case 'e':
      Help.FindTopic(buf2, "wakyambi");
      break;
    case 'f':
      Help.FindTopic(buf2, "ogre");
      break;
    case 'g':
      Help.FindTopic(buf2, "minotaur");
      break;
    case 'h':
      Help.FindTopic(buf2, "satyr");
      break;
    case 'i':
      Help.FindTopic(buf2, "night-one");
      break;
    default:
      return RACE_UNDEFINED;
    }
    strcat(buf2, "\r\nª Press [return] to continue ´");
    SEND_TO_Q(buf2, d);
    d->ccr.temp = CCR_RACE;
    d->ccr.mode = CCR_AWAIT_CR;
    return RETURN_HELP;
  default:
    return RACE_UNDEFINED;
  }
}

int parse_totem(struct descriptor_data *d, char *arg)
{
  char *temp = arg;
  int i;

  if (*temp == '?')
  {
    i = atoi(++temp);
    Help.FindTopic(buf2, (char *)totem_types[i]);
    strcat(buf2, "\r\nª Press [return] to continue ´");
    SEND_TO_Q(buf2, d);
    d->ccr.temp = CCR_TOTEM;
    d->ccr.mode = CCR_AWAIT_CR;
    return RETURN_HELP;
  }
  i = atoi(arg);
  if (i < 1 || i > NUM_TOTEMS)
    i = TOTEM_UNDEFINED;
  return i;
}

int parse_assign(struct descriptor_data *d, char *arg)
{
  int i;

  switch (*arg)
  {
  case '1':
  case '2':
  case '3':
  case '4':
    for (i = 0; i < 5; i++)
      if (d->ccr.pr[i] == (int)(*arg - '0')) {
        switch (d->ccr.pr[i]) {
        case PR_ATTRIB:
          GET_ATT_POINTS(d->character) = 0;
          break;
        case PR_MAGIC:
          break;
        case PR_RESOURCE:
          GET_NUYEN(d->character) = 0;
          d->ccr.force_points = 0;
          break;
        case PR_SKILL:
          GET_SKILL_POINTS(d->character) = 0;
          break;
        }
        d->ccr.pr[i] = PR_NONE;
      }
    d->ccr.pr[d->ccr.temp] = (int)(*arg - '0');
    switch (d->ccr.pr[d->ccr.temp]) {
    case PR_ATTRIB:
      GET_ATT_POINTS(d->character) = attrib_vals[d->ccr.temp];
      break;
    case PR_MAGIC:
      break;
    case PR_RESOURCE:
      GET_NUYEN(d->character) = nuyen_vals[d->ccr.temp];
      d->ccr.force_points = force_vals[d->ccr.temp];
      break;
    case PR_SKILL:
      GET_SKILL_POINTS(d->character) = skill_vals[d->ccr.temp];
      break;
    }
    d->ccr.temp = 0;
    GET_PP(d->character) = 600;
    return 1;
  case 'c':
    d->ccr.pr[d->ccr.temp] = PR_NONE;
    d->ccr.temp = 0;
    return 1;
  }
  return 0;
}

void priority_menu(struct descriptor_data *d)
{
  int i;

  d->ccr.mode = CCR_PRIORITY;
  SEND_TO_Q("\r\nPriority  Setting     Attributes   Skills Resources\r\n", d);

  d->character->real_abils.ess = 600;
  for (i = 0; i < 5; i++)
  {
    sprintf(buf2, "%-10c", 'A' + i);
    switch (d->ccr.pr[i]) {
    case PR_NONE:
      sprintf(buf2, "%s?           %-2d           %-2d        %d ¥ / %d\r\n",
              buf2, attrib_vals[i], skill_vals[i], nuyen_vals[i], force_vals[i]);
      break;
    case PR_RACE:
      if (GET_RACE(d->character) == RACE_ELF)
        strcat(buf2, "Elf         -            -         -\r\n");
      else if (GET_RACE(d->character) == RACE_DRAGON)
        strcat(buf2, "Dragon     -            -        -\r\n");
      else if (GET_RACE(d->character) == RACE_TROLL)
        strcat(buf2, "Troll       -            -         -\r\n");
      else if (GET_RACE(d->character) == RACE_ORK)
        strcat(buf2, "Ork         -            -         -\r\n");
      else if (GET_RACE(d->character) == RACE_DWARF)
        strcat(buf2, "Dwarf       -            -         -\r\n");
      else if (GET_RACE(d->character) == RACE_CYCLOPS)
        strcat(buf2, "Cyclops     -            -         -\r\n");
      else if (GET_RACE(d->character) == RACE_KOBOROKURU)
        strcat(buf2, "Koborokuru  -            -         -\r\n");
      else if (GET_RACE(d->character) == RACE_FOMORI)
        strcat(buf2, "Fomori      -            -         -\r\n");
      else if (GET_RACE(d->character) == RACE_MENEHUNE)
        strcat(buf2, "Menehune    -            -         -\r\n");
      else if (GET_RACE(d->character) == RACE_HOBGOBLIN)
        strcat(buf2, "Hobgoblin   -            -         -\r\n");
      else if (GET_RACE(d->character) == RACE_GIANT)
        strcat(buf2, "Giant       -            -         -\r\n");
      else if (GET_RACE(d->character) == RACE_GNOME)
        strcat(buf2, "Gnome       -            -         -\r\n");
      else if (GET_RACE(d->character) == RACE_ONI)
        strcat(buf2, "Oni         -            -         -\r\n");
      else if (GET_RACE(d->character) == RACE_WAKYAMBI)
        strcat(buf2, "Wakyambi    -            -         -\r\n");
      else if (GET_RACE(d->character) == RACE_OGRE)
        strcat(buf2, "Ogre        -            -         -\r\n");
      else if (GET_RACE(d->character) == RACE_MINOTAUR)
        strcat(buf2, "Minotaur    -            -         -\r\n");
      else if (GET_RACE(d->character) == RACE_SATYR)
        strcat(buf2, "Satyr       -            -         -\r\n");
      else if (GET_RACE(d->character) == RACE_NIGHTONE)
        strcat(buf2, "Night-One   -            -         -\r\n");
      else
        strcat(buf2, "Human       -            -         -\r\n");
      break;
    case PR_MAGIC:
      if ( i == 0 )
        strcat(buf2, "Full Mage   -            -         -\r\n");
      else if ( i == 1 )
        strcat(buf2, "Adept/Aspect-            -         -\r\n");
      else
        strcat(buf2, "Mundane     -            -         -\r\n");
      break;
    case PR_ATTRIB:
      sprintf(buf2, "%sAttributes  %-2d           -         -\r\n", buf2,
              attrib_vals[i]);
      break;
    case PR_SKILL:
      sprintf(buf2, "%sSkills      -            %-2d        -\r\n", buf2,
              skill_vals[i]);
      break;
    case PR_RESOURCE:
      sprintf(buf2, "%sResources   -            -         %d ¥ / %d\r\n",
              buf2, nuyen_vals[i], force_vals[i]);
      break;
    }
    SEND_TO_Q(buf2, d);
  }
  SEND_TO_Q("\r\nChoose a priority (A-D) to set (? for help, p to continue): ", d);
}

static void start_game(descriptor_data *d)
{
  playerDB.CreateChar(d->character);

  d->character->player.host = str_dup(d->host);

  GET_SKILL(d->character, SKILL_ENGLISH) = 10;
  GET_LANGUAGE(d->character) = SKILL_ENGLISH;
  GET_RESTRING_POINTS(d->character) = 5;

  playerDB.SaveChar(d->character);
  if(PLR_FLAGGED(d->character,PLR_AUTH)) {
    sprintf(buf, "%s [%s] new player.",
            GET_CHAR_NAME(d->character), d->host);
    mudlog(buf, d->character, LOG_CONNLOG, TRUE);
    SEND_TO_Q(motd, d);
    SEND_TO_Q("\r\n\n*** PRESS RETURN: ", d);
    STATE(d) = CON_RMOTD;

    init_create_vars(d);
  } else {
    SEND_TO_Q(motd, d);
    SEND_TO_Q("\r\n\n*** PRESS RETURN: ", d);
    STATE(d) = CON_RMOTD;

    init_create_vars(d);

    sprintf(buf, "%s [%s] new player.",
            GET_CHAR_NAME(d->character), d->host);
    mudlog(buf, d->character, LOG_CONNLOG, TRUE);
  }
}

void ccr_totem_menu(struct descriptor_data *d)
{
  sprintf(buf, "Select your totem: \r\n");
  int x = 1;
  for (int i = 1; i < NUM_TOTEMS; i++, x++)
    if (!(GET_ASPECT(CH) == ASPECT_SHAMANIST && (i == TOTEM_OWL || i == TOTEM_COYOTE)))
      sprintf(ENDOF(buf), "  [%d] %s\r\n", x, totem_types[i]);
    else
      x--;
  SEND_TO_Q(buf, d);
  SEND_TO_Q("\r\nTotem: ", d);
  d->ccr.mode = CCR_TOTEM;
}

void ccr_totem_spirit_menu(struct descriptor_data *d)
{
  sprintf(buf, "Select your totem spirit bonus: \r\n");
  switch (GET_TOTEM(CH))
  {
  case TOTEM_GATOR:
    strcat(buf, "  [1] Swamp\r\n  [2] Lake\r\n  [3] River\r\n  [4] City\r\n");
    break;
  case TOTEM_SNAKE:
    strcat(buf, "  [1] City\r\n  [2] Field\r\n  [3] Hearth\r\n  [4] Desert\r\n  [5] Forest\r\n  [6] Mountain\r\n  [7] Prairie\r\n");
    break;
  case TOTEM_WOLF:
    strcat(buf, "  [1] Forest\r\n  [2] Prairie\r\n  [3] Mountain\r\n");
    break;
  }

  SEND_TO_Q(buf, d);
  SEND_TO_Q("\r\nTotem Spirit Bonus: ", d);
  d->ccr.mode = CCR_TOTEM2;
}

void ccr_aspect_menu(struct descriptor_data *d)
{
  sprintf(buf, "Select your aspect: \r\n");
  sprintf(ENDOF(buf), "  [1] Conjurer\r\n"
          "  [2] Sorcerer\r\n");
  if (GET_TRADITION(d->character) == TRAD_SHAMANIC)
    sprintf(ENDOF(buf), "  [3] Shamanist\r\n");
  else
    sprintf(ENDOF(buf), "  [3] Elementalist (Earth)\r\n"
            "  [4] Elementalist (Air)\r\n"
            "  [5] Elementalist (Fire)\r\n"
            "  [6] Elementalist (water)\r\n");
  sprintf(ENDOF(buf), "\r\nAspect: ");
  SEND_TO_Q(buf, d);
  d->ccr.mode = CCR_ASPECT;
}

void create_parse(struct descriptor_data *d, char *arg)
{
  int i, ok;
  long shirt = 0, pants = 0;

  switch (d->ccr.mode)
  {
  case CCR_AWAIT_CR:
    d->ccr.mode = d->ccr.temp;
    d->ccr.temp = 0;
    switch (d->ccr.mode) {
    case CCR_RACE:
      SEND_TO_Q(race_menu, d);
      SEND_TO_Q("\r\nRace: ", d);
      break;
    case CCR_PRIORITY:
      priority_menu(d);
      break;
    default:
      log("Invalid mode in AWAIT_CR in chargen.");
    }
    break;
  case CCR_SEX:
    switch (*arg) {
    case 'm':
    case 'M':
      d->character->player.sex = SEX_MALE;
      break;
    case 'f':
    case 'F':
      d->character->player.sex = SEX_FEMALE;
      break;
    default:
      SEND_TO_Q("That is not a sex..\r\nWhat IS your sex? ", d);
      return;
    }
    SEND_TO_Q(race_menu, d);
    SEND_TO_Q("\r\nRace: ", d);
    d->ccr.mode = CCR_RACE;
    break;
  case CCR_RACE:
    if ((GET_RACE(d->character) = parse_race(d, arg)) == RACE_UNDEFINED) {
      SEND_TO_Q("\r\nThat's not a race.\r\nRace: ", d);
      return;
    } else if (GET_RACE(d->character) == RETURN_HELP) // for when they use help
      return;

    if (GET_RACE(d->character) == RACE_DRAGON)
      d->ccr.pr[0] = PR_RACE;
    else if (GET_RACE(d->character) == RACE_HUMAN)
      d->ccr.pr[4] = PR_RACE;
    else if (GET_RACE(d->character) == RACE_DWARF)
      d->ccr.pr[3] = PR_RACE;
    else if (GET_RACE(d->character) == RACE_ORK)
      d->ccr.pr[3] = PR_RACE;
    else if (GET_RACE(d->character) == RACE_TROLL)
      d->ccr.pr[2] = PR_RACE;
    else if (GET_RACE(d->character) == RACE_ELF)
      d->ccr.pr[2] = PR_RACE;
    else if (GET_RACE(d->character) == RACE_CYCLOPS)
      d->ccr.pr[2] = PR_RACE;
    else if (GET_RACE(d->character) == RACE_KOBOROKURU)
      d->ccr.pr[2] = PR_RACE;
    else if (GET_RACE(d->character) == RACE_FOMORI)
      d->ccr.pr[2] = PR_RACE;
    else if (GET_RACE(d->character) == RACE_MENEHUNE)
      d->ccr.pr[2] = PR_RACE;
    else if (GET_RACE(d->character) == RACE_HOBGOBLIN)
      d->ccr.pr[2] = PR_RACE;
    else if (GET_RACE(d->character) == RACE_GIANT)
      d->ccr.pr[2] = PR_RACE;
    else if (GET_RACE(d->character) == RACE_GNOME)
      d->ccr.pr[2] = PR_RACE;
    else if (GET_RACE(d->character) == RACE_ONI)
      d->ccr.pr[2] = PR_RACE;
    else if (GET_RACE(d->character) == RACE_WAKYAMBI)
      d->ccr.pr[2] = PR_RACE;
    else if (GET_RACE(d->character) == RACE_OGRE)
      d->ccr.pr[2] = PR_RACE;
    else if (GET_RACE(d->character) == RACE_MINOTAUR)
      d->ccr.pr[2] = PR_RACE;
    else if (GET_RACE(d->character) == RACE_SATYR)
      d->ccr.pr[2] = PR_RACE;
    else if (GET_RACE(d->character) == RACE_NIGHTONE)
      d->ccr.pr[2] = PR_RACE;
    else if (GET_RACE(d->character) == RACE_DRYAD)
      d->ccr.pr[1] = PR_RACE;
    if (real_object(2041) > -1)
      obj_to_char(read_object(2041, VIRTUAL), d->character);
    switch (number(0 ,4)) {
    case 0:
      shirt = 60563;
      break;
    case 1:
      shirt = 16218;
      break;
    case 2:
      shirt = 1024;
      break;
    case 3:
      shirt = 28462;
      break;
    case 4:
      shirt = 22680;
      break;
    }
    switch (number(0 ,4)) {
    case 0:
      pants = 60564;
      break;
    case 1:
      pants = 39828;
      break;
    case 2:
      pants = 17594;
      break;
    case 3:
      pants = 38009;
      break;
    case 4:
      pants = 22683;
      break;
    }

    GET_EQ(d->character, WEAR_BODY) = read_object(shirt, VIRTUAL);
    GET_EQ(d->character, WEAR_LEGS) = read_object(pants, VIRTUAL);
    priority_menu(d);
    d->ccr.mode = CCR_PRIORITY;
    break;
  case CCR_TOTEM:
    if ((i = parse_totem(d, arg)) == TOTEM_UNDEFINED) {
      SEND_TO_Q("\r\nThat's not a totem.\r\nTotem: ", d);
      return;
    } else if (i == RETURN_HELP) // for when they use help
      return;
    if (GET_ASPECT(CH) == ASPECT_SHAMANIST) {
      if (i > TOTEM_OWL)
        i += 2;
      else if (i > TOTEM_COYOTE)
        i++;
    }
    if (i >= NUM_TOTEMS) {
      SEND_TO_Q("\r\nThat's not a totem.\r\nTotem: ", d);
      return;
    }
    GET_TOTEM(d->character) = (ubyte) i;
    if (GET_TOTEM(CH) == TOTEM_WOLF || GET_TOTEM(CH) == TOTEM_SNAKE || GET_TOTEM(CH) == TOTEM_GATOR)
      ccr_totem_spirit_menu(d);
    else
      start_game(d);
    break;
  case CCR_TOTEM2:
    i = atoi(arg);
    switch (GET_TOTEM(CH)) {
    case TOTEM_GATOR:
      if (i < 1 || i > 4) {
        SEND_TO_Q("Invalid spirit bonus:\r\nTotem Spirit Bonus: ", d);
        return;
      }
      switch (i) {
      case 1:
        GET_TOTEMSPIRIT(CH) = SPIRIT_SWAMP;
        break;
      case 2:
        GET_TOTEMSPIRIT(CH) = SPIRIT_LAKE;
        break;
      case 3:
        GET_TOTEMSPIRIT(CH) = SPIRIT_RIVER;
        break;
      case 4:
        GET_TOTEMSPIRIT(CH) = SPIRIT_CITY;
        break;
      }
      break;
    case TOTEM_SNAKE:
      if (i < 1 || i > 7) {
        SEND_TO_Q("Invalid spirit bonus:\r\nTotem Spirit Bonus: ", d);
        return;
      }
      switch (i) {
      case 1:
        GET_TOTEMSPIRIT(CH) = SPIRIT_CITY;
        break;
      case 2:
        GET_TOTEMSPIRIT(CH) = SPIRIT_FIELD;
        break;
      case 3:
        GET_TOTEMSPIRIT(CH) = SPIRIT_HEARTH;
        break;
      case 4:
        GET_TOTEMSPIRIT(CH) = SPIRIT_DESERT;
        break;
      case 5:
        GET_TOTEMSPIRIT(CH) = SPIRIT_FOREST;
        break;
      case 6:
        GET_TOTEMSPIRIT(CH) = SPIRIT_MOUNTAIN;
        break;
      case 7:
        GET_TOTEMSPIRIT(CH) = SPIRIT_PRAIRIE;
        break;
      }
      break;
    case TOTEM_WOLF:
      if (i < 1 || i > 3) {
        SEND_TO_Q("Invalid spirit bonus:\r\nTotem Spirit Bonus: ", d);
        return;
      }
      switch (i) {
      case 1:
        GET_TOTEMSPIRIT(CH) = SPIRIT_FOREST;
        break;
      case 2:
        GET_TOTEMSPIRIT(CH) = SPIRIT_PRAIRIE;
        break;
      case 3:
        GET_TOTEMSPIRIT(CH) = SPIRIT_MOUNTAIN;
        break;
      }
      break;
    }
    start_game(d);
    break;
  case CCR_PRIORITY:
    switch (LOWER(*arg)) {
    case 'a':
    case 'b':
    case 'c':
    case 'd':
    case 'e':
      if (d->ccr.pr[(int)(LOWER(*arg)-'a')] == PR_RACE)
        priority_menu(d);
      else {
        d->ccr.temp = (int)(LOWER(*arg)-'a');
        SEND_TO_Q(assign_menu, d);
        SEND_TO_Q("\r\nPriority to assign (c to clear): ", d);
        d->ccr.mode = CCR_ASSIGN;
      }
      break;
    case 'p':
      ok = TRUE;
      for (i = 0; ok && i < 5; i++)
        ok = (d->ccr.pr[i] != PR_NONE);
      if (ok) {
        if (d->ccr.pr[0] == PR_MAGIC || d->ccr.pr[1] == PR_MAGIC)
          set_attributes(d->character, 1);
        else
          set_attributes(d->character, 0);
        if (d->ccr.pr[0] == PR_MAGIC || d->ccr.pr[1] == PR_MAGIC) {
          d->ccr.mode = CCR_TRADITION;
          SEND_TO_Q("\r\nFollow [h]ermetic, [s]hamanic or som[a]tic (Adept) tradition? ", d);
          return;
        } else
          GET_TRADITION(d->character) = TRAD_MUNDANE;
        start_game(d);
      } else
        priority_menu(d);
      break;
    case '?':
      Help.FindTopic(buf2, "priorities");
      strcat(buf2, "\r\nª Press [return] to continue ´");
      SEND_TO_Q(buf2, d);
      d->ccr.temp = CCR_PRIORITY;
      d->ccr.mode = CCR_AWAIT_CR;
      break;
    default:
      priority_menu(d);
    }
    break;
  case CCR_ASSIGN:
    if (!(i = parse_assign(d, arg)))
      SEND_TO_Q("\r\nThat's not a valid priority!\r\nPriority to assign (c to clear): ", d);
    else
      priority_menu(d);
    break;

  case CCR_TRADITION:
    switch (LOWER(*arg)) {
    case 'h':
      GET_TRADITION(d->character) = TRAD_HERMETIC;
      if (d->ccr.pr[1] == PR_MAGIC) {
        GET_FORCE_POINTS(CH) = 35;
        ccr_aspect_menu(d);
      } else {
        GET_FORCE_POINTS(CH) = 25;
        start_game(d);
      }
      break;
    case 's':
      GET_TRADITION(d->character) = TRAD_SHAMANIC;
      if (d->ccr.pr[1] == PR_MAGIC) {
        GET_FORCE_POINTS(CH) = 35;
        ccr_aspect_menu(d);
      } else {
        GET_FORCE_POINTS(CH) = 25;
        ccr_totem_menu(d);
      }
      break;
    case 'a':
      GET_TRADITION(d->character) = TRAD_ADEPT;
      start_game(d);
      break;
    default:
      SEND_TO_Q("\r\nThat's not a tradition!\r\n"
                "Follow [h]ermetic, [s]hamanic or som[a]tic tradition? ", d);
      break;
    }
    break;
  case CCR_ASPECT:
    i = atoi(arg);
    if (GET_TRADITION(d->character) == TRAD_SHAMANIC) {
      switch (i) {
      case 1:
        GET_ASPECT(d->character) = ASPECT_CONJURER;
        break;
      case 2:
        GET_ASPECT(d->character) = ASPECT_SORCERER;
        break;
      case 3:
        GET_ASPECT(d->character) = ASPECT_SHAMANIST;
        break;
      default:
        SEND_TO_Q("\r\nInvalid Aspect! Aspect: ", d);
        return;
      }
      ccr_totem_menu(d);
      return;
    } else
      switch (i) {
      case 1:
        GET_ASPECT(d->character) = ASPECT_CONJURER;
        break;
      case 2:
        GET_ASPECT(d->character) = ASPECT_SORCERER;
        break;
      case 3:
        GET_ASPECT(d->character) = ASPECT_ELEMEARTH;
        break;
      case 4:
        GET_ASPECT(d->character) = ASPECT_ELEMAIR;
        break;
      case 5:
        GET_ASPECT(d->character) = ASPECT_ELEMFIRE;
        break;
      case 6:
        GET_ASPECT(d->character) = ASPECT_ELEMWATER;
        break;
      default:
        SEND_TO_Q("\r\nInvalid Aspect! Aspect: ", d);
        return;
      }
    start_game(d);
    break;
  }
}

