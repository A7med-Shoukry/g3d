//===========================================================================
//
// Name:		lisab_c.c
// Function:	chat for lisab
// Programmer:		MrElusive (MrElusive@idsoftware.com)
// Author:		David Coen fish_dsc@yahoo.co.uk
// Editor:		
// Last update:		1/5/01
// Tab Size:		2 (real tabs)
//===========================================================================

chat "lisab"
{
	//the teamplay.h file is included for all kinds of teamplay chats
	#include "teamplay.h"

	//======================================================
	//======================================================

	type "game_enter" //initiated when the bot enters the game
	{
		"So where are we";
            "Oh man, you're ugly";
            "Mum told me just to shoot guys like you!";
		"I'm so cool";
		"I ROCK!";
		"So, like only dweebs play this game?";
		// 0 = bot name
	} //end type

	type "game_exit" //initiated when the bot exits the game
	{
		"Remeber, I'll call you, ", 0,"";
		"Good bye, will you miss me!";
		"I'm out of here!";
		"Oh, I have to go, bye";
		"catch you latter, loosers";
		// 0 = bot name
	} //end type

	type "level_start" //initiated when a new level starts
	{
		"So you do you look like inside!";
		"Hey ", 0," don't call me some time! ";
		"", 0,", If the phone doesn't ring, it's me!";
		"yar, keep yaking away!";	
		// 0 = bot name
	} //end type

	type "level_end" //initiated when a level ends and the bot is not first and not last in the rankings
	{
		"Well that suxed!";
		"You guys played like shit!";
		"You guys sux!";
		"Again?";
		"alwyas time for one more!";
		"You all so ugly!";
		// 0 = bot name
	} //end type

	type "level_end_victory" //initiated when a level ends and the bot is first in the rankings
	{
		"Ohhh bite me!";
		"You need a bigger gun, little boy!";
		"I Rule!";
		"Can't touch this!";
		"You all sux!";
		"Finnaly!";
		// 0 = bot name
	} //end type

	type "level_end_lose" //initiated when a level ends and the bot is last in the rankings
	{
            "you all sux!";
		"this suxs";
		"I want my money back!";
		"ok, now play a good level";
		"perverts!";
		// 0 = bot name
	} //end type

	//======================================================
	//======================================================

	type "hit_talking" //bot is hit while chat balloon is visible; lecture attacker on poor sportsmanship
	{
            "Get lost ", 0,", I'm bussy";
		"", 0," is a looser!";
		"Not now! dickless ", 0,".";
		"Could you stop that ", 0,".";	
		"Get a life ", 0,".";
		//0 = shooter
	} //end type

	type "hit_nodeath" //bot is hit by an opponent's weapon attack; either praise or insult
	{
		"Leave me alone ", 0,"!";
		"No fair!";
		"Go kill someone else ", 0,"!";
		"Piss off ", 0,"!";
		"Watch it ", 0,"!";

		//0 = shooter
	} //end type

	type "hit_nokill" //bot hits an opponent but does not kill it
	{
		"Lucky!";
            "Hey, that's my kill!";
		"Oppsie, pardon me";
		"You been powering up or something ", 0,"!";
            "Im not finished with you ", 0,"!"; 
		"Oh, ", 0," you like that!";
		//0 = opponent
	} //end type

	//======================================================
	//======================================================

	type "death_telefrag" //initiated when the bot is killed by a telefrag
	{
		"Hey! ", 0,"! I was here first pervert!'";
		"Always trying to get into my pants!"; 
		"Lame!";
		// 0 = enemy name
	} //end type

	type "death_cratered" //initiated when the bot is killed by taking "normal" falling damage
	{
		"I hate these shoes!";
		"That suxed";
		"Gravity is a bitch!";
		"Ouch!.";
		"Shit that hurts!";

		// 0 = enemy name
	} //end type

	type "death_lava" //initiated when the bot dies in lava
	{
		"Lava! What will they think of next!";
		"Don't need this sort of tan!";
		"HELP!";
		"ok, that was dumb";
            "ouch, ouch, ouch, hot, hot";
		// 0 = enemy name
	} //end type

	type "death_slime" //initiated when the bot dies in slime
	{
		"Ewww, yuck!";
		"This smeels like you! ";
		"So where are the tenticles?";
		"This is ridiculous!";
		// 0 = enemy name
	} //end type

	type "death_drown" //initiated when the bot drowns
	{
		"Glugg, they said this was vodka!";
		"Oh, this suxs!";
		"I want my money back!";
		"doh!";
		"Feeling somewhat bloated!";
		"Oh that's cold!";
		// 0 = enemy name
	} //end type

	type "death_suicide" //initiated when bot blows self up with a weapon or craters
	{
            "Well, you wheren't going to!"; 
		"Oppsie!";
		"Just getting away from ", 0,"!";
		"goodbye crule world";
		"One more time into the void";
            "Slipped!";
		// 0 = enemy name
	} //end type

	type "death_gauntlet" //initiated when the bot is killed by a gauntlet attack
	{
		"Pervert!";
            "You keep sneaking up on me";
		"You're so lame ", 0, ".";
		"Just try that again.";
		// 0 = enemy name
	} //end type

	type "death_rail" //initiated when the bot is killed by a rail gun shot
	{
		"You sux ", 0,"!";
            "Learn to play you camper!";
		"Watch out for snipers! like ", 0,"!";
            "Oh, Harder!";
            "Wuss!";
            "Come down here and do that?";
		// 0 = enemy name
	} //end type

	type "death_bfg" //initiated when the bot died by a BFG
	{
		"Feeling a bit insecure?";
		"That should be mine";
		"what they say about a man with a big..?";
            "this is fun";
		// 0 = enemy name
	} //end type

	type "death_insult" //insult initiated when the bot died
	{
		"As if you could ever have me ", 0,"!";
		"I would rather die than be with you! ", 0,"!";
		"You are no fun!";
            "Don't you like me!";   
		"I don't like this game anymore";
		"Is that all you have got!";
		"Saves me from looking at your face";
            "Time for you to sux on my lance ", 0,"!";
            "Can you call a doc! ", 0,"!";
            "Didn't hurt, but this will ", 0,"!";
            "Oh lucky shot ", 0,"!"; 
            "Behind ya ", 0,"!";
            "Over here lover!";
            "Look out for my missiles";             
            "Here comes another one ", 0,"!";
            "Poor devil";             
            "Look out ", 0,"!"; 
            "Lets do it again!";
            "Your mine lover!"; 
            "Don't stop now!", 0,"!";


		// 0 = enemy name
	} //end type

	type "death_praise" //praise initiated when the bot died
	{
		"Ok, you play well ", 0,", pervert!";
		"Not bad for a fanboy";
		"Where are you going, this is fun!";
		"I want to play again!";
		"You were almost good enough ", 0,"!";
		// 0 = enemy name
	} //end type

	//======================================================
	//======================================================

	type "kill_rail" //initiated when the bot kills someone with rail gun
	{
            "Sux on that looser!";
		"Pop!";
		"No that was a good shot!";
		"Take it you pussy!";
		"Loose weight now, ask me how!";
		"You have never looked better?";
		"This is fun!";
		"You never where that smart!";
            "Ha!";
		"right in the head!";
            "Bite me!";
		// 0 = enemy name
	} //end type

	type "kill_gauntlet" //initiated when the bot kills someone with gauntlet
	{
		"Damn, I was aiming lower!";
		"It slices, it dices!"
		"Save the whale, eat people!";
		"shit, i broke a nail!";
            "You never where that pretty!";
		"Buzzzz!";
		// 0 = enemy name
	} //end type

	type "kill_telefrag" //initiated when the bot telefragged someone
	{
		"Too slow ", 0,"!";
            "You realy need to loose weight!'";
		"Get out of my way ", 0,"!"; 
		
		// 0 = enemy name
	} //end type

	type "kill_suicide" //initiated when the player kills self with a weapon of craters
	{
		"Hey, that's my job!";
            "You have never looked better, ", 0,"!";
		"Gewd, what a mess!";
		"Are you paying attention ", 0, "?";
		"You know how to play this, don't you!";
		"Man you sux ", 0, "!";
		// 0 = enemy name
	} //end type

	type "kill_insult" //insult initiated when the bot killed someone
	{

		"You not my type ,", 0, "!";
            "Hey, ugly, out of my way!";
		"Oh, and you where playing so well!";
		"You're so stupid!";
		"Awww, are there any men here?";
		"Couldn't take the pace";
		// 0 = enemy name
	} //end type

	type "kill_praise" //praise initiated when the bot killed someone
	{
		"Harder ", 0, "!";
		"Come back!";
		"Play with me some more!";
		"Hey ,", 0, " this is for you!";
		"Never touch me!";
		// 0 = enemy name
	} //end type

	//======================================================
	//======================================================

	type "random_insult" //insult initiated randomly (just when the bot feels like it)
	{
		"Any mem in here!";
		"Man enough for me?";
		"Bring it on!";
		"Where are the cute guys?";
		"So when is this fun?";
		"", 1, " is the one";
		"these shoes are killing me!";
		// 0 = name of randomly chosen player
		// 1 = bot name
	} //end type

	type "random_misc" //miscellanous chats initiated randomly
	{
		"Anyone out their!";
		"I want to play!";
		"is the ugly one still here?";
		"Stop looking at my tits";
		"I'm board?";
		"If only this wasn't a game.";
            "Behind ya,", 0, "!";
		"You think you can take me,", 0, "?";
		"You want some of me?";
		// 0 = name of randomly chosen player
		// 1 = bot name
	} //end type
} //end Girl chat







