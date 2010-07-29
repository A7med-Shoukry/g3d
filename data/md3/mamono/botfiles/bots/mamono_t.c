//===========================================================================
//	Mamono	version 0.0
//
//
//	Made with doktor's BotStudio v. 0.98
//	http://www.planetquake.com/botstudio  e-mail: doktor@cutey.com
//===========================================================================


chat "mamono"
{
	#include "teamplay.h"

	type "game_enter"
	{
		HELLO10;
		HELLO2;
		HELLO7;
	}

	type "game_exit"
	{
		GOODBYE5;
		GOODBYE4;
		GOODBYE1;
	}

	type "level_start"
	{
		LEVEL_START;
		LEVEL_START3;
		LEVEL_START2;
	}

	type "level_end"
	{
		LEVEL_END0;
		LEVEL_END4;
		LEVEL_END0;
	}

	type "level_end_victory"
	{
		LEVEL_END_VICTORY2;
		LEVEL_END_VICTORY2;
		LEVEL_END_VICTORY3;
	}

	type "level_end_lose"
	{
		LEVEL_END_LOSE2;
		LEVEL_END_LOSE2;
		LEVEL_END_LOSE2;
	}

	type "hit_talking"
	{
		DEATH_TALKING;
		DEATH_TALKING;
		DEATH_TALKING;
	}

	type "hit_nodeath"
	{
		TAUNT2;
		TAUNT;
		TAUNT9;
	}

	type "hit_nokill"
	{
		HIT_NOKILL0;
		HIT_NOKILL0;
		HIT_NOKILL0;
	}

	type "death_telefrag"
	{
		DEATH_TELEFRAGGED1;
		DEATH_TELEFRAGGED0;
		DEATH_TELEFRAGGED0;
	}

	type "death_cratered"
	{
		DEATH_FALLING;
		DEATH_FALLING;
		DEATH_FALLING;
	}

	type "death_lava"
	{
		DEATH_LAVA2;
		DEATH_LAVA1;
		DEATH_LAVA0;
	}

	type "death_slime"
	{
		DEATH_SLIME0;
		DEATH_SLIME1;
		DEATH_SLIME1;
	}

	type "death_drown"
	{
		DEATH_DROWN0;
		DEATH_DROWN1;
		DEATH_DROWN0;
	}

	type "death_suicide"
	{
		DEATH_SUICIDE2;
		DEATH_SUICIDE2;
		DEATH_SUICIDE2;
	}

	type "death_gauntlet"
	{
		DEATH_GAUNTLET2;
		DEATH_GAUNTLET0;
		DEATH_GAUNTLET1;
	}

	type "death_rail"
	{
		DEATH_RAIL2;
		DEATH_RAIL0;
		DEATH_RAIL1;
	}

	type "death_bfg"
	{
		DEATH_BFG1;
		DEATH_BFG1;
		DEATH_BFG2;
	}

	type "death_insult"
	{
		DEATH_INSULT2;
		DEATH_INSULT6;
		DEATH_INSULT0;
	}

	type "death_praise"
	{
		D_PRAISE6;
		D_PRAISE;
		D_PRAISE2;
	}

	type "kill_rail"
	{
		DEATH_RAIL0;
		DEATH_RAIL1;
		DEATH_RAIL1;
	}

	type "kill_gauntlet"
	{
		KILL_GAUNTLET1;
		KILL_GAUNTLET0;
		KILL_GAUNTLET1;
	}

	type "kill_telefrag"
	{
		TELEFRAGGED4;
		TELEFRAGGED7;
		TELEFRAGGED5;
	}

	type "kill_suicide"
	{
		TAUNT3;
		TAUNT2;
		TAUNT1;
	}

	type "kill_insult"
	{
		KILL_INSULT39;
		KILL_INSULT28;
		KILL_INSULT0;
	}

	type "kill_praise"
	{
		PRAISE;
		PRAISE4;
		PRAISE3;
	}

	type "random_insult"
	{
		MISC2;
		MISC3;
		MISC10;
	}

	type "random_misc"
	{
		MISC2;
		MISC6;
		MISC4;
	}
}
