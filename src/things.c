/*
 * Contains functions for dealing with things like potions, scrolls,
 * and other items.
 *
 * @(#)things.c	X.XX (Berkeley) XX/XX/XX
 */

#include <curses.h>
#include <ctype.h>
#include "rogue.h"

/*
 * inv_name:
 *	Return the name of something as it would appear in an
 *	inventory.
 */
char *
inv_name(obj, drop)
register THING *obj;
bool drop;
{
    register char *pb;
    register struct obj_info *op;
    register char *sp;
    register int which;

    pb = Prbuf;
    which = obj->o_which;
    switch (obj->o_type)
    {
	when POTION:
	    nameit(obj, "potion", P_colors[which], &Pot_info[which], nullstr);
	when RING:
	    nameit(obj, "ring", R_stones[which], &Ring_info[which], ring_num);
	when STICK:
	    nameit(obj, Ws_type[which], Ws_made[which], &Ws_info[which], charge_str);
	when SCROLL:
	    if (obj->o_count == 1)
	    {
		strcpy(pb, "A scroll ");
		pb = &Prbuf[9];
	    }
	    else
	    {
		sprintf(pb, "%d scrolls ", obj->o_count);
		pb = &Prbuf[strlen(Prbuf)];
	    }
	    op = &Scr_info[which];
	    if (op->oi_know)
		sprintf(pb, "of %s", op->oi_name);
	    else if (op->oi_guess)
		sprintf(pb, "called %s", op->oi_guess);
	    else
		sprintf(pb, "titled '%s'", S_names[which]);
	when FOOD:
	    if (which == 1)
		if (obj->o_count == 1)
		    sprintf(pb, "A%s %s", vowelstr(Fruit), Fruit);
		else
		    sprintf(pb, "%d %ss", obj->o_count, Fruit);
	    else
		if (obj->o_count == 1)
		    strcpy(pb, "Some food");
		else
		    sprintf(pb, "%d rations of food", obj->o_count);
	when WEAPON:
	    sp = Weap_info[which].oi_name;
	    if (obj->o_count > 1)
		sprintf(pb, "%d ", obj->o_count);
	    else
		sprintf(pb, "A%s ", vowelstr(sp));
	    pb = &Prbuf[strlen(Prbuf)];
	    if (obj->o_flags & ISKNOW)
		sprintf(pb, "%s %s", num(obj->o_hplus,obj->o_dplus,WEAPON), sp);
	    else
		sprintf(pb, "%s", sp);
	    if (obj->o_count > 1)
		strcat(pb, "s");
	    if (obj->o_label != NULL)
	    {
		pb = &Prbuf[strlen(Prbuf)];
		sprintf(pb, " called %s", obj->o_label);
	    }
	when ARMOR:
	    sp = Arm_info[which].oi_name;
	    if (obj->o_flags & ISKNOW)
	    {
		sprintf(pb, "%s %s [",
		    num(A_class[which] - obj->o_arm, 0, ARMOR), sp);
		if (!Terse)
		    strcat(pb, "protection ");
		pb = &Prbuf[strlen(Prbuf)];
		sprintf(pb, "%d]", 10 - obj->o_arm);
	    }
	    else
		sprintf(pb, "%s", sp);
	    if (obj->o_label != NULL)
	    {
		pb = &Prbuf[strlen(Prbuf)];
		sprintf(pb, " called %s", obj->o_label);
	    }
	when AMULET:
	    strcpy(pb, "The Amulet of Yendor");
	when GOLD:
	    sprintf(Prbuf, "%d Gold pieces", obj->o_goldval);
#ifdef MASTER
	otherwise:
	    debug("Picked up something funny %s", unctrl(obj->o_type));
	    sprintf(pb, "Something bizarre %s", unctrl(obj->o_type));
#endif
    }
    if (Inv_describe)
    {
	if (obj == Cur_armor)
	    strcat(pb, " (being worn)");
	if (obj == Cur_weapon)
	    strcat(pb, " (weapon in hand)");
	if (obj == Cur_ring[LEFT])
	    strcat(pb, " (on left hand)");
	else if (obj == Cur_ring[RIGHT])
	    strcat(pb, " (on right hand)");
    }
    if (drop && isupper(Prbuf[0]))
	Prbuf[0] = tolower(Prbuf[0]);
    else if (!drop && islower(*Prbuf))
	*Prbuf = toupper(*Prbuf);
    Prbuf[MAXSTR-1] = '\0';
    return Prbuf;
}

/*
 * drop:
 *	Put something down
 */
drop()
{
    char ch;
    register THING *obj;
    bool nflytrap;

    ch = chat(Hero.y, Hero.x);
    if (ch != FLOOR && ch != PASSAGE)
    {
	After = FALSE;
	msg("there is something there already");
	return;
    }
    if ((obj = get_item("drop", 0)) == NULL)
	return;
    if (!dropcheck(obj))
	return;
    obj = leave_pack(obj, TRUE, !ISMULT(obj->o_type));
    /*
     * Link it into the level object list
     */
    attach(Lvl_obj, obj);
    chat(Hero.y, Hero.x) = obj->o_type;
    flat(Hero.y, Hero.x) |= F_DROPPED;
    obj->o_pos = Hero;
    nflytrap = FALSE;
    if (obj->o_type == AMULET)
	Amulet = FALSE;
    else if (obj->o_type == SCROLL && obj->o_which == S_SCARE
	&& on(Player, ISHELD))
    {
	nflytrap = TRUE;
	reset_fight();
    }
    if (!Terse)
	msg("you ");
    addmsg("dropped %s", inv_name(obj, TRUE));
    endmsg();
    if (nflytrap)
	msg("the %s suddenly lets you go", Monsters['F'-'A'].m_name);
}

/*
 * dropcheck:
 *	Do special checks for dropping or unweilding|unwearing|unringing
 */
bool
dropcheck(obj)
register THING *obj;
{
    if (obj == NULL)
	return TRUE;
    if (obj != Cur_armor && obj != Cur_weapon
	&& obj != Cur_ring[LEFT] && obj != Cur_ring[RIGHT])
	    return TRUE;
    if (obj->o_flags & ISCURSED)
    {
	msg("you can't.  It appears to be cursed");
	return FALSE;
    }
    if (obj == Cur_weapon)
	Cur_weapon = NULL;
    if (obj == Cur_armor)
    {
	waste_time();
	Cur_armor = NULL;
    }
    else
    {
	Cur_ring[obj == Cur_ring[LEFT] ? LEFT : RIGHT] = NULL;
	switch (obj->o_which)
	{
	    case R_ADDSTR:
		chg_str(-obj->o_arm);
		break;
	    case R_SEEINVIS:
		unsee();
		extinguish(unsee);
		break;
	}
    }
    return TRUE;
}

/*
 * new_thing:
 *	Return a new thing
 */
THING *
new_thing()
{
    register THING *cur;
    register int r;

    cur = new_item();
    cur->o_hplus = 0;
    cur->o_dplus = 0;
    cur->o_damage = cur->o_hurldmg = "0x0";
    cur->o_arm = 11;
    cur->o_count = 1;
    cur->o_group = 0;
    cur->o_flags = 0;
    /*
     * Decide what kind of object it will be
     * If we haven't had food for a while, let it be food.
     */
    switch (No_food > 3 ? 2 : pick_one(Things, NUMTHINGS))
    {
	when 0:
	    cur->o_type = POTION;
	    cur->o_which = pick_one(Pot_info, MAXPOTIONS);
	when 1:
	    cur->o_type = SCROLL;
	    cur->o_which = pick_one(Scr_info, MAXSCROLLS);
	when 2:
	    cur->o_type = FOOD;
	    No_food = 0;
	    if (rnd(10) != 0)
		cur->o_which = 0;
	    else
		cur->o_which = 1;
	when 3:
	    init_weapon(cur, pick_one(Weap_info, MAXWEAPONS));
	    if ((r = rnd(100)) < 10)
	    {
		cur->o_flags |= ISCURSED;
		cur->o_hplus -= rnd(3) + 1;
	    }
	    else if (r < 15)
		cur->o_hplus += rnd(3) + 1;
	when 4:
	    cur->o_type = ARMOR;
	    cur->o_which = pick_one(Arm_info, MAXARMORS);
	    cur->o_arm = A_class[cur->o_which];
	    if ((r = rnd(100)) < 20)
	    {
		cur->o_flags |= ISCURSED;
		cur->o_arm += rnd(3) + 1;
	    }
	    else if (r < 28)
		cur->o_arm -= rnd(3) + 1;
	when 5:
	    cur->o_type = RING;
	    cur->o_which = pick_one(Ring_info, MAXRINGS);
	    switch (cur->o_which)
	    {
		when R_ADDSTR:
		case R_PROTECT:
		case R_ADDHIT:
		case R_ADDDAM:
		    if ((cur->o_arm = rnd(3)) == 0)
		    {
			cur->o_arm = -1;
			cur->o_flags |= ISCURSED;
		    }
		when R_AGGR:
		case R_TELEPORT:
		    cur->o_flags |= ISCURSED;
	    }
	when 6:
	    cur->o_type = STICK;
	    cur->o_which = pick_one(Ws_info, MAXSTICKS);
	    fix_stick(cur);
#ifdef MASTER
	otherwise:
	    debug("Picked a bad kind of object");
	    wait_for(' ');
#endif
    }
    return cur;
}

/*
 * pick_one:
 *	Pick an item out of a list of nitems possible objects
 */
int
pick_one(info, nitems)
register struct obj_info *info;
int nitems;
{
    register struct obj_info *end;
    register struct obj_info *start;
    register int i;

    start = info;
    for (end = &info[nitems], i = rnd(100); info < end; info++)
	if (i < info->oi_prob)
	    break;
    if (info == end)
    {
#ifdef MASTER
	if (Wizard)
	{
	    msg("bad pick_one: %d from %d items", i, nitems);
	    for (info = start; info < end; info++)
		msg("%s: %d%%", info->oi_name, info->oi_prob);
	}
#endif
	info = start;
    }
    return info - start;
}

/*
 * discovered:
 *	list what the player has discovered in this game of a certain type
 */
static int Line_cnt = 0;

static bool Newpage = FALSE;

static char *Lastfmt, *Lastarg;

discovered()
{
    char ch;
    bool disc_list;
    char *index();

    do {
	disc_list = FALSE;
	if (!Terse)
	    addmsg("for ");
	addmsg("what type");
	if (!Terse)
	    addmsg(" of object do you want a list");
	msg("? (* for all)");
	ch = readchar();
	switch (ch)
	{
	    case ESCAPE:
		msg("");
		return;
	    case POTION:
	    case SCROLL:
	    case RING:
	    case STICK:
	    case '*':
		disc_list = TRUE;
		break;
	    default:
		if (Terse)
		    msg("Not a type");
		else
		    msg("Please type one of %c%c%c%c (ESCAPE to quit)", POTION, SCROLL, RING, STICK);
	}
    } while (!disc_list);
    if (ch == '*')
    {
	print_disc(POTION);
	add_line("");
	print_disc(SCROLL);
	add_line("");
	print_disc(RING);
	add_line("");
	print_disc(STICK);
	end_line();
    }
    else
    {
	print_disc(ch);
	end_line();
    }
}

/*
 * print_disc:
 *	Print what we've discovered of type 'type'
 */

#define MAX4(a,b,c,d)	(a > b ? (a > c ? (a > d ? a : d) : (c > d ? c : d)) : (b > c ? (b > d ? b : d) : (c > d ? c : d)))

print_disc(type)
char type;
{
    register struct obj_info *info;
    register int i, maxnum, num_found;
    static THING obj;
    static short order[MAX4(MAXSCROLLS, MAXPOTIONS, MAXRINGS, MAXSTICKS)];

    switch (type)
    {
	case SCROLL:
	    maxnum = MAXSCROLLS;
	    info = Scr_info;
	    break;
	case POTION:
	    maxnum = MAXPOTIONS;
	    info = Pot_info;
	    break;
	case RING:
	    maxnum = MAXRINGS;
	    info = Ring_info;
	    break;
	case STICK:
	    maxnum = MAXSTICKS;
	    info = Ws_info;
	    break;
    }
    set_order(order, maxnum);
    obj.o_count = 1;
    obj.o_flags = 0;
    num_found = 0;
    for (i = 0; i < maxnum; i++)
	if (info[order[i]].oi_know || info[order[i]].oi_guess)
	{
	    obj.o_type = type;
	    obj.o_which = order[i];
	    add_line("%s", inv_name(&obj, FALSE));
	    num_found++;
	}
    if (num_found == 0)
	add_line(nothing(type));
}

/*
 * set_order:
 *	Set up order for list
 */
set_order(order, numthings)
short *order;
int numthings;
{
    register int i, r, t;

    for (i = 0; i< numthings; i++)
	*(short *)((char *)order + i + i) = i;

    for (i = numthings; i > 0; i--)
    {
	r = rnd(i);
	t = order[i - 1];
	order[i - 1] = *(short *)((char *)order + r + r);
	*(short *)((char *)order + r + r) = t;
    }
}

/*
 * add_line:
 *	Add a line to the list of discoveries
 */
/* VARARGS1 */
char
add_line(fmt, arg)
char *fmt;
char *arg;
{
    register WINDOW *sw, *tw;
    register WINDOW *savscr;
    register int x, y;
    register char *prompt = "--Press space to continue--";
    static int maxlen = -1;

    if (Line_cnt == 0)
    {
	werase(Hw);
	if (Inv_type == INV_SLOW)
	    Mpos = 0;
    }
    if (Inv_type == INV_SLOW)
    {
	if (*fmt != '\0')
	    if (msg(fmt, arg) == ESCAPE)
		return ESCAPE;
	Line_cnt++;
    }
    else
    {
	if (maxlen < 0)
	    maxlen = strlen(prompt);
	if (Line_cnt >= LINES - 1 || fmt == NULL)
	{
	    if (Inv_type == INV_OVER && fmt == NULL && !Newpage)
	    {
		msg("");
		refresh();
		tw = newwin(Line_cnt + 1, maxlen, 0, 0);
		overwrite(Hw, tw);
		if (LINES <= NUMLINES || NUMLINES + Line_cnt >= LINES)
		{
		    sw = newwin(Line_cnt + 1, maxlen + 1, 0, 0);
		    mvwin(tw, 0, 1);
		    overwrite(tw, sw);
		    wmove(tw, Line_cnt, 0);
		    waddstr(tw, prompt);
		    /*
		     * if there are lines below, use 'em
		     */
		    if (LINES > NUMLINES)
			mvwin(sw, LINES - (Line_cnt + 1), COLS - (maxlen + 1));
		    else
			mvwin(sw, 0, COLS - (maxlen + 1));
		    delwin(tw);
		    wmove(sw, Line_cnt, 1);
		}
		else
		{
		    mvwin(tw, NUMLINES, 0);
		    sw = tw;
		    wmove(sw, Line_cnt, 0);
		}
		waddstr(sw, prompt);
		touchwin(sw);
		wrefresh(sw);
		savscr = stdscr;
		stdscr = sw;
		wait_for(' ');
		stdscr = savscr;
		if (CE)
		{
		    werase(sw);
		    leaveok(sw, TRUE);
		    wrefresh(sw);
		}
		touchoverlap(sw, stdscr);
		delwin(sw);
	    }
	    else
	    {
		wmove(Hw, LINES - 1, 0);
		waddstr(Hw, prompt);
		wrefresh(Hw);
		wait_for(' ');
		clearok(curscr, TRUE);
		wclear(Hw);
	    }
	    Newpage = TRUE;
	    Line_cnt = 0;
	    maxlen = strlen(prompt);
	}
	if (fmt != NULL && !(Line_cnt == 0 && *fmt == '\0'))
	{
	    mvwprintw(Hw, Line_cnt++, 0, fmt, arg);
	    getyx(Hw, y, x);
	    if (maxlen < x)
		maxlen = x;
	    Lastfmt = fmt;
	    Lastarg = arg;
	}
    }
    return ~ESCAPE;
}

/*
 * end_line:
 *	End the list of lines
 */
end_line()
{
    register int dum;

    if (Inv_type != INV_SLOW)
	if (Line_cnt == 1 && !Newpage)
	{
	    Mpos = 0;
	    dum = msg(Lastfmt, Lastarg);
	}
	else
	    dum = add_line((char *) NULL);
    Line_cnt = 0;
    Newpage = FALSE;
    return dum;
}

/*
 * nothing:
 *	Set up Prbuf so that message for "nothing found" is there
 */
char *
nothing(type)
char type;
{
    register char *sp, *tystr;

    if (Terse)
	sprintf(Prbuf, "Nothing");
    else
	sprintf(Prbuf, "Haven't discovered anything");
    if (type != '*')
    {
	sp = &Prbuf[strlen(Prbuf)];
	switch (type)
	{
	    when POTION: tystr = "potion";
	    when SCROLL: tystr = "scroll";
	    when RING: tystr = "ring";
	    when STICK: tystr = "stick";
	}
	sprintf(sp, " about any %ss", tystr);
    }
    return Prbuf;
}

/*
 * nameit:
 *	Give the proper name to a potion, stick, or ring
 */
nameit(obj, type, which, op, prfunc)
register THING *obj;
char *type;
char *which;
register struct obj_info *op;
char *(*prfunc)();
{
    register char *pb;

    if (op->oi_know || op->oi_guess)
    {
	if (obj->o_count == 1)
	    sprintf(Prbuf, "A %s ", type);
	else
	    sprintf(Prbuf, "%d %ss ", obj->o_count, type);
	pb = &Prbuf[strlen(Prbuf)];
	if (op->oi_know)
	    sprintf(pb, "of %s%s(%s)", op->oi_name, (*prfunc)(obj), which);
	else if (op->oi_guess)
	    sprintf(pb, "called %s%s(%s)", op->oi_guess, (*prfunc)(obj), which);
    }
    else if (obj->o_count == 1)
	sprintf(Prbuf, "A%s %s %s", vowelstr(which), which, type);
    else
	sprintf(Prbuf, "%d %s %ss", obj->o_count, which, type);
}

/*
 * nullstr:
 *	Return a pointer to a null-length string
 */
char *
nullstr(ignored)
THING *ignored;
{
    return "";
}

#ifdef MASTER
/*
 * pr_list:
 *	List possible potions, scrolls, etc. for wizard.
 */
pr_list()
{
    int ch;

    if (!Terse)
	addmsg("for ");
    addmsg("what type");
    if (!Terse)
	addmsg(" of object do you want a list");
    msg("? ");
    ch = readchar();
    switch (ch)
    {
	when POTION:
	    pr_spec(Pot_info, MAXPOTIONS);
	when SCROLL:
	    pr_spec(Scr_info, MAXSCROLLS);
	when RING:
	    pr_spec(Ring_info, MAXRINGS);
	when STICK:
	    pr_spec(Ws_info, MAXSTICKS);
	when ARMOR:
	    pr_spec(Arm_info, MAXARMORS);
	when WEAPON:
	    pr_spec(Weap_info, MAXWEAPONS);
	otherwise:
	    return;
    }
}

/*
 * pr_spec:
 *	Print specific list of possible items to choose from
 */
pr_spec(info, nitems)
struct obj_info *info;
int nitems;
{
    struct obj_info *endp;
    int i, lastprob;

    endp = &info[nitems];
    lastprob = 0;
    for (i = '0'; info < endp; i++)
    {
	if (i == '9' + 1)
	    i = 'a';
	sprintf(Prbuf, "%c: %%s (%d%%%%)", i, info->oi_prob - lastprob);
	lastprob = info->oi_prob;
	add_line(Prbuf, info->oi_name);
	info++;
    }
    end_line();
}
#endif
