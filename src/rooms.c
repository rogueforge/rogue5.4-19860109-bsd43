/*
 * Create the layout for the new level
 *
 * @(#)rooms.c	X.XX (Berkeley) XX/XX/XX
 */

#include <ctype.h>
#include <curses.h>
#include "rogue.h"

typedef struct spot {		/* position matrix for maze positions */
	int	nexits;
	coord	exits[4];
	int	used;
} SPOT;

#define GOLDGRP 1

/*
 * do_rooms:
 *	Create rooms and corridors with a connectivity graph
 */
do_rooms()
{
    register int i;
    register struct room *rp;
    register THING *tp;
    register int left_out;
    static coord top;
    coord bsze;				/* maximum room size */
    coord mp;

    bsze.x = NUMCOLS / 3;
    bsze.y = NUMLINES / 3;
    /*
     * Clear things for a new level
     */
    bzero(Rooms, 1188);
    /*
     * Put the gone rooms, if any, on the level
     */
    left_out = rnd(4);
    for (i = 0; i < left_out; i++)
	Rooms[rnd_room()].r_flags |= ISGONE;
    /*
     * dig and populate all the rooms on the level
     */
    for (i = 0, rp = Rooms; i < MAXROOMS; rp++, i++)
    {
	/*
	 * Find upper left corner of box that this room goes in
	 */
	top.x = (i % 3) * bsze.x + 1;
	top.y = (i / 3) * bsze.y;
	if (rp->r_flags & ISGONE)
	{
	    /*
	     * Place a gone room.  Make certain that there is a blank line
	     * for passage drawing.
	     */
	    do
	    {
		rp->r_pos.x = top.x + rnd(bsze.x - 2) + 1;
		rp->r_pos.y = top.y + rnd(bsze.y - 2) + 1;
		rp->r_max.x = -NUMCOLS;
		rp->r_max.y = -NUMLINES;
	    } until (rp->r_pos.y > 0 && rp->r_pos.y < NUMLINES-1);
	    continue;
	}
	/*
	 * set room type
	 */
	if (rnd(10) < Level - 1)
	{
	    rp->r_flags |= ISDARK;		/* dark room */
	    if (rnd(15) == 0)
		rp->r_flags = ISMAZE;		/* maze room */
	}
	/*
	 * Find a place and size for a random room
	 */
	if (rp->r_flags & ISMAZE)
	{
	    rp->r_max.x = bsze.x - 1;
	    rp->r_max.y = bsze.y - 1;
	    if ((rp->r_pos.x = top.x) == 1)
		rp->r_pos.x = 0;
	    if ((rp->r_pos.y = top.y) == 0)
	    {
		rp->r_pos.y++;
		rp->r_max.y--;
	    }
	}
	else
	    do
	    {
		rp->r_max.x = rnd(bsze.x - 4) + 4;
		rp->r_max.y = rnd(bsze.y - 4) + 4;
		rp->r_pos.x = top.x + rnd(bsze.x - rp->r_max.x);
		rp->r_pos.y = top.y + rnd(bsze.y - rp->r_max.y);
	    } until (rp->r_pos.y != 0);
	draw_room(rp);
	/*
	 * Put the gold in
	 */
	if (rnd(2) == 0 && (!Amulet || Level >= Max_level))
	{
	    tp = new_item();
	    tp->o_goldval = rp->r_goldval = GOLDCALC;
	    find_floor(rp, &rp->r_gold, FALSE, FALSE);
	    tp->o_pos = rp->r_gold;
	    chat(rp->r_gold.y, rp->r_gold.x) = GOLD;
	    tp->o_flags = ISMANY;
	    tp->o_group = GOLDGRP;
	    tp->o_type = GOLD;
	    attach(Lvl_obj, tp);
	}
	/*
	 * Put the monster in
	 */
	if (rnd(100) < (rp->r_goldval > 0 ? 80 : 25))
	{
	    tp = new_item();
	    find_floor(rp, &mp, FALSE, TRUE);
	    new_monster(tp, randmonster(FALSE), &mp);
	    give_pack(tp);
	}
    }
}

/*
 * draw_room:
 *	Draw a box around a room and lay down the floor for normal
 *	rooms; for maze rooms, draw maze.
 */
draw_room(rp)
register struct room *rp;
{
    register int y, x;

    if (rp->r_flags & ISMAZE)
	do_maze(rp);
    else
    {
	vert(rp, rp->r_pos.x);				/* Draw left side */
	vert(rp, rp->r_pos.x + rp->r_max.x - 1);	/* Draw right side */
	horiz(rp, rp->r_pos.y);				/* Draw top */
	horiz(rp, rp->r_pos.y + rp->r_max.y - 1);	/* Draw bottom */

	/*
	 * Put the floor down
	 */
	for (y = rp->r_pos.y + 1; y < rp->r_pos.y + rp->r_max.y - 1; y++)
	    for (x = rp->r_pos.x + 1; x < rp->r_pos.x + rp->r_max.x - 1; x++)
		chat(y, x) = FLOOR;
    }
}

/*
 * vert:
 *	Draw a vertical line
 */
vert(rp, startx)
register struct room *rp;
register int startx;
{
    register int y;

    for (y = rp->r_pos.y + 1; y <= rp->r_max.y + rp->r_pos.y - 1; y++)
	chat(y, startx) = '|';
}

/*
 * horiz:
 *	Draw a horizontal line
 */
horiz(rp, starty)
register struct room *rp;
int starty;
{
    register int x;

    for (x = rp->r_pos.x; x <= rp->r_pos.x + rp->r_max.x - 1; x++)
	chat(starty, x) = '-';
}

/*
 * do_maze:
 *	Dig a maze
 */

static int	Maxy, Maxx, Starty, Startx;

static SPOT	Maze[NUMLINES/3+1][NUMCOLS/3+1];

do_maze(rp)
register struct room *rp;
{
    register SPOT *sp;
    register int starty, startx;
    static coord pos;

    for (sp = &Maze[0][0]; sp < &Maze[NUMLINES / 3][NUMCOLS / 3 + 1]; sp++)
    {
	sp->used = FALSE;
	sp->nexits = 0;
    }

    Maxy = rp->r_max.y;
    Maxx = rp->r_max.x;
    Starty = rp->r_pos.y;
    Startx = rp->r_pos.x;
    starty = (rnd(rp->r_max.y) >> 1) * 2;
    startx = (rnd(rp->r_max.x) >> 1) * 2;
    pos.y = starty + Starty;
    pos.x = startx + Startx;
    putpass(&pos);
    dig(starty, startx);
}

/*
 * dig:
 *	Dig out from around where we are now, if possible
 */
dig(y, x)
register int y;
register int x;
{
    register coord *cp;
    register int cnt, newy, newx;
    int nexty, nextx;
    static coord pos;
    static coord del[4] = {
	{2, 0}, {-2, 0}, {0, 2}, {0, -2}
    };

    for (;;)
    {
	cnt = 0;
	for (cp = del; cp < &del[4]; cp++)
	{
	    newy = y + cp->y;
	    newx = x + cp->x;
	    if (newy < 0 || newy > Maxy || newx < 0 || newx > Maxx)
		continue;
	    if (flat(newy + Starty, newx + Startx) & F_PASS)
		continue;
	    if (rnd(++cnt) == 0)
	    {
		nexty = newy;
		nextx = newx;
	    }
	}
	if (cnt == 0)
	    return;
	accnt_maze(y, x, nexty, nextx);
	accnt_maze(nexty, nextx, y, x);
	if (nexty == y)
	{
	    pos.y = y + Starty;
	    if (nextx - x < 0)
		pos.x = nextx + Startx + 1;
	    else
		pos.x = nextx + Startx - 1;
	}
	else
	{
	    pos.x = x + Startx;
	    if (nexty - y < 0)
		pos.y = nexty + Starty + 1;
	    else
		pos.y = nexty + Starty - 1;
	}
	putpass(&pos);
	pos.y = nexty + Starty;
	pos.x = nextx + Startx;
	putpass(&pos);
	dig(nexty, nextx);
    }
}

/*
 * accnt_maze:
 *	Account for maze exits
 */
accnt_maze(y, x, ny, nx)
int y;
int x;
int ny;
int nx;
{
    register SPOT *sp;
    register coord *cp;

    sp = &Maze[y][x];
    for (cp = sp->exits; cp < &sp->exits[sp->nexits]; cp++)
	if (cp->y == ny && cp->x == nx)
	    return;
    cp->y = ny;
    cp->x = nx;
}

/*
 * rnd_pos:
 *	Pick a random spot in a room
 */
rnd_pos(rp, cp)
register struct room *rp;
register coord *cp;
{
    cp->x = rp->r_pos.x + rnd(rp->r_max.x - 2) + 1;
    cp->y = rp->r_pos.y + rnd(rp->r_max.y - 2) + 1;
}

/*
 * find_floor:
 *	Find a valid floor spot in this room.  If rp is NULL, then
 *	pick a new room each time around the loop.
 */
bool
find_floor(rp, cp, limit, monst)
register struct room *rp;
register coord *cp;
int limit;
bool monst;
{
    register PLACE *pp;
    register int cnt;
    char compchar;
    bool pickroom;

    if (!(pickroom = (rp == NULL)))
	compchar = ((rp->r_flags & ISMAZE) ? PASSAGE : FLOOR);
    cnt = limit;
    for (;;)
    {
	if (limit && cnt-- == 0)
	    return FALSE;
	if (pickroom)
	{
	    rp = &Rooms[rnd_room()];
	    compchar = ((rp->r_flags & ISMAZE) ? PASSAGE : FLOOR);
	}
	rnd_pos(rp, cp);
	pp = INDEX(cp->y, cp->x);
	if (monst)
	{
	    if (pp->p_monst == NULL && step_ok(pp->p_ch))
		return TRUE;
	}
	else if (pp->p_ch == compchar)
	    return TRUE;
    }
}

/*
 * enter_room:
 *	Code that is executed whenver you appear in a room
 */
enter_room(cp, door)
register coord *cp;
bool door;
{
    register struct room *rp;
    register THING *tp;
    register int y, x;
    char ch;

    rp = Proom = roomin(cp);
    door_open(rp);
    if (!(rp->r_flags & ISDARK) && !on(Player, ISBLIND))
	for (y = rp->r_pos.y; y < rp->r_max.y + rp->r_pos.y; y++)
	{
	    move(y, rp->r_pos.x);
	    for (x = rp->r_pos.x; x < rp->r_max.x + rp->r_pos.x; x++)
	    {
		if (door)
		{
		    if (!door_look(y, x, cp))
			continue;
		    move(y, x);
		}
		tp = moat(y, x);
		ch = chat(y, x);
		if (tp == NULL)
		    if (inch() != ch)
			addch(ch);
		    else
			move(y, x + 1);
		else
		{
		    tp->t_oldch = ch;
		    if (!see_monst(tp))
			if (on(Player, SEEMONST))
			{
			    standout();
			    addch(tp->t_disguise);
			    standend();
			}
			else
			    addch(ch);
		    else
			addch(tp->t_disguise);
		}
	    }
	}
}

/*
 * show_room:
 *	Display the destination room when moving room-to-room
 */
show_room(cp)
register coord *cp;
{
    register struct room *rp;
    register THING *tp;
    register int y, x;
    char ch;

    rp = Proom = roomin(cp);
    if (rp->r_flags & ISDARK || on(Player, ISBLIND))
	return;
    for (y = rp->r_pos.y; y < rp->r_max.y + rp->r_pos.y; y++)
    {
	move(y, rp->r_pos.x);
	for (x = rp->r_pos.x; x < rp->r_max.x + rp->r_pos.x; x++)
	{
	    tp = moat(y, x);
	    ch = chat(y, x);
	    if (tp == NULL)
		if (inch() != ch)
		    addch(ch);
		else
		    move(y, x + 1);
	    else
	    {
		tp->t_oldch = ch;
		if (!see_monst(tp))
		    if (on(Player, SEEMONST))
		    {
			standout();
			addch(tp->t_disguise);
			standend();
		    }
		    else
			addch(ch);
		else
		    addch(tp->t_disguise);
	    }
	}
    }
}

/*
 * leave_room:
 *	Code for when we exit a room
 */
leave_room(cp)
register coord *cp;
{
    register struct room *rp;
    register int y, x;
    char floor;
    char ch;

    rp = Proom;

    if (rp->r_flags & ISMAZE)
	return;

    if (rp->r_flags & ISGONE)
	floor = PASSAGE;
    else if (!(rp->r_flags & ISDARK) || on(Player, ISBLIND))
	floor = FLOOR;
    else
	floor = ' ';

    Proom = &Passages[flat(cp->y, cp->x) & F_PNUM];
    for (y = rp->r_pos.y; y < rp->r_max.y + rp->r_pos.y; y++)
	for (x = rp->r_pos.x; x < rp->r_max.x + rp->r_pos.x; x++)
	{
	    move(y, x);
	    switch (ch = inch())
	    {
		case FLOOR:
		    if (floor == ' ' && ch != ' ')
			addch(' ');
		    break;
		default:
		    /*
		     * to check for monster, we have to strip out
		     * standout bit
		     */
		    if (isupper(toascii(ch)))
		    {
			if (on(Player, SEEMONST))
			{
			    standout();
			    addch(ch);
			    standend();
			}
			else
			    addch(INDEX(y, x)->p_ch == DOOR ? DOOR : floor);
		    }
	    }
	}
    door_open(rp);
}
