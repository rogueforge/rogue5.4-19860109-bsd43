/*
 * Hero movement commands
 *
 * @(#)move.c	X.XX (Berkeley) XX/XX/XX
 */

#include <curses.h>
#include <ctype.h>
#include "rogue.h"

/*
 * Used to hold the new Hero position
 */
char *Tr_mystery[] = {
    "you are suddenly in a parallel dimension",
    "the light in here suddenly seems %s",
    "you feel a sting in the side of your neck",
    "multi-colored lines swirl around you, then fade",
    "a %s light flashes in your eyes",
    "a spike shoots past your ear!",
    "%s sparks dance across your armor",
    "you suddenly feel very thirsty",
    "you feel time speed up suddenly",
    "time now seems to be going slower",
    "you pack turns %s!",
};

coord nh;

/*
 * do_run:
 *	Start the hero running
 */
do_run(ch)
char ch;
{
    Running = TRUE;
    After = FALSE;
    Runch = ch;
}

/*
 * do_move:
 *	Check to see that a move is legal.  If it is handle the
 * consequences (fighting, picking up, etc.)
 */
do_move(dy, dx)
int dy, dx;
{
    char ch, fl, huh;

    Firstmove = FALSE;
    if (No_move)
    {
	No_move--;
	msg("you are still stuck in the bear trap");
	return;
    }
    /*
     * Do a confused move (maybe)
     */
    if (on(Player, ISHUH) && rnd(5) != 0)
    {
	nh = *rndmove(&Player);
	if (nh.y != Hero.y + dy || nh.x != Hero.x + dx)
	    To_death = FALSE;
	if (ce(nh, Hero))
	    return;
	huh = TRUE;
    }
    else
    {
over:
	nh.y = Hero.y + dy;
	nh.x = Hero.x + dx;
	huh = FALSE;
    }

    /*
     * Check if he tried to move off the screen or make an illegal
     * diagonal move, and stop him if he did.
     */
    if (nh.x < 0 || nh.x >= NUMCOLS || nh.y <= 0 || nh.y >= NUMLINES - 1)
	goto hit_bound;
    if (!diag_ok(&Hero, &nh))
    {
	Running = After = FALSE;
	return;
    }
    fl = flat(nh.y, nh.x);
    ch = winat(nh.y, nh.x);
    if (on(Player, ISHELD) && ch != 'F')
    {
	msg("you are being held");
	To_death = FALSE;
	return;
    }
    else if (!(fl & F_REAL) && ch == FLOOR)
    {
	if (!on(Player, ISLEVIT))
	{
	    chat(nh.y, nh.x) = ch = TRAP;
	    flat(nh.y, nh.x) |= F_REAL;
	}
    }
    switch (ch)
    {
	case ' ':
	case '|':
	case '-':
hit_bound:
	    if (Passgo && Running && (Proom->r_flags & ISGONE)
		&& !on(Player, ISBLIND))
	    {
		bool	b1, b2;

		switch (Runch)
		{
		    case 'h':
		    case 'l':
			b1 = (Hero.y != 1 && turn_ok(Hero.y - 1, Hero.x));
			b2 = (Hero.y != NUMLINES - 2 && turn_ok(Hero.y + 1, Hero.x));
			if (!(b1 ^ b2))
			    break;
			if (b1)
			{
			    Move_dir = Runch = 'k';
			    dy = -1;
			}
			else
			{
			    Move_dir = Runch = 'j';
			    dy = 1;
			}
			dx = 0;
			turnref();
			goto over;
		    case 'j':
		    case 'k':
			b1 = (Hero.x != 0 && turn_ok(Hero.y, Hero.x - 1));
			b2 = (Hero.x != NUMCOLS - 1 && turn_ok(Hero.y, Hero.x + 1));
			if (!(b1 ^ b2))
			    break;
			if (b1)
			{
			    Move_dir = Runch = 'h';
			    dx = -1;
			}
			else
			{
			    Move_dir = Runch = 'l';
			    dx = 1;
			}
			dy = 0;
			turnref();
			goto over;
		}
	    }
	    Running = FALSE;
	    After = FALSE;
	    break;
	case DOOR:
	    Running = FALSE;
	    if (flat(Hero.y, Hero.x) & F_PASS)
	    {
		static coord saved;
		saved = Hero;
		Hero = nh;
		enter_room(&nh, TRUE);
		Hero = saved;
	    }
	    goto move_stuff;
	case TRAP:
	    ch = be_trapped(&nh);
	    if (ch == T_DOOR || ch == T_TELEP)
		return;
	    goto move_stuff;
	case PASSAGE:
	    /*
	     * when you're in a corridor, you don't know if you're in
	     * a maze room or not, and there ain't no way to find out
	     * if you're leaving a maze room, so it is necessary to
	     * always recalculate Proom.
	     */
	    Proom = roomin(&Hero);
	    goto move_stuff;
	case FLOOR:
	    if (!(fl & F_REAL))
		be_trapped(&Hero);
	    goto move_stuff;
	case STAIRS:
	    Seenstairs = TRUE;
	    /* FALLTHROUGH */
	default:
	    Running = FALSE;
	    if (isupper(ch) || moat(nh.y, nh.x))
		fight(&nh, Cur_weapon, FALSE);
	    else
	    {
		if (ch != STAIRS)
		    Take = ch;
move_stuff:
		mvaddch(Hero.y, Hero.x, floor_at());
		Hero = nh;
		if (chat(Oldpos.y, Oldpos.x) == DOOR)
		{
		    if (fl & F_PASS)
			leave_room(&nh);
		    else
			show_room(&nh);
		}
	    }
    }
}

/*
 * turn_ok:
 *	Decide whether it is legal to turn onto the given space
 */
turn_ok(y, x)
int y, x;
{
    register PLACE *pp;

    pp = INDEX(y, x);
    return (pp->p_ch == DOOR
	|| (pp->p_flags & (F_REAL|F_PASS)) == (F_REAL|F_PASS));
}

/*
 * turnref:
 *	Decide whether to refresh at a passage turning or not
 */
turnref()
{
    register PLACE *pp;

    pp = INDEX(Hero.y, Hero.x);
    if (!(pp->p_flags & F_SEEN))
    {
	if (Jump)
	{
	    leaveok(stdscr, TRUE);
	    refresh();
	    leaveok(stdscr, FALSE);
	}
	pp->p_flags |= F_SEEN;
    }
}

/*
 * door_open:
 *	Called to illuminate a room.  If it is dark, remove anything
 *	that might move.
 */
door_open(rp)
register struct room *rp;
{
    register int y, x;

    if (!(rp->r_flags & ISGONE))
	for (y = rp->r_pos.y; y < rp->r_pos.y + rp->r_max.y; y++)
	    for (x = rp->r_pos.x; x < rp->r_pos.x + rp->r_max.x; x++)
		if (isupper(winat(y, x)))
		    wake_monster(y, x);
}

/*
 * be_trapped:
 *	The guy stepped on a trap.... Make him pay.
 */
be_trapped(tc)
register coord *tc;
{
    register PLACE *pp;
    register THING *arrow;
    char tr, was_halu;

    if (on(Player, ISLEVIT))
	return T_RUST;	/* anything that's not a door or teleport */
    Running = FALSE;
    Count = FALSE;
    pp = INDEX(tc->y, tc->x);
    pp->p_ch = TRAP;
    tr = pp->p_flags & F_TMASK;
    pp->p_flags |= F_SEEN;
    switch (tr)
    {
	when T_DOOR:
	    Level++;
	    new_level();
	    msg("you fell into a trap!");
	when T_BEAR:
	    No_move += BEARTIME;
	    msg("you are caught in a bear trap");
	when T_SLEEP:
	    No_command += SLEEPTIME;
	    Player.t_flags &= ~ISRUN;
	    msg("a strange white mist envelops you and you fall asleep");
	when T_ARROW:
	    if (swing(Pstats.s_lvl - 1, Pstats.s_arm, 1))
	    {
		Pstats.s_hpt -= roll(1, 6);
		if (Pstats.s_hpt <= 0)
		{
		    msg("an arrow killed you");
		    death('a');
		}
		else
		    msg("oh no! An arrow shot you");
	    }
	    else
	    {
		arrow = new_item();
		init_weapon(arrow, ARROW);
		arrow->o_count = 1;
		arrow->o_pos = Hero;
		fall(arrow, FALSE);
		msg("an arrow shoots past you");
	    }
	when T_TELEP:
	    /*
	     * since the hero's leaving, look() won't put a TRAP
	     * down for us, so we have to do it ourself
	     */
	    teleport();
	    mvaddch(tc->y, tc->x, TRAP);
	when T_DART:
	    if (!swing(Pstats.s_lvl+1, Pstats.s_arm, 1))
		msg("a small dart whizzes by your ear and vanishes");
	    else
	    {
		Pstats.s_hpt -= roll(1, 4);
		if (Pstats.s_hpt <= 0)
		{
		    msg("a poisoned dart killed you");
		    death('d');
		}
		if (!ISWEARING(R_SUSTSTR) && !save(VS_POISON))
		    chg_str(-1);
		msg("a small dart just hit you in the shoulder");
	    }
	when T_RUST:
	    msg("a gush of water hits you on the head");
	    rust_armor(Cur_armor);
	when T_MYSTERY:
	    was_halu = on(Player, ISHALU);
	    if (!was_halu)
		Player.t_flags |= ISHALU;
	    msg(Tr_mystery[rnd(11)], pick_color("blue"));
	    if (!was_halu)
		Player.t_flags &= ~ISHALU;
    }
    flush_type();
    return tr;
}

/*
 * rndmove:
 *	Move in a random direction if the monster/person is confused
 */
coord *
rndmove(who)
THING *who;
{
    register THING *obj;
    register int x, y;
    char ch;
    static coord ret;  /* what we will be returning */

    y = ret.y = who->t_pos.y + rnd(3) - 1;
    x = ret.x = who->t_pos.x + rnd(3) - 1;
    /*
     * Now check to see if that's a legal move.  If not, don't move.
     * (I.e., bump into the wall or whatever)
     */
    if (y == who->t_pos.y && x == who->t_pos.x)
	return &ret;
    if (who == &Player)
	return &ret;
    if (!diag_ok(&who->t_pos, &ret))
	goto bad;
    ch = winat(y, x);
    if (!step_ok(ch))
	ret = who->t_pos;
    if (ch == SCROLL)
    {
	for (obj = Lvl_obj; obj != NULL; obj = next(obj))
	    if (y == obj->o_pos.y && x == obj->o_pos.x)
		break;
	if (obj != NULL && obj->o_which == S_SCARE)
	    goto bad;
    }
    return &ret;

bad:
    ret = who->t_pos;
    return &ret;
}

/*
 * rust_armor:
 *	Rust the given armor, if it is a legal kind to rust, and we
 *	aren't wearing a magic ring.
 */
rust_armor(arm)
register THING *arm;
{
    if (arm == NULL || arm->o_type != ARMOR || arm->o_which == LEATHER ||
	arm->o_arm >= 9)
	    return;

    if ((arm->o_flags & ISPROT) || ISWEARING(R_SUSTARM))
    {
	if (!To_death)
	    msg("the rust vanishes instantly");
    }
    else
    {
	arm->o_arm++;
	if (!Terse)
	    msg("your armor appears to be weaker now. Oh my!");
	else
	    msg("your armor weakens");
    }
}
