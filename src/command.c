/*
 * Read and execute the user commands
 *
 * @(#)command.c	X.XX (Berkeley) XX/XX/XX
 */

#include <curses.h>
#include <ctype.h>
#include "rogue.h"

/*
 * command:
 *	Process the user commands
 */
command()
{
    register int ntimes = 1;			/* Number of player moves */
    register char ch;
    register char *fp;
    register THING *mp;
    static char countch, direction, newcount = FALSE;

    if (on(Player, ISHASTE))
	ntimes++;
    /*
     * Let the daemons start up
     */
    do_daemons(BEFORE);
    do_fuses(BEFORE);
    while (ntimes--)
    {
	Again = FALSE;
	if (Has_hit)
	{
	    endmsg();
	    Has_hit = FALSE;
	}
	/*
	 * these are illegal things for the player to be, so if any are
	 * set, someone's been poking in memeory
	 */
	if (on(Player, ISSLOW|ISGREED|ISINVIS|ISREGEN|ISTARGET))
	    exit(1);

	look(TRUE);
	if (!Running)
	    Door_stop = FALSE;
	status();
	Lastscore = Purse;
	move(Hero.y, Hero.x);
	if (!((Running || Count) && Jump))
	    refresh();			/* Draw screen */
	Take = 0;
	After = TRUE;
	/*
	 * Read command or continue run
	 */
#ifdef MASTER
	if (Wizard)
	    Noscore = TRUE;
#endif
	if (!No_command)
	{
	    if (Running || To_death)
		ch = Runch;
	    else if (Count)
		ch = countch;
	    else
	    {
		ch = readchar();
		Move_on = FALSE;
		if (Mpos != 0)		/* Erase message if its there */
		    msg("");
	    }
	}
	else
	    ch = '.';
	if (No_command)
	{
	    if (--No_command == 0)
	    {
		Player.t_flags |= ISRUN;
		msg("you can move again");
	    }
	}
	else
	{
	    /*
	     * check for prefixes
	     */
	    newcount = FALSE;
	    if (isdigit(ch))
	    {
		Count = 0;
		newcount = TRUE;
		while (isdigit(ch))
		{
		    Count = Count * 10 + (ch - '0');
		    ch = readchar();
		}
		countch = ch;
		/*
		 * turn off Count for commands which don't make sense
		 * to repeat
		 */
		switch (ch)
		{
		    case CTRL(B): case CTRL(H): case CTRL(J):
		    case CTRL(K): case CTRL(L): case CTRL(N):
		    case CTRL(U): case CTRL(Y):
		    case '.': case 'a': case 'b': case 'h': case 'j':
		    case 'k': case 'l': case 'm': case 'n': case 'q':
		    case 'r': case 's': case 't': case 'u': case 'y':
		    case 'z': case 'B': case 'C': case 'H': case 'I':
		    case 'J': case 'K': case 'L': case 'N': case 'U':
		    case 'Y':
#ifdef MASTER
		    case CTRL(D): case CTRL(A):
#endif
			break;
		    default:
			Count = 0;
		}
	    }
	    /*
	     * execute a command
	     */
	    if (Count && !Running)
		Count--;
	    /*
	     * Save last command for 'a' (again)
	     */
	    if (ch != 'a' && ch != ESCAPE && !(Running || Count || To_death || No_command))
	    {

		L_last_comm = Last_comm;
		L_last_dir = Last_dir;
		L_last_pick = Last_pick;
		Last_comm = ch;
		Last_dir = '\0';
		Last_pick = NULL;
	    }
over:
	    switch (ch)
	    {
		when '!': shell();
		when 'h': Move_dir = 'h'; do_move(0, -1);
		when 'j': Move_dir = 'j'; do_move(1, 0);
		when 'k': Move_dir = 'k'; do_move(-1, 0);
		when 'l': Move_dir = 'l'; do_move(0, 1);
		when 'y': Move_dir = 'y'; do_move(-1, -1);
		when 'u': Move_dir = 'u'; do_move(-1, 1);
		when 'b': Move_dir = 'b'; do_move(1, -1);
		when 'n': Move_dir = 'n'; do_move(1, 1);
		when 'H': do_run('h');
		when 'J': do_run('j');
		when 'K': do_run('k');
		when 'L': do_run('l');
		when 'Y': do_run('y');
		when 'U': do_run('u');
		when 'B': do_run('b');
		when 'N': do_run('n');
		when CTRL(H): case CTRL(J): case CTRL(K): case CTRL(L):
		case CTRL(Y): case CTRL(U): case CTRL(B): case CTRL(N):
		{
		    if (!on(Player, ISBLIND))
		    {
			Door_stop = TRUE;
			Firstmove = TRUE;
		    }
		    if (Count && !newcount)
			ch = direction;
		    else
		    {
			ch += ('A' - CTRL(A));
			direction = ch;
		    }
		    goto over;
		}
		when 'F':
		    Kamikaze = TRUE;
		    /* FALLTHROUGH */
		case 'f':
		    if (!get_dir())
		    {
			After = FALSE;
			break;
		    }
		    Delta.y += Hero.y;
		    Delta.x += Hero.x;
		    if ((mp = moat(Delta.y, Delta.x)) == NULL
			|| !see_monst(mp) && !on(Player, SEEMONST))
		    {
			if (!Terse)
			    addmsg("I see ");
			msg("no monster there");
			After = FALSE;
		    }
		    else if (diag_ok(&Hero, &Delta))
		    {
			To_death = TRUE;
			Max_hit = 0;
			mp->t_flags |= ISTARGET;
			Runch = ch = Dir_ch;
			goto over;
		    }
		when 't':
		    if (!get_dir())
			After = FALSE;
		    else
			missile(Delta.y, Delta.x);
		when 'a':
		    if (Last_comm == '\0')
		    {
			msg("you haven't typed a command yet");
			After = FALSE;
		    }
		    else
		    {
			ch = Last_comm;
			Again = TRUE;
			goto over;
		    }
		when 'q': quaff();
		when 'Q':
		    After = FALSE;
		    Q_comm = TRUE;
		    quit();
		    Q_comm = FALSE;
		when 'i': After = FALSE; inventory(Pack, 0);
		when 'I': After = FALSE; picky_inven();
		when 'd': drop();
		when 'r': read_scroll();
		when 'e': eat();
		when 'w': wield();
		when 'W': wear();
		when 'T': take_off();
		when 'P': ring_on();
		when 'R': ring_off();
		when 'o': option(); After = FALSE;
		when 'c': call(); After = FALSE;
		when '>': After = FALSE; d_level();
		when '<': After = FALSE; u_level();
		when '?': After = FALSE; help();
		when '/': After = FALSE; identify();
		when 's': search();
		when 'z':
		    if (get_dir())
			do_zap();
		    else
			After = FALSE;
		when 'D': After = FALSE; discovered();
		when CTRL(P): After = FALSE; msg(Huh);
		when CTRL(R):
		    After = FALSE;
		    clearok(curscr,TRUE);
		    wrefresh(curscr);
		when 'v':
		    After = FALSE;
		    msg("version %s. (mctesq was here)", Release);
		when 'S':
		    After = FALSE;
		    save_game();
		when ',':
		    Take = chat(Hero.y, Hero.x);
		when '^':
		    After = FALSE;
		    if (get_dir()) {
			Delta.y += Hero.y;
			Delta.x += Hero.x;
			fp = &flat(Delta.y, Delta.x);
			if (chat(Delta.y, Delta.x) != TRAP)
			{
			    if (!Terse)
				addmsg("You have found ");
			    msg("no trap there");
			}
			else if (on(Player, ISHALU))
			    msg(Tr_name[rnd(NTRAPS)]);
			else {
			    msg(Tr_name[*fp & F_TMASK]);
			    *fp |= F_SEEN;
			}
		    }
#ifdef MASTER
		when '+':
		    After = FALSE;
		    if (Wizard)
		    {
			Wizard = FALSE;
			turn_see(TRUE);
			msg("not wizard any more");
		    }
		    else
		    {
			if (Wizard = passwd())
			{
			    Noscore = TRUE;
			    turn_see(FALSE);
			    msg("you are suddenly as smart as Ken Arnold in dungeon #%d", Dnum);
			}
			else
			    msg("sorry");
		    }
#endif
		when ESCAPE:	/* Escape */
		    Door_stop = FALSE;
		    Count = 0;
		    After = FALSE;
		    Again = FALSE;
		when 'm':
		    Move_on = TRUE;
		    if (!get_dir())
			After = FALSE;
		    else
		    {
			ch = Dir_ch;
			countch = Dir_ch;
			goto over;
		    }
		when ')': current(Cur_weapon, "wielding", NULL);
		when ']': current(Cur_armor, "wearing", NULL);
		when '=':
		    current(Cur_ring[LEFT], "wearing",
				Terse ? "(L)" : "on left hand");
		    current(Cur_ring[RIGHT], "wearing",
				Terse ? "(R)" : "on right hand");
		when ' ': After = FALSE;	/* "Legal" illegal command */
		when '.': break;			/* Rest command */
		otherwise:
		    After = FALSE;
#ifdef MASTER
		    if (Wizard) switch (ch)
		    {
			when '|': msg("@ %d,%d", Hero.y, Hero.x);
			when 'C': create_obj();
			when '$': msg("Inpack = %d", Inpack);
			when CTRL(G): inventory(Lvl_obj, 0);
			when CTRL(W): whatis(FALSE, 0);
			when CTRL(D): Level++; new_level();
			when CTRL(A): Level--; new_level();
			when CTRL(F): show_map();
			when CTRL(T): teleport();
			when CTRL(E): msg("food left: %d", Food_left);
			when CTRL(C): add_pass();
			when CTRL(X): turn_see(on(Player, SEEMONST));
			when CTRL(~):
			{
			    THING *item;

			    if ((item = get_item("charge", STICK)) != NULL)
				item->o_charges = 10000;
			}
			when CTRL(I):
			{
			    int i;
			    THING *obj;

			    for (i = 0; i < 9; i++)
				raise_level();
			    /*
			     * Give him a sword (+1,+1)
			     */
			    obj = new_item();
			    init_weapon(obj, TWOSWORD);
			    obj->o_hplus = 1;
			    obj->o_dplus = 1;
			    add_pack(obj, TRUE);
			    Cur_weapon = obj;
			    /*
			     * And his suit of armor
			     */
			    obj = new_item();
			    obj->o_type = ARMOR;
			    obj->o_which = PLATE_MAIL;
			    obj->o_arm = -5;
			    obj->o_flags |= ISKNOW;
			    obj->o_count = 1;
			    obj->o_group = 0;
			    Cur_armor = obj;
			    add_pack(obj, TRUE);
			}
			when '*' :
			    pr_list();
			otherwise:
			    illcom(ch);
		    }
		    else
#endif
			illcom(ch); break;
		case '@':
		    Stat_msg = TRUE;
		    status();
		    Stat_msg = FALSE;
		    After = FALSE;
	    }
	    /*
	     * turn off flags if no longer needed
	     */
	    if (!Running)
		Door_stop = FALSE;
	}
	/*
	 * If he ran into something to take, let him pick it up.
	 */
	if (Take != 0)
	    pick_up(Take);
	if (!Running)
	    Door_stop = FALSE;
	if (!After)
	{
	    ntimes++;
	    continue;
	}
	if (ISRING(LEFT, R_SEARCH))
	    search();
	else if (ISRING(LEFT, R_TELEPORT) && rnd(50) == 0)
	    teleport();
	if (ISRING(RIGHT, R_SEARCH))
	    search();
	else if (ISRING(RIGHT, R_TELEPORT) && rnd(50) == 0)
	    teleport();
    }
    do_daemons(AFTER);
    do_fuses(AFTER);
}

/*
 * illcom:
 *	What to do with an illegal command
 */
illcom(ch)
char ch;
{
    Save_msg = FALSE;
    Count = 0;
    msg("illegal command '%s'", unctrl(ch));
    Save_msg = TRUE;
}

/*
 * search:
 *	Player gropes about him to find hidden things.
 */
search()
{
    register int y, x;
    register char *fp;
    register int ey, ex;
    register int probinc;
    char *found;

    ey = Hero.y + 1;
    ex = Hero.x + 1;
    probinc = 0;
    if (on(Player, ISBLIND))
	probinc = 25;
    if (on(Player, ISHALU))
	probinc += 3;
    found = NULL;
    for (y = Hero.y - 1; y <= ey; y++)
	for (x = Hero.x - 1; x <= ex; x++)
	{
	    if (y == Hero.y && x == Hero.x)
		continue;
	    fp = &chat(y, x);
	    if (!(fp[1] & F_REAL))
		switch (*fp)
		{
		    case '|':
		    case '-':
			if (rnd(5 + probinc) != 0)
			    break;
			*fp = DOOR;
			found = "a secret door";
foundone:
			if (on(Player, ISBLIND) || *fp == TRAP)
			{
			    wmove(stdscr, y, x);
			    waddch(stdscr, *fp);
			    if (!Terse)
				addmsg("you found ");
			    msg(found);
			}
			fp[1] |= F_REAL;
			Count = 0;
			Running = FALSE;
			break;
		    case FLOOR:
			if (rnd(2 + probinc) != 0)
			    break;
			*fp = TRAP;
			if (on(Player, ISHALU))
			{
			    found = Tr_name[rnd(NTRAPS)];
			}
			else
			{
			    found = Tr_name[fp[1] & F_TMASK];
			    fp[1] |= F_SEEN;
			}
			goto foundone;
			break;
		    case ' ':
			if (rnd(3 + probinc) != 0)
			    break;
			*fp = PASSAGE;
			goto foundone;
		}
	}
    if (found)
	look(FALSE);
}

/*
 * help:
 *	Give single character help, or the whole mess if he wants it
 */
help()
{
    register struct h_list *strp;
    char helpch;
    register unsigned int numprint, cnt;

    msg("character you want help for (* for all): ");
    helpch = readchar();
    Mpos = 0;
    /*
     * If its not a *, print the right help string
     * or an error if he typed a funny character.
     */
    if (helpch != '*')
    {
	move(0, 0);
	for (strp = Helpstr; strp->h_desc != NULL; strp++)
	    if (strp->h_ch == helpch)
	    {
		Lower_msg = TRUE;
		msg("%s%s", unctrl(strp->h_ch), strp->h_desc);
		Lower_msg = FALSE;
		return;
	    }
	msg("unknown character '%s'", unctrl(helpch));
	return;
    }
    /*
     * Here we print help for everything.
     * Then wait before we return to command mode
     */
    numprint = 0;
    for (strp = Helpstr; strp->h_desc != NULL; strp++)
	if (strp->h_print)
	    numprint++;
    if (numprint & 01)		/* round odd numbers up */
	numprint++;
    numprint >>= 1;
    if (numprint > LINES - 1)
	numprint = LINES - 1;

    wclear(Hw);
    cnt = 0;
    for (strp = Helpstr; strp->h_desc != NULL; strp++)
	if (strp->h_print)
	{
	    wmove(Hw, cnt % numprint, cnt >= numprint ? COLS / 2 : 0);
	    if (strp->h_ch)
		waddstr(Hw, unctrl(strp->h_ch));
	    waddstr(Hw, strp->h_desc);
	    if (++cnt >= numprint + numprint)
		break;
	}
    wmove(Hw, LINES - 1, 0);
    waddstr(Hw, "--Press space to continue--");
    wrefresh(Hw);
    wait_for(' ');
    clearok(stdscr, TRUE);
    wrefresh(stdscr);
}

/*
 * identify:
 *	Tell the player what a certain thing is.
 */
identify()
{
    register int ch;
    register struct h_list *hp;
    register char *str;
    static struct h_list ident_list[] = {
	'|',		"wall of a room",		FALSE,
	'-',		"wall of a room",		FALSE,
	GOLD,		"gold",				FALSE,
	STAIRS,		"a staircase",			FALSE,
	DOOR,		"door",				FALSE,
	FLOOR,		"room floor",			FALSE,
	PLAYER,		"you",				FALSE,
	PASSAGE,	"passage",			FALSE,
	TRAP,		"trap",				FALSE,
	POTION,		"potion",			FALSE,
	SCROLL,		"scroll",			FALSE,
	FOOD,		"food",				FALSE,
	WEAPON,		"weapon",			FALSE,
	' ',		"solid rock",			FALSE,
	ARMOR,		"armor",			FALSE,
	AMULET,		"the Amulet of Yendor",		FALSE,
	RING,		"ring",				FALSE,
	STICK,		"wand or staff",		FALSE,
	'\0'
    };

    msg("what do you want identified? ");
    ch = readchar();
    Mpos = 0;
    if (ch == ESCAPE)
    {
	msg("");
	return;
    }
    if (isupper(ch))
	str = Monsters[ch-'A'].m_name;
    else
    {
	str = "unknown character";
	for (hp = ident_list; hp->h_ch != '\0'; hp++)
	    if (hp->h_ch == ch)
	    {
		str = hp->h_desc;
		break;
	    }
    }
    msg("'%s': %s", unctrl(ch), str);
}

/*
 * d_level:
 *	He wants to go down a level
 */
d_level()
{
    if (levit_check())
	return;
    if (chat(Hero.y, Hero.x) != STAIRS)
	msg("I see no way down");
    else
    {
	Level++;
	Seenstairs = FALSE;
	new_level();
    }
}

/*
 * u_level:
 *	He wants to go up a level
 */
u_level()
{
    if (levit_check())
	return;
    if (chat(Hero.y, Hero.x) == STAIRS)
	if (Amulet)
	{
	    Level--;
	    if (Level == 0)
		total_winner();
	    new_level();
	    msg("you feel a wrenching sensation in your gut");
	}
	else
	    msg("your way is magically blocked");
    else
	msg("I see no way up");
}

/*
 * levit_check:
 *	Check to see if she's levitating, and if she is, print an
 *	appropriate message.
 */
levit_check()
{
    if (!on(Player, ISLEVIT))
	return FALSE;
    msg("You can't.  You're floating off the ground!");
    return TRUE;
}

/*
 * call:
 *	Allow a user to call a potion, scroll, or ring something
 */
call()
{
    register THING *obj;
    register struct obj_info *op;
    register char **guess, *elsewise;
    register bool *know;

    obj = get_item("call", CALLABLE);
    /*
     * Make certain that it is somethings that we want to wear
     */
    if (obj == NULL)
	return;
    switch (obj->o_type)
    {
	when RING:
	    op = &Ring_info[obj->o_which];
	    elsewise = R_stones[obj->o_which];
	    goto norm;
	when POTION:
	    op = &Pot_info[obj->o_which];
	    elsewise = P_colors[obj->o_which];
	    goto norm;
	when SCROLL:
	    op = &Scr_info[obj->o_which];
	    elsewise = S_names[obj->o_which];
	    goto norm;
	when STICK:
	    op = &Ws_info[obj->o_which];
	    elsewise = Ws_made[obj->o_which];
norm:
	    know = &op->oi_know;
	    guess = &op->oi_guess;
	    if (*guess != NULL)
		elsewise = *guess;
	when FOOD:
	    msg("you can't call that anything");
	    return;
	otherwise:
	    guess = &obj->o_label;
	    know = NULL;
	    elsewise = obj->o_label;
    }
    if (know != NULL && *know)
    {
	msg("that has already been identified");
	return;
    }
    if (elsewise && elsewise == op->oi_guess)
    {
	if (!Terse)
	    addmsg("Was ");
	msg("called \"%s\"", elsewise);
    }
    if (Terse)
	msg("call it: ");
    else
	msg("what do you want to call it? ");
    strcpy(Prbuf, elsewise);
    if (get_str(Prbuf, stdscr) == NORM)
    {
	if (*guess != NULL)
	    cfree(*guess);
	*guess = malloc((unsigned int) strlen(Prbuf) + 1);
	strcpy(*guess, Prbuf);
    }
}

/*
 * current:
 *	Print the current weapon/armor
 */
/* VARARGS2 */
current(cur, how, where)
register THING *cur;
register char *how;
register char *where;
{
    After = FALSE;
    if (cur != NULL)
    {
	if (!Terse)
	    addmsg("you are %s (", how);
	Inv_describe = FALSE;
	addmsg("%c) %s", cur->o_packch, inv_name(cur, TRUE));
	Inv_describe = TRUE;
	if (where)
	    addmsg(" %s", where);
	endmsg();
    }
    else
    {
	if (!Terse)
	    addmsg("you are ");
	addmsg("%s nothing", how);
	if (where)
	    addmsg(" %s", where);
	endmsg();
    }
}
