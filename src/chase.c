/*
 * Code for one creature to chase another
 *
 * @(#)chase.c	X.XX (Berkeley) XX/XX/XX
 */

#include <curses.h>
#include "rogue.h"

#define DRAGONSHOT  5	/* one chance in DRAGONSHOT that a dragon will flame */

static coord Ch_ret;				/* Where chasing takes you */

/*
 * runners:
 *	Make all the running monsters move.
 */
runners()
{
    register THING *tp;
    register THING *ntp;
    bool wastarget;
    static coord orig_pos;

    for (tp = Mlist; tp != NULL; tp = ntp)
    {
	ntp = next(tp);
	if (!on(*tp, ISHELD) && on(*tp, ISRUN))
	{
	    orig_pos = tp->t_pos;
	    wastarget = on(*tp, ISTARGET);
	    if (!move_monst(tp))
		continue;
	    if (on(*tp, ISFLY) && dist_cp(&Hero, &tp->t_pos) >= 3)
		move_monst(tp);
	    if (wastarget && !ce(orig_pos, tp->t_pos))
	    {
		tp->t_flags &= ~ISTARGET;
		To_death = FALSE;
	    }
	}
    }
    if (Has_hit)
    {
	endmsg();
	Has_hit = FALSE;
    }
}

/*
 * move_monst:
 *	Execute a single turn of running for a monster
 */
move_monst(tp)
register THING *tp;
{
    if (!on(*tp, ISSLOW) || tp->t_turn)
	if (!do_chase(tp))
	    return FALSE;
    if (on(*tp, ISHASTE))
	if (!do_chase(tp))
	    return FALSE;
    tp->t_turn ^= TRUE;
    return TRUE;
}

/*
 * do_chase:
 *	Make one thing chase another.
 */
do_chase(th)
register THING *th;
{
    register coord *cp;
    register struct room *rer, *ree;	/* room of chaser, room of chasee */
    register int mindist = 32767, curdist;
    register bool stoprun = FALSE;	/* TRUE means we are there */
    register bool door;
    THING *obj;
    struct room *oroom;
    static coord this;			/* Temporary destination for chaser */

    rer = th->t_room;		/* Find room of chaser */
    if (on(*th, ISGREED) && rer->r_goldval == 0)
	th->t_dest = &Hero;	/* If gold has been taken, run after hero */
    if (th->t_dest == &Hero)	/* Find room of chasee */
	ree = Proom;
    else
	ree = roomin(th->t_dest);
    /*
     * We don't count doors as inside rooms for this routine
     */
    door = (chat(th->t_pos.y, th->t_pos.x) == DOOR);
    /*
     * If the object of our desire is in a different room,
     * and we are not in a corridor, run to the door nearest to
     * our goal.
     */
over:
    if (rer != ree)
    {
	for (cp = rer->r_exit; cp < &rer->r_exit[rer->r_nexits]; cp++)
	{
	    curdist = dist_cp(th->t_dest, cp);
	    if (curdist < mindist)
	    {
		this = *cp;
		mindist = curdist;
	    }
	}
	if (door)
	{
	    rer = &Passages[flat(th->t_pos.y, th->t_pos.x) & F_PNUM];
	    door = FALSE;
	    goto over;
	}
    }
    else
    {
	this = *th->t_dest;
	/*
	 * For dragons check and see if (a) the hero is on a straight
	 * line from it, and (b) that it is within shooting distance,
	 * but outside of striking range.
	 */
	if (th->t_type == 'D' && (th->t_pos.y == Hero.y || th->t_pos.x == Hero.x
	    || abs(th->t_pos.y - Hero.y) == abs(th->t_pos.x - Hero.x))
	    && dist_cp(&th->t_pos, &Hero) <= BOLT_LENGTH * BOLT_LENGTH
	    && !on(*th, ISCANC) && rnd(DRAGONSHOT) == 0)
	{
	    Delta.y = sign(Hero.y - th->t_pos.y);
	    Delta.x = sign(Hero.x - th->t_pos.x);
	    if (Has_hit)
		endmsg();
	    fire_bolt(&th->t_pos, &Delta, "flame");
	    Running = FALSE;
	    Count = 0;
	    Quiet = 0;
	    if (To_death && !on(*th, ISTARGET))
	    {
		To_death = FALSE;
		Kamikaze = FALSE;
	    }
	    return TRUE;
	}
    }
    /*
     * This now contains what we want to run to this time
     * so we run to it.  If we hit it we either want to fight it
     * or stop running
     */
    if (!chase(th, &this))
    {
	if (ce(this, Hero))
	    return attack(th);
	else if (ce(this, *th->t_dest))
	{
	    for (obj = Lvl_obj; obj != NULL; obj = next(obj))
		if (th->t_dest == &obj->o_pos)
		{
		    detach(Lvl_obj, obj);
		    attach(th->t_pack, obj);
		    chat(obj->o_pos.y, obj->o_pos.x) =
			(th->t_room->r_flags & ISGONE) ? PASSAGE : FLOOR;
		    th->t_dest = find_dest(th);
		    break;
		}
	    if (th->t_type != 'F')
		stoprun = TRUE;
	}
    }
    else
    {
	if (th->t_type == 'F')
	    return TRUE;
    }
    if (!ce(Ch_ret, th->t_pos))
    {
	mvaddch(th->t_pos.y, th->t_pos.x, th->t_oldch);
	set_oldch(th, &Ch_ret);
	oroom = th->t_room;
	th->t_room = roomin(&Ch_ret);
	if (oroom != th->t_room)
	    th->t_dest = find_dest(th);

	moat(th->t_pos.y, th->t_pos.x) = NULL;
	moat(Ch_ret.y, Ch_ret.x) = th;
	th->t_pos = Ch_ret;
    }
    move(Ch_ret.y, Ch_ret.x);
    if (see_monst(th))
	addch(th->t_disguise);
    else if (on(Player, SEEMONST))
    {
	standout();
	addch(th->t_type);
	standend();
    }
    /*
     * And stop running if need be
     */
    if (stoprun && ce(th->t_pos, *(th->t_dest)))
	th->t_flags &= ~ISRUN;
    return TRUE;
}

/*
 * set_oldch:
 *	Set the oldch character for the monster
 */
set_oldch(tp, cp)
register THING *tp;
register coord *cp;
{
    register int sch;

    move(cp->y, cp->x);
    sch = tp->t_oldch;
    tp->t_oldch = inch();
    if (!on(Player, ISBLIND))
	if ((sch == FLOOR || tp->t_oldch == FLOOR) &&
	    (tp->t_room->r_flags & ISDARK))
		tp->t_oldch = ' ';
	else if (See_floor && dist_cp(cp, &Hero) <= LAMPDIST)
	    tp->t_oldch = chat(Ch_ret.y, Ch_ret.x);
}

/*
 * see_monst:
 *	Return TRUE if the hero can see the monster
 */
see_monst(mp)
register THING *mp;
{
    register int y, x;

    if (on(Player, ISBLIND))
	return FALSE;
    if (on(*mp, ISINVIS) && !on(Player, CANSEE))
	return FALSE;
    y = mp->t_pos.y;
    x = mp->t_pos.x;
    if (dist(y, x, Hero.y, Hero.x) < LAMPDIST)
    {
	if (y != Hero.y && x != Hero.x &&
	    !step_ok(chat(y, Hero.x)) && !step_ok(chat(Hero.y, x)))
		return FALSE;
	return TRUE;
    }
    if (mp->t_room != Proom)
	return FALSE;
    if (chat(Hero.y, Hero.x) == DOOR)
	if (!door_look(y, x, &Hero))
	    return FALSE;
    return (!(mp->t_room->r_flags & ISDARK));
}

/*
 * runto:
 *	Set a monster running after the hero.
 */
runto(runner)
register coord *runner;
{
    register THING *tp;

    /*
     * If we couldn't find him, something is funny
     */
#ifdef MASTER
    if ((tp = moat(runner->y, runner->x)) == NULL)
	msg("couldn't find monster in runto at (%d,%d)", runner->y, runner->x);
#else
    tp = moat(runner->y, runner->x);
#endif
    /*
     * Start the beastie running
     */
    tp->t_flags |= ISRUN;
    tp->t_flags &= ~ISHELD;
    tp->t_dest = find_dest(tp);
}

/*
 * chase:
 *	Find the spot for the chaser(er) to move closer to the
 *	chasee(ee).  Returns TRUE if we want to keep on chasing later
 *	FALSE if we reach the goal.
 */
chase(tp, ee)
THING *tp;
coord *ee;
{
    register THING *obj;
    register int x, y;
    register int curdist, thisdist;
    register coord *er = &tp->t_pos;
    char ch;
    int plcnt = 1;
    static coord tryp;

    /*
     * If the thing is confused, let it move randomly. Invisible
     * Stalkers are slightly confused all of the time, and bats are
     * quite confused all the time
     */
    if ((on(*tp, ISHUH) && rnd(5) != 0) || (tp->t_type == 'P' && rnd(5) == 0)
	|| (tp->t_type == 'B' && rnd(2) == 0))
    {
	/*
	 * get a valid random move
	 */
	Ch_ret = *rndmove(tp);
	curdist = dist_cp(&Ch_ret, ee);
	/*
	 * Small chance that it will become un-confused
	 */
	if (rnd(20) == 0)
	    tp->t_flags &= ~ISHUH;
    }
    /*
     * Otherwise, find the empty spot next to the chaser that is
     * closest to the chasee.
     */
    else
    {
	register int ey, ex;
	/*
	 * This will eventually hold where we move to get closer
	 * If we can't find an empty spot, we stay where we are.
	 */
	curdist = dist_cp(er, ee);
	Ch_ret = *er;

	ey = er->y + 1;
	if (ey >= NUMLINES - 1)
	    ey = NUMLINES - 2;
	ex = er->x + 1;
	if (ex >= NUMCOLS)
	    ex = NUMCOLS - 1;

	for (x = er->x - 1; x <= ex; x++)
	{
	    if (x < 0)
		continue;
	    tryp.x = x;
	    for (y = er->y - 1; y <= ey; y++)
	    {
		tryp.y = y;
		if (!diag_ok(er, &tryp))
		    continue;
		ch = winat(y, x);
		if (step_ok(ch))
		{
		    /*
		     * If it is a scroll, it might be a scare monster scroll
		     * so we need to look it up to see what type it is.
		     */
		    if (ch == SCROLL)
		    {
			for (obj = Lvl_obj; obj != NULL; obj = next(obj))
			{
			    if (y == obj->o_pos.y && x == obj->o_pos.x)
				break;
			}
			if (obj != NULL && obj->o_which == S_SCARE)
			    continue;
		    }
		    /*
		     * It can also be a Xeroc, which we shouldn't step on
		     */
		    if ((obj = moat(y, x)) != NULL && obj->t_type == 'X')
			continue;
		    /*
		     * If we didn't find any scrolls at this place or it
		     * wasn't a scare scroll, then this place counts
		     */
		    thisdist = dist(y, x, ee->y, ee->x);
		    if (thisdist < curdist)
		    {
			plcnt = 1;
			Ch_ret = tryp;
			curdist = thisdist;
		    }
		    else if (thisdist == curdist && rnd(++plcnt) == 0)
		    {
			Ch_ret = tryp;
			curdist = thisdist;
		    }
		}
	    }
	}
    }
    return (curdist != 0 && !ce(Ch_ret, Hero));
}

/*
 * roomin:
 *	Find what room some coordinates are in. NULL means they aren't
 *	in any room.
 */
struct room *
roomin(cp)
register coord *cp;
{
    register struct room *rp;
    register char *fp;

    fp = &flat(cp->y, cp->x);
    if (*fp & F_PASS)
	return &Passages[*fp & F_PNUM];

    for (rp = Rooms; rp < &Rooms[MAXROOMS]; rp++)
	if (cp->x <= rp->r_pos.x + rp->r_max.x && rp->r_pos.x <= cp->x
	 && cp->y <= rp->r_pos.y + rp->r_max.y && rp->r_pos.y <= cp->y)
	    return rp;

    msg("in some bizarre place (%d, %d)", unc(*cp));
#ifdef MASTER
    abort();
    /* NOTREACHED */
#else
    return NULL;
#endif
}

/*
 * diag_ok:
 *	Check to see if the move is legal if it is diagonal
 */
diag_ok(sp, ep)
register coord *sp, *ep;
{
    if (ep->x < 0 || ep->x >= NUMCOLS || ep->y <= 0 || ep->y >= NUMLINES - 1)
	return FALSE;
    if (ep->x == sp->x || ep->y == sp->y)
	return TRUE;
    return (step_ok(chat(ep->y, sp->x)) && step_ok(chat(sp->y, ep->x)));
}

/*
 * cansee:
 *	Returns true if the hero can see a certain coordinate.
 */
cansee(y, x)
register int y, x;
{
    register struct room *rer;
    static coord tp;

    if (on(Player, ISBLIND))
	return FALSE;
    if (dist(y, x, Hero.y, Hero.x) < LAMPDIST)
    {
	if (flat(y, x) & F_PASS)
	    if (y != Hero.y && x != Hero.x &&
		!step_ok(chat(y, Hero.x)) && !step_ok(chat(Hero.y, x)))
		    return FALSE;
	return TRUE;
    }
    /*
     * We can only see if the hero in the same room as
     * the coordinate and the room is lit or if it is close.
     */
    tp.y = y;
    tp.x = x;
    if (chat(Hero.y, Hero.x) == DOOR)
	if (!door_look(y, x, &Hero))
	    return FALSE;
    return ((rer = roomin(&tp)) == Proom && !(rer->r_flags & ISDARK));
}

/*
 * find_dest:
 *	find the proper destination for the monster
 */
coord *
find_dest(tp)
register THING *tp;
{
    register THING *obj;
    register int prob;

    if ((prob = Monsters[tp->t_type - 'A'].m_carry) <= 0 || tp->t_room == Proom
	|| see_monst(tp))
	    return &Hero;
    for (obj = Lvl_obj; obj != NULL; obj = next(obj))
    {
	if (obj->o_type == SCROLL && obj->o_which == S_SCARE)
	    continue;
	if (roomin(&obj->o_pos) == tp->t_room && rnd(100) < prob)
	{
	    for (tp = Mlist; tp != NULL; tp = next(tp))
		if (tp->t_dest == &obj->o_pos)
		    break;
	    if (tp == NULL)
		return &obj->o_pos;
	}
    }
    return &Hero;
}

/*
 * dist:
 *	Calculate the "distance" between to points.  Actually,
 *	this calculates d^2, not d, but that's good enough for
 *	our purposes, since it's only used comparitively.
 */
dist(y1, x1, y2, x2)
int y1, x1, y2, x2;
{
    register int dy, dx;

    dy = y2 - y1;
    dx = x2 - x1;
    return (dx * dx + dy * dy);
}

/*
 * dist_cp:
 *	Call dist() with appropriate arguments for coord pointers
 */
dist_cp(c1, c2)
register coord *c1, *c2;
{
    return dist(c1->y, c1->x, c2->y, c2->x);
}

/*
 * door_look:
 *	Check directional visibility from a doorway.
 *	Returns TRUE if the coordinate (y, x) is visible from
 *	the hero's position based on the current movement direction.
 */
door_look(y, x, cp)
int y, x;
register coord *cp;
{
    switch (Move_dir)
    {
	case 'h':
	case 'l':
	    return (abs(y - cp->y) <= abs(x - cp->x));
	case 'j':
	case 'k':
	    return (abs(x - cp->x) <= abs(y - cp->y));
	default:
	    return TRUE;
    }
}
