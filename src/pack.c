/*
 * Routines to deal with the pack
 *
 * @(#)pack.c	X.XX (Berkeley) XX/XX/XX
 */

#include <curses.h>
#include <ctype.h>
#include "rogue.h"

/*
 * add_pack:
 *	Pick up an object and add it to the pack.  If the argument is
 *	non-null use it as the linked_list pointer instead of gettting
 *	it off the ground.
 */
add_pack(obj, silent)
register THING *obj;
bool silent;
{
    register THING *op, *lp;
    bool from_floor;

    from_floor = FALSE;
    if (obj == NULL)
    {
	if ((obj = find_obj(Hero.y, Hero.x)) == NULL)
	{
	    if (!Terse)
		addmsg("there is ");
	    addmsg("nothing here");
	    if (!Terse)
		addmsg(" to pick up");
	    endmsg();
	    return;
	}
	from_floor = TRUE;
    }

    /*
     * Check for and deal with scare monster scrolls
     */
    if (obj->o_type == SCROLL && obj->o_which == S_SCARE)
	if (obj->o_flags & ISFOUND)
	{
	    detach(Lvl_obj, obj);
	    mvaddch(Hero.y, Hero.x, floor_ch());
	    chat(Hero.y, Hero.x) = (Proom->r_flags & ISGONE) ? PASSAGE : FLOOR;
	    discard(obj);
	    msg("the scroll turns to dust as you pick it up");
	    return;
	}

    if (Pack == NULL)
    {
	Inpack = 0;
	pack_room(from_floor, obj);
	Pack = obj;
	obj->o_packch = pack_char();
	obj->o_count = 1;
    }
    else
    {
	lp = NULL;
	for (op = Pack; op != NULL; op = next(op))
	{
	    if (op->o_type != obj->o_type)
		lp = op;
	    else
	    {
		while (op->o_type == obj->o_type && op->o_which != obj->o_which)
		{
		    lp = op;
		    if (next(op) == NULL)
			break;
		    else
			op = next(op);
		}
		if (op->o_type == obj->o_type && op->o_which == obj->o_which)
		    if (ISMULT(op->o_type))
		    {
			if (!pack_room(from_floor, obj))
			    return;
			op->o_count++;
dump_it:
			discard(obj);
			obj = op;
			lp = NULL;
			goto out;
		    }
		    else if (obj->o_group)
		    {
			lp = op;
			while (op->o_type == obj->o_type
			  && op->o_which == obj->o_which
			  && op->o_group != obj->o_group)
			{
			    lp = op;
			    if (next(op) == NULL)
				break;
			    else
				op = next(op);
			}
			if (op->o_type == obj->o_type
			  && op->o_which == obj->o_which
			  && op->o_group == obj->o_group)
			{
			    op->o_count += obj->o_count;
			    Inpack--;
			    if (!pack_room(from_floor, obj))
				return;
			    goto dump_it;
			}
		    }
		    else
			lp = op;
out:
		break;
	    }
	}

	if (lp != NULL)
	    if (!pack_room(from_floor, obj))
		return;
	    else
	    {
		obj->o_packch = pack_char();
		next(obj) = next(lp);
		prev(obj) = lp;
		if (next(lp) != NULL)
		    prev(next(lp)) = obj;
		next(lp) = obj;
	    }
    }

    obj->o_flags |= ISFOUND;

    /*
     * If this was the object of something's desire, that monster will
     * get mad and run at the hero.
     */
    for (op = Mlist; op != NULL; op = next(op))
	if (op->t_dest = &obj->o_pos)
	    op->t_dest = &Hero;

    if (obj->o_type == AMULET)
	Amulet = TRUE;
    /*
     * Notify the user
     */
    if (!silent)
    {
	if (!Terse)
	    addmsg("you now have ");
	msg("%s (%c)", inv_name(obj, !Terse), obj->o_packch);
    }
}

/*
 * pack_room:
 *	See if there's room in the pack.  If not, print out an
 *	appropriate message
 */
bool
pack_room(from_floor, obj)
bool from_floor;
THING *obj;
{
    if (++Inpack > MAXPACK)
    {
	if (!Terse)
	    addmsg("there's ");
	addmsg("no room");
	if (!Terse)
	    addmsg(" in your pack");
	endmsg();
	if (from_floor)
	    move_msg(obj);
	Inpack = MAXPACK;
	return FALSE;
    }

    if (from_floor)
    {
	detach(Lvl_obj, obj);
	mvaddch(Hero.y, Hero.x, floor_ch());
	chat(Hero.y, Hero.x) = (Proom->r_flags & ISGONE) ? PASSAGE : FLOOR;
    }

    return TRUE;
}

/*
 * leave_pack:
 *	Take an item out of the pack
 */
THING *
leave_pack(obj, newobj, all)
register THING *obj;
bool newobj;
bool all;
{
    register THING *nobj;

    Inpack--;
    nobj = obj;
    if (obj->o_count > 1 && !all)
    {
	Last_pick = obj;
	obj->o_count--;
	if (obj->o_group)
	    Inpack++;
	if (newobj)
	{
	    nobj = new_item();
	    *nobj = *obj;
	    next(nobj) = NULL;
	    prev(nobj) = NULL;
	    nobj->o_count = 1;
	}
    }
    else
    {
	Last_pick = NULL;
	Pack_used[obj->o_packch - 'a'] = FALSE;
	detach(Pack, obj);
    }
    return nobj;
}

/*
 * pack_char:
 *	Return the next unused pack character.
 */
pack_char()
{
    register bool *bp;

    for (bp = Pack_used; *bp; bp++)
	continue;
    *bp = TRUE;
    return (bp - Pack_used) + 'a';
}

/*
 * inventory:
 *	List what is in the pack.  Return TRUE if there is something of
 *	the given type.
 */
bool
inventory(list, type)
register THING *list;
register int type;
{
    register int dum;
    static char inv_temp[MAXSTR];
    char *index();

    N_objs = 0;
    for (; list != NULL; list = next(list))
    {
	if (type && type != list->o_type && !(type == CALLABLE &&
	    list->o_type != FOOD && list->o_type != AMULET) &&
	    !(type == R_OR_S && (list->o_type == RING || list->o_type == STICK)))
		continue;
	N_objs++;
#ifdef MASTER
	if (!list->o_packch)
	    strcpy(inv_temp, "%s");
	else
#endif
	    sprintf(inv_temp, "%c) %%s", list->o_packch);
	if (add_line(inv_temp, inv_name(list, FALSE)) == ESCAPE)
	{
	    msg("");
	    return FALSE;
	}
    }
    if (N_objs == 0)
    {
	Msg_esc = FALSE;
	if (Terse)
	    msg(type == 0 ? "empty handed" :
			    "nothing appropriate");
	else
	    msg(type == 0 ? "you are empty handed" :
			    "you don't have anything appropriate");
	return FALSE;
    }
    if (end_line() == ESCAPE)
    {
	msg("");
	return FALSE;
    }
    return TRUE;
}

/*
 * pick_up:
 *	Add something to characters pack.
 */
pick_up(ch)
char ch;
{
    register THING *obj;

    if (on(Player, ISLEVIT))
	return;

    obj = find_obj(Hero.y, Hero.x);
    if (Move_on)
	move_msg(obj);
    else
	switch (ch)
	{
	    case GOLD:
		if (obj == NULL)
		    return;
		money(obj->o_goldval);
		detach(Lvl_obj, obj);
		discard(obj);
		Proom->r_goldval = 0;
		break;
	    default:
#ifdef MASTER
		debug("Where did you pick a '%s' up???", unctrl(ch));
#endif
	    case ARMOR:
	    case POTION:
	    case FOOD:
	    case WEAPON:
	    case SCROLL:
	    case AMULET:
	    case RING:
	    case STICK:
		add_pack((THING *) NULL, FALSE);
		break;
	}
}

/*
 * move_msg:
 *	Print out the message if you are just moving onto an object
 */
move_msg(obj)
THING *obj;
{
    if (!Terse)
	addmsg("you ");
    msg("moved onto %s", inv_name(obj, TRUE));
}

/*
 * picky_inven:
 *	Allow player to inventory a single item
 */
picky_inven()
{
    register THING *obj;
    char mch;

    if (Pack == NULL)
	msg("you aren't carrying anything");
    else if (next(Pack) == NULL)
	msg("a) %s", inv_name(Pack, FALSE));
    else
    {
	msg(Terse ? "item: " : "which item do you wish to inventory: ");
	Mpos = 0;
	if ((mch = readchar()) == ESCAPE)
	{
	    msg("");
	    return;
	}
	for (obj = Pack; obj != NULL; obj = next(obj))
	    if (mch == obj->o_packch)
	    {
		msg("%c) %s", mch, inv_name(obj, FALSE));
		return;
	    }
	msg("'%s' not in pack", unctrl(mch));
    }
}

/*
 * get_item:
 *	Pick something out of a pack for a purpose
 */
THING *
get_item(purpose, type)
char *purpose;
int type;
{
    register THING *obj = NULL;
    char ch;

    if (Pack == NULL)
	msg("you aren't carrying anything");
    else if (Again)
	if (Last_pick)
	    return Last_pick;
	else
	    msg("you ran out");
    else
    {
	for (;;)
	{
	    Msg_esc = TRUE;
	    if (!Terse)
		addmsg("which object do you want to ");
	    addmsg(purpose);
	    if (Terse)
		addmsg(" what");
	    msg("? (* for list): ");
	    ch = readchar();
	    Mpos = 0;
	    /*
	     * Give the poor player a chance to abort the command
	     */
	    if (ch == ESCAPE)
	    {
		reset_last();
		After = FALSE;
		obj = NULL;
		msg("");
		break;
	    }
	    N_objs = 1;		/* normal case: person types one char */
	    if (ch == '*')
	    {
		Mpos = 0;
		if (inventory(Pack, type) == 0)
		{
		    After = FALSE;
		    obj = NULL;
		    break;
		}
		continue;
	    }
	    for (obj = Pack; obj != NULL; obj = next(obj))
		if (obj->o_packch == ch)
		    break;
	    if (obj == NULL)
	    {
		msg("'%s' is not a valid item", unctrl(ch));
		continue;
	    }
	    else
		break;
	}
    }
    Msg_esc = FALSE;
    return obj;
}

/*
 * money:
 *	Add or subtract gold from the pack
 */
money(value)
register int value;
{
    Purse += value;
    mvaddch(Hero.y, Hero.x, floor_ch());
    chat(Hero.y, Hero.x) = (Proom->r_flags & ISGONE) ? PASSAGE : FLOOR;
    if (value > 0)
    {
	if (!Terse)
	    addmsg("you found ");
	msg("%d gold pieces", value);
    }
}

/*
 * floor_ch:
 *	Return the appropriate floor character for her room
 */
floor_ch()
{
    if (Proom->r_flags & ISGONE)
	return PASSAGE;
    return (show_floor() ? FLOOR : ' ');
}

/*
 * floor_at:
 *	Return the character at hero's position, taking See_floor
 *	into account
 */
floor_at()
{
    char ch;

    ch = chat(Hero.y, Hero.x);
    if (ch == FLOOR)
	ch = floor_ch();
    return ch;
}

/*
 * reset_last:
 *	Reset the last command when the current one is aborted
 */
reset_last()
{
    Last_comm = L_last_comm;
    Last_dir = L_last_dir;
    Last_pick = L_last_pick;
}
