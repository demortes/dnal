/* ***********************************************************************
*   File: house.c                                       Part of CircleMUD *
*  Usage: Handling of player houses                                       *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <time.h>
#include <errno.h>


#if defined(WIN32) && !defined(__CYGWIN__)
#else
#include <unistd.h>
#endif

#include "structs.h"
#include "awake.h"
#include "comm.h"
#include "handler.h"
#include "db.h"
#include "newdb.h"
#include "interpreter.h"
#include "utils.h"
#include "house.h"
#include "memory.h"
#include "constants.h"
#include "file.h"
#include "vtable.h"

extern char *cleanup(char *dest, const char *src);
extern void ASSIGNMOB(int mob, SPECIAL(fname));
struct landlord *landlords = NULL;
ACMD(do_say);

struct life_data
{
  char *name;
  sh_int cost;
};

struct life_data lifestyle[] =
  {
    { "Low", 1
    },
    { "Middle", 3 },
    { "High", 10 },
    { "Luxury", 25 }
  };

/* First, the basics: finding the filename; loading/saving objects */

/* Return a filename given a house vnum */
bool House_get_filename(vnum_t vnum, char *filename)
{
  if (vnum == NOWHERE)
    return FALSE;

  sprintf(filename, "house/%ld.house", vnum);
  return TRUE;
}

/* Load all objects for a house */
bool House_load(struct house_control_rec *house)
{
  vnum_t room;
  File fl;
  char fname[MAX_STRING_LENGTH];
  rnum_t rnum;
  struct obj_data *obj = NULL, *last_obj = NULL;
  struct phone_data *k, *j;
  long vnum;
  int inside = 0, last_in = 0;

  room = house->vnum;
  if ((rnum = real_room(room)) == NOWHERE)
    return FALSE;
  if (!House_get_filename(room, fname))
    return FALSE;
  if (!(fl.Open(fname, "r+b"))) /* no file found */
    return FALSE;
  VTable data;
  data.Parse(&fl);
  fl.Close();
  for (int i = 0; i < MAX_GUESTS; i++)
  {
    sprintf(buf, "GUESTS/Guest%d", i);
    house->guests[i] = data.GetLong(buf, 0);
  }
  int num_objs = data.NumSubsections("HOUSE");
  for (int i = 0; i < num_objs; i++)
  {
    const char *sect_name = data.GetIndexSection("HOUSE", i);
    sprintf(buf, "%s/Vnum", sect_name);
    vnum = data.GetLong(buf, 0);
    if (vnum > 0 && (obj = read_object(vnum, VIRTUAL))) {
      sprintf(buf, "%s/Name", sect_name);
      obj->restring = str_dup(data.GetString(buf, NULL));
      sprintf(buf, "%s/Photo", sect_name);
      obj->photo = str_dup(data.GetString(buf, NULL));
      for (int x = 0; x < 10; x++) {
        sprintf(buf, "%s/Value %d", sect_name, x);
        GET_OBJ_VAL(obj, x) = data.GetInt(buf, GET_OBJ_VAL(obj, x));
      }
      if (GET_OBJ_TYPE(obj) == ITEM_PHONE && GET_OBJ_VAL(obj, 2)) {
        bool found = FALSE;
        sprintf(buf, "%d%d", GET_OBJ_VAL(obj, 0), GET_OBJ_VAL(obj, 1));
        for (j = phone_list; j; j = j->next)
          if (j->number == atoi(buf)) {
            found = TRUE;
            break;
          }
        if (!found) {
          k = new phone_data;
          k->number = atoi(buf);
          k->phone = obj;
          if (phone_list)
            k->next = phone_list;
          phone_list = k;
        }
      }
      sprintf(buf, "%s/ExtraFlags", sect_name);
      GET_OBJ_EXTRA(obj).FromString(data.GetString(buf, "0"));
      sprintf(buf, "%s/AffFlags", sect_name);
      obj->obj_flags.bitvector.FromString(data.GetString(buf, "0"));
      sprintf(buf, "%s/Condition", sect_name);
      GET_OBJ_CONDITION(obj) = data.GetInt(buf, GET_OBJ_CONDITION(obj));
      sprintf(buf, "%s/Timer", sect_name);
      GET_OBJ_TIMER(obj) = data.GetInt(buf, GET_OBJ_TIMER(obj));
      sprintf(buf, "%s/Cost", sect_name);
      GET_OBJ_COST(obj) = data.GetInt(buf, GET_OBJ_COST(obj));
      for (int c = 0; c < MAX_OBJ_AFFECT; c++) {
        sprintf(buf, "%s/Affect%dLoc", sect_name, c);
        obj->affected[c].location = data.GetInt(buf, 0);
        sprintf(buf, "%s/Affect%dmod", sect_name, c);
        obj->affected[c].modifier = data.GetInt(buf, 0);
      }
      sprintf(buf, "%s/Inside", sect_name);
      inside = data.GetInt(buf, 0);
      if (inside > 0) {
        if (inside == last_in)
          last_obj = last_obj->in_obj;
        else if (inside < last_in)
          while (inside <= last_in) {
            last_obj = last_obj->in_obj;
            last_in--;
          }
        obj_to_obj(obj, last_obj);
      } else
        obj_to_room(obj, rnum);

      last_in = inside;
      last_obj = obj;
    }
  }
  return TRUE;
}

void House_save(struct house_control_rec *house, FILE *fl, long rnum)
{
  int level = 0, o = 0;
  struct obj_data *obj = NULL;
  if (house)
  {
    obj = world[real_room(house->vnum)].contents;
    fprintf(fl, "[GUESTS]\n");
    for (;o < MAX_GUESTS; o++)
      fprintf(fl, "\tGuest%d:\t%ld\n", o, house->guests[o]);
  } else
  {
    obj = world[rnum].contents;
  }
  fprintf(fl, "[HOUSE]\n");
  o = 0;
  for (;obj;)
  {
    if (!IS_OBJ_STAT(obj, ITEM_NORENT) || !GET_OBJ_TYPE(obj) == ITEM_KEY) {
      fprintf(fl, "\t[Object %d]\n", o);
      o++;
      fprintf(fl, "\t\tVnum:\t%ld\n", GET_OBJ_VNUM(obj));
      fprintf(fl, "\t\tInside:\t%d\n", level);
      if (GET_OBJ_TYPE(obj) == ITEM_PHONE)
        for (int x = 0; x < 4; x++)
          fprintf(fl, "\t\tValue %d:\t%d\n", x, GET_OBJ_VAL(obj, x));
      else if (GET_OBJ_TYPE(obj) != ITEM_WORN)
        for (int x = 0; x < 10; x++)
          fprintf(fl, "\t\tValue %d:\t%d\n", x, GET_OBJ_VAL(obj, x));
      fprintf(fl, "\t\tExtraFlags:\t%s\n", GET_OBJ_EXTRA(obj).ToString());
      fprintf(fl, "\t\tAffFlags:\t%s\n", obj->obj_flags.bitvector.ToString());
      fprintf(fl, "\t\tCondition:\t%d\n", GET_OBJ_CONDITION(obj));
      fprintf(fl, "\t\tTimer:\t%d\n", GET_OBJ_TIMER(obj));
      fprintf(fl, "\t\tCost:\t%d\n", GET_OBJ_COST(obj));
      if (obj->restring)
        fprintf(fl, "\t\tName:\t%s\n", obj->restring);
      if (obj->photo)
        fprintf(fl, "\t\tPhoto:$\n%s~\n", cleanup(buf2, obj->photo));
      for (int c = 0; c < MAX_OBJ_AFFECT; c++)
        if (obj->affected[c].location != APPLY_NONE && obj->affected[c].modifier != 0)
          fprintf(fl, "\t\tAffect%dLoc:\t%d\n\t\tAffect%dmod:\t%d\n",
                  c, obj->affected[c].location, c, obj->affected[c].modifier);
    }

    if (obj->contains && !IS_OBJ_STAT(obj, ITEM_NORENT) && GET_OBJ_TYPE(obj) != ITEM_PART) {
      obj = obj->contains;
      level++;
      continue;
    } else if (!obj->next_content && obj->in_obj)
      while (obj && !obj->next_content && level >= 0) {
        obj = obj->in_obj;
        level--;
      }

    if (obj)
      obj = obj->next_content;
  }
}

struct house_control_rec *find_house(vnum_t vnum)
{
  struct landlord *llord;
  struct house_control_rec *room;

  for (llord = landlords; llord; llord = llord->next)
    for (room = llord->rooms; room; room = room->next)
      if (room->vnum == vnum)
        return room;

  return NULL;
}

/* Save all objects in a house */
void House_crashsave(vnum_t vnum)
{
  int rnum;
  char buf[MAX_STRING_LENGTH];
  FILE *fp;

  if ((rnum = real_room(vnum)) == NOWHERE)
    return;
  if (!House_get_filename(vnum, buf))
    return;
  if (!(fp = fopen(buf, "wb"))) {
    perror("SYSERR: Error saving house file");
    return;
  }
  House_save(find_house(vnum), fp, 0);
  fclose(fp);
  ROOM_FLAGS(rnum).RemoveBit(ROOM_HOUSE_CRASH);
}

/* Delete a house save file */
void House_delete_file(vnum_t vnum)
{
  char buf[MAX_INPUT_LENGTH], fname[MAX_INPUT_LENGTH];
  FILE *fl;

  if (!House_get_filename(vnum, fname))
    return;
  if (!(fl = fopen(fname, "rb"))) {
    if (errno != ENOENT) {
      sprintf(buf, "SYSERR: Error deleting house file #%ld. (1)", vnum);
      perror(buf);
    }
    return;
  }
  fclose(fl);
  if (unlink(fname) < 0) {
    sprintf(buf, "SYSERR: Error deleting house file #%ld. (2)", vnum);
    perror(buf);
  }
}

/******************************************************************
 *  Functions for house administration (creation, deletion, etc.  *
 *****************************************************************/


/* Save the house control information */
void House_save_control(void)
{
  struct house_control_rec *room;
  struct landlord *llord = landlords;
  int i = 0;
  FILE *fl;
  if (!(fl = fopen(HCONTROL_FILE, "w+b"))) {
    perror("SYSERR: Unable to open house control file");
    return;
  }
  for (;llord; llord = llord->next)
    i++;
  fprintf(fl, "%d\n", i);
  for (llord = landlords; llord; llord = llord->next) {
    fprintf(fl, "%ld %s %d %d\n", llord->vnum, llord->race.ToString(), llord->basecost, llord->num_room);
    for (room = llord->rooms; room; room = room->next)
      fprintf(fl, "%ld %ld %d %d %s %ld 0 %ld\n", room->vnum, room->key, room->exit_num, room->mode,
              room->name, room->owner, room->owner ? room->date : 0);
  }

  fclose(fl);
}

struct house_control_rec *find_room(char *arg, struct house_control_rec *room, struct char_data *recep)
{
  for (; room; room = room->next)
    if (isname(arg, room->name))
      return room;
  do_say(recep, "Which room is that?", 0, 0);
  return NULL;
}

SPECIAL(landlord_spec)
{
  struct char_data *recep = (struct char_data *) me;
  struct landlord *lord;
  struct house_control_rec *room;
  char buf3[MAX_STRING_LENGTH];

  int i = 0;
  if (!(CMD_IS("list") || CMD_IS("retrieve") || CMD_IS("lease")
        || CMD_IS("leave") || CMD_IS("pay") || CMD_IS("status")))
    return FALSE;

  if (!CAN_SEE(recep, ch)) {
    do_say(recep, "I don't deal with people I can't see!", 0, 0);
    return TRUE;
  }
  for (lord = landlords; lord; lord = lord->next)
    if (GET_MOB_VNUM(recep) == lord->vnum)
      break;
  if (!lord)
    return FALSE;
  one_argument(argument, arg);

  if (CMD_IS("list")) {
    send_to_char(ch, "The following rooms are free: \r\n");
    send_to_char(ch, "Name     Class     Price      Name     Class     Price\r\n");
    send_to_char(ch, "-----    ------    ------     -----    ------    -----\r\n");
    for (room = lord->rooms; room; room = room->next)
      if (!room->owner) {
        if (!i) {
          sprintf(buf, "%-5s    %-6s    %-8d", room->name, lifestyle[room->mode].name,
                  lord->basecost * lifestyle[room->mode].cost);
          i++;
        } else {
          sprintf(buf2, "   %-5s    %-6s    %-8d\r\n", room->name, lifestyle[room->mode].name,
                  lord->basecost * lifestyle[room->mode].cost);
          strcat(buf, buf2);
          send_to_char(ch, buf);
          i = 0;
        }
      }
    if (i)
      strcat(buf, "\r\n\n");
    else
      sprintf(buf, "\r\n");
    send_to_char(ch, buf);
    return TRUE;
  } else if (CMD_IS("retrieve")) {
    if (!*arg) {
      do_say(recep, "Which room would you like the key to?", 0, 0);
      return TRUE;
    }
    room = find_room(arg, lord->rooms, recep);
    if (!room)
      do_say(recep, "Which room is that?", 0, 0);
    else if (room->owner != GET_IDNUM(ch))
      do_say(recep, "I would get fired if I did that.", 0, 0);
    else {
      do_say(recep, "Here you go...", 0, 0);
      struct obj_data *key = read_object(room->key, VIRTUAL);
      obj_to_char(key, ch);
    }
    return TRUE;
  } else if (CMD_IS("lease")) {
    if (!*arg) {
      do_say(recep, "Which room would you like to lease?", 0, 0);
      return TRUE;
    }
    room = find_room(arg, lord->rooms, recep);
    if (!room) {
      do_say(recep, "Which room is that?", 0, 0);
      return TRUE;
    } else if (room->owner) {
      do_say(recep, "Sorry, I'm afraid that room is already taken.", 0, 0);
      return TRUE;
    }
    int cost = lord->basecost * lifestyle[room->mode].cost;
    sprintf(buf3, "That will be %d nuyen.", cost);
    do_say(recep, buf3, 0, 0);
    if (GET_NUYEN(ch) < cost) {
      if (GET_BANK(ch) >= cost) {
        do_say(recep, "You don't have the money on you so I'll transfer it from your bank account.", 0, 0);
        GET_BANK(ch) -= cost;
      } else {
        do_say(recep, "Sorry, you don't have the required funds.", 0, 0);
        return TRUE;
      }
    } else {
      GET_NUYEN(ch) -= cost;
      send_to_char(ch, "You hand over the cash.\r\n");
    }
    do_say(recep, "Thank you, here is your key.", 0, 0);
    struct obj_data *key = read_object(room->key, VIRTUAL);
    obj_to_char(key, ch);
    room->owner = GET_IDNUM(ch);
    room->date = time(0) + (SECS_PER_REAL_DAY*30);
    ROOM_FLAGS(real_room(room->vnum)).SetBits(ROOM_HOUSE, ROOM_PRIVATE, ENDBIT);
    ROOM_FLAGS(room->atrium).SetBit(ROOM_ATRIUM);
    House_crashsave(room->vnum);
    House_save_control();
    return TRUE;
  } else if (CMD_IS("leave")) {
    if (!*arg) {
      do_say(recep, "Which room would you like to leave?", 0, 0);
      return TRUE;
    }
    room = find_room(arg, lord->rooms, recep);
    if (!room)
      do_say(recep, "Which room is that?", 0, 0);
    else if (room->owner != GET_IDNUM(ch))
      do_say(recep, "I would get fired if I did that.", 0, 0);
    else {
      room->owner = 0;
      ROOM_FLAGS(real_room(room->vnum)).RemoveBits(ROOM_HOUSE, ROOM_PRIVATE, ENDBIT);
      ROOM_FLAGS(room->atrium).RemoveBit(ROOM_ATRIUM);
      House_save_control();
      House_delete_file(room->vnum);
      do_say(recep, "I hope you enjoyed your time here.", 0, 0);
    }
    return TRUE;
  } else if (CMD_IS("pay")) {
    if (!*arg) {
      do_say(recep, "Which room would you like to pay the rent to?", 0, 0);
      return TRUE;
    }
    room = find_room(arg, lord->rooms, recep);
    if (!room)
      do_say(recep, "Which room is that?", 0, 0);
    else if (room->owner != GET_IDNUM(ch))
      do_say(recep, "But that isn't your room.", 0, 0);
    else {
      int cost = lord->basecost * lifestyle[room->mode].cost;
      sprintf(buf3, "That will be %d nuyen.", cost);
      do_say(recep, buf3, 0, 0);
      if (GET_NUYEN(ch) < cost) {
        if (GET_BANK(ch) >= cost) {
          do_say(recep, "You don't have the money on you so I'll transfer it from your bank account.", 0, 0);
          GET_BANK(ch) -= cost;
        } else {
          do_say(recep, "Sorry, you don't have the required funds.", 0, 0);
          return TRUE;
        }
      } else {
        GET_NUYEN(ch) -= cost;
        send_to_char(ch, "You hand over the cash.\r\n");
      }
      do_say(recep, "You are payed up for the next period.", 0, 0);
      room->date += SECS_PER_REAL_DAY*30;
      House_save_control();
    }
    return TRUE;
  } else if (CMD_IS("status")) {
    if (!*arg) {
      do_say(recep, "Which room would you like to check the rent on?", 0, 0);
      return TRUE;
    }
    room = find_room(arg, lord->rooms, recep);
    if (!room)
      do_say(recep, "Which room is that?", 0, 0);
    else if (room->owner != GET_IDNUM(ch))
      do_say(recep, "I'm afraid I can't release that information.", 0, 0);
    else {
      sprintf(buf2, "You are payed for another %d days.", (int)((room->date - time(0)) / 86400));
      do_say(recep, buf2, 0, 0);
    }
    return TRUE;
  }
  return FALSE;
}
/* call from boot_db - will load control recs, load objs, set atrium bits */
/* should do sanity checks on vnums & remove invalid records */
void House_boot(void)
{
  vnum_t real_house, owner, land, payed;
  FILE *fl;
  int num_land;
  char line[256], name[20];
  int t[4];
  struct house_control_rec *temp;
  struct landlord *templ = NULL, *lastl = NULL;
  if (!(fl = fopen(HCONTROL_FILE, "r+b"))) {
    log("House control file does not exist.");
    return;
  }
  if (!get_line(fl, line) || sscanf(line, "%d", &num_land) != 1) {
    log("Error at beginning of house control file.");
    return;
  }

  for (int i = 0; i < num_land; i++) {
    get_line(fl, line);
    if (sscanf(line, "%ld %s %d %d", &land, name, t, t + 1) != 4) {
      fprintf(stderr, "Format error in landlord #%d.\r\n", i);
      return;
    }
    ASSIGNMOB(land, landlord_spec);
    templ = new struct landlord;
    templ->vnum = land;
    templ->race.FromString(name);
    templ->basecost = t[0];
    templ->num_room = t[1];
    struct house_control_rec *last = NULL, *first = NULL;
    for (int x = 0; x < templ->num_room; x++) {
      get_line(fl, line);
      if (sscanf(line, "%ld %ld %d %d %s %ld %d %ld", &real_house, &land, t, t + 1, name,
                 &owner, t + 2, &payed) != 8) {
        fprintf(stderr, "Format error in landlord #%d room #%d.\r\n", i, x);
        return;
      }
      temp = new struct house_control_rec;
      temp->vnum = real_house;
      temp->key = land;
      temp->atrium = TOROOM(real_room(real_house), t[0]);
      temp->exit_num = t[0];
      temp->owner = owner;
      temp->date = payed;
      temp->mode = t[1];
      temp->name = str_dup(name);
      if (playerDB.DoesExist(temp->owner) && temp->date > time(0))
        House_load(temp);
      else {
        temp->owner = 0;
        House_delete_file(temp->vnum);
      }
      if (last)
        last->next = temp;
      if (!first)
        first = temp;
      if (temp->owner) {
        ROOM_FLAGS(real_room(temp->vnum)).SetBits(ROOM_HOUSE, ROOM_PRIVATE, ENDBIT);
        ROOM_FLAGS(temp->atrium).SetBit(ROOM_ATRIUM);
      }
      last = temp;
    }
    templ->rooms = first;
    templ->next = lastl;
    lastl = templ;
  }
  landlords = templ;

  fclose(fl);
  House_save_control();
}

/* "House Control" functions */

char *HCONTROL_FORMAT =
  "Usage:  hcontrol destroy <house vnum>\r\n"
  "       hcontrol show\r\n";

static inline const char *NAME(DBIndex::vnum_t id)
{
  const char *name = playerDB.GetNameV(id);

  return (name? name : "<UNDEF>");
}

void hcontrol_list_houses(struct char_data *ch)
{
  char *own_name;

  strcpy(buf, "Address  Atrium  Guests  Owner\r\n");
  strcat(buf, "-------  ------  ------  ------------\r\n");
  send_to_char(buf, ch);

  for (struct landlord *llord = landlords; llord; llord = llord->next)
    for (struct house_control_rec *house = llord->rooms; house; house = house->next)
      if (house->owner)
      {
        own_name = str_dup(playerDB.GetNameV(house->owner));
        if (!own_name)
          own_name = str_dup("<UNDEF>");
        sprintf(buf, "%7ld %7ld    0     %-12s\r\n",
                house->vnum, world[house->atrium].number, CAP(own_name));
        send_to_char(buf, ch);
      }
}

void hcontrol_destroy_house(struct char_data * ch, char *arg)
{
  struct house_control_rec *i = NULL;
  vnum_t real_atrium, real_house;

  if (!*arg)
  {
    send_to_char(HCONTROL_FORMAT, ch);
    return;
  }
  if ((i = find_house(atoi(arg))) == NULL)
  {
    send_to_char("Unknown house.\r\n", ch);
    return;
  }
  if ((real_atrium = real_room(i->atrium)) < 0)
    log("SYSERR: House had invalid atrium!");
  else
    ROOM_FLAGS(real_atrium).RemoveBit(ROOM_ATRIUM);

  if ((real_house = real_room(i->vnum)) < 0)
    log("SYSERR: House had invalid vnum!");
  else
  {
    ROOM_FLAGS(real_house).RemoveBits(ROOM_HOUSE, ROOM_PRIVATE,
                                      ROOM_HOUSE_CRASH, ENDBIT);

    House_delete_file(i->vnum);
    i->owner = 0;
    send_to_char("House deleted.\r\n", ch);
    House_save_control();

    for (struct landlord *llord = landlords; llord; llord = llord->next)
      for (i = llord->rooms; i; i = i->next)
        if ((real_atrium = real_room(i->atrium)) >= 0) {
          ROOM_FLAGS(real_atrium).SetBit(ROOM_ATRIUM);
        }
  }
}


/* The hcontrol command itself, used by imms to create/destroy houses */
ACMD(do_hcontrol)
{
  char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];

  half_chop(argument, arg1, arg2);

  if (is_abbrev(arg1, "destroy"))
    hcontrol_destroy_house(ch, arg2);
  else if (is_abbrev(arg1, "show"))
    hcontrol_list_houses(ch);
  else
    send_to_char(HCONTROL_FORMAT, ch);
}


ACMD(do_decorate)
{
  extern void write_world_to_disk(int vnum);
  struct house_control_rec *i;
  skip_spaces(&argument);
  if (!ROOM_FLAGGED(ch->in_room, ROOM_HOUSE))
    send_to_char("You must be in your house to set the description.\r\n", ch);
  else if ((i = find_house(GET_ROOM_VNUM(IN_ROOM(ch)))) == NULL)
    send_to_char("Um.. this house seems to be screwed up.\r\n", ch);
  else if (GET_IDNUM(ch) != i->owner)
    send_to_char("Only the primary owner can set the room description.\r\n", ch);
  else if (!*argument)
    send_to_char(ch, "How do you wish to decorate the apartment?\r\n");
  else {
    strcat(argument, "\r\n");
    delete [] world[ch->in_room].description;
    world[ch->in_room].description = str_dup(argument);
    write_world_to_disk(zone_table[world[ch->in_room].zone].number);
    send_to_char(ch, "Description Set!\r\n");
  }
}

/* The house command, used by mortal house owners to assign guests */
ACMD(do_house)
{
  struct house_control_rec *i = NULL;
  int j, id;

  one_argument(argument, arg);

  if (!ROOM_FLAGGED(IN_ROOM(ch), ROOM_HOUSE))
    send_to_char("You must be in your house to set guests.\r\n", ch);
  else if ((i = find_house(GET_ROOM_VNUM(IN_ROOM(ch)))) == NULL)
    send_to_char("Um.. this house seems to be screwed up.\r\n", ch);
  else if (GET_IDNUM(ch) != i->owner)
    send_to_char("Only the primary owner can set guests.\r\n", ch);
  else if (!*arg)
    House_list_guests(ch, i, FALSE);
  else if ((id = playerDB.GetID(arg)) < 0)
    send_to_char("No such player.\r\n", ch);
  else if (id == GET_IDNUM(ch))
    send_to_char("It's your house!\r\n", ch);
  else {
    for (j = 0; j < MAX_GUESTS; j++)
      if (i->guests[j] == id) {
        i->guests[j] = 0;
        House_crashsave(i->vnum);
        send_to_char("Guest deleted.\r\n", ch);
        return;
      }
    for (j = 0; j < MAX_GUESTS; j++)
      if (i->guests[j] == 0)
        break;
    if (j == MAX_GUESTS) {
      send_to_char("You have too many guests already.\r\n", ch);
      return;
    }
    i->guests[j] = id;
    House_crashsave(i->vnum);
    send_to_char("Guest added.\r\n", ch);
  }
}



/* Misc. administrative functions */


/* crash-save all the houses */
void House_save_all(void)
{
  struct landlord *llord;
  struct house_control_rec *room;
  rnum_t real_house;

  for (llord = landlords; llord; llord = llord->next)
    for (room = llord->rooms; room; room = room->next)
      if ((real_house = real_room(room->vnum)) != NOWHERE)
        if (ROOM_FLAGGED(real_house, ROOM_HOUSE_CRASH) && room->owner)
          House_crashsave(room->vnum);
  FILE *fl;

  for (int x = 0; x <= top_of_world; x++)
    if (ROOM_FLAGGED(x, ROOM_STORAGE)) {
      sprintf(buf, "storage/%ld", world[x].number);
      if (!(fl = fopen(buf, "wb"))) {
        sprintf(buf2, "Cannot open %s. Aborting Storage Saving.", buf);
        mudlog(buf2, NULL, LOG_WIZLOG, TRUE);
        return;
      }
      House_save(NULL, fl, x);
      fclose(fl);
    }
}


/* note: arg passed must be house vnum, so there. */
bool House_can_enter(struct char_data *ch, vnum_t house)
{
  int j;
  struct house_control_rec *room = find_house(house);

  if (IS_NPC(ch) || !room)
    return FALSE;
  if (GET_LEVEL(ch) >= LVL_ADMIN)
    return TRUE;
  if (GET_IDNUM(ch) == room->owner)
    return TRUE;
  for (j = 0; j < MAX_GUESTS; j++)
    if (GET_IDNUM(ch) == room->guests[j])
      return TRUE;

  return FALSE;
}

void House_list_guests(struct char_data *ch, struct house_control_rec *i, int quiet)
{
  int j;
  char buf[MAX_STRING_LENGTH], buf2[MAX_NAME_LENGTH + 2];

  if (i->guests[0] == 0)
  {
    if (!quiet)
      send_to_char("  Guests: None\r\n", ch);
    return;
  }

  strcpy(buf, "  Guests: ");

  for (j = 0; j < MAX_GUESTS; j++)
  {
    const char *temp = playerDB.GetNameV(i->guests[j]);

    if (!temp || i->guests[j] == 0)
      continue;

    sprintf(buf2, "%s ", temp);
    strcat(buf, CAP(buf2));
  }

  strcat(buf, "\r\n");
  send_to_char(buf, ch);
}
