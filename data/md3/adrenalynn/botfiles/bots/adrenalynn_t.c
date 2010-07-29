
//===========================================================================
//
// Name:		adrenalynn_c.c
// Function:		chat lines for adrenalynn
// Programmer:		MrElusive (MrElusive@idsoftware.com)
// Author:		Tony Daniel & Mark Forsyth
//Editor: 		Mark Forsyth
// Last update:		Jun.19, 2001
// Tab Size:		3 (real tabs)
//===========================================================================

//example initial chats
chat "adrenalynn"
{
	//the teamplay.h file is included for all kinds of teamplay chats
	#include "teamplay.h"

	//======================================================
	//======================================================

	type "game_enter" //initiated when the bot enters the game
	{
		"Where are we, Boss?";
		"It's ADRENALYNN!!";
		"We are Adrenalynn. A cybergenetic fighting being.";
		"Who will be Adrenalynn's first victim?";
		"What's my mission here, Boss?";
		"My name is Sabina. You may call me Adrenalynn. Or better yet ... CALL ME YOUR KILLER!";
		// 0 = bot name
	} //end type

	type "game_exit" //initiated when the bot exits the game
	{
		"Let's drop all the lines, Papa Smurf, I want OUT of here.";
		"I will not be anyone's plaything! I am not a toy!";
		"Later.";
		"I'm sick of all the yahoos acting like they own me.";
		// 0 = bot name
	} //end type

	type "level_start" //initiated when a new level starts
	{
		"We know who and what we are looking for.";
		"They know I'm here.";
		"Can't get enough of me can ya?";
		"We are Adrenalynn. A cybergenetic fighting being.";
		// 0 = bot name
	} //end type

	type "level_end" //initiated when a level ends and the bot is not first and not last in the rankings
	{
		"That was just a warm up match.";
		"Second place = first loser";
		":p";
		"*sigh*";
		"GG";
		// 0 = bot name
	} //end type

	type "level_end_victory" //initiated when a level ends and the bot is first in the rankings
	{
		"My name's Adrenalynn. Thanks for dropping by.";
		"Not bad, for a 'school girl' ;)";
		"Mission successful. Awaiting further orders.";
		"I'll be glad to show you the door - pipsqueek.";
		// 0 = bot name
	} //end type

	type "level_end_lose" //initiated when a level ends and the bot is last in the rankings
	{
		"Mission failed. Awaiting further orders.";
		"I demand a recount!";
		"Even a robot girl's gotta have her 'off' days.";
		"Boo-frickin'-hoo!";
		":p";
		// 0 = bot name
	} //end type

	//======================================================
	//======================================================

	type "hit_talking" //bot is hit while chat balloon is visible; lecture attacker on poor sportsmanship
	{
		"Cheap shot ", 0, "!";
		"Fight like a robot, ", 0, "!";
		"Hey ", 0, "! I'm talkin' here.";
		"You take what you can get there, ", 0, ".";
		"They didn't tell me that I'd have to deal with this crap!";
		//0 = shooter
	} //end type

	type "hit_nodeath" //bot is hit by an opponent's weapon attack; either praise or insult
	{
		"I detect a life form approaching.";
		"It's gonna take a bit more than that to stop me, ", 0, ".";
		"You want some ", 0, "?";
		"Take a number, ", 0, ".";
		"Stop it, ", 0, ", that tickles.";
		"Target: ", 0, " - approaching";
		//0 = shooter
	} //end type

	type "hit_nokill" //bot hits an opponent but does not kill it
	{
		"Stop hiding behind that cloud of dust, worm!";
		"Target acquired.";
		"Target,", 0, " identified.";
		"Get back here and fight like a robot, ", 0, "!";
		"You can run ", 0, ", but you'll only die tired.";
		HIT_NOKILL1;
		//0 = opponent
	} //end type

	//======================================================
	//======================================================

	type "death_telefrag" //initiated when the bot is killed by a telefrag
	{
		"I guess if you can't score a legit kill, ", 0, ", you gotta take what you can get.";
		"How about an 'excuse me' dammit!";
		// 0 = eneour name
	} //end type

	type "death_cratered" //initiated when the bot is killed by taking "normal" falling damage
	{
		"Who the hell designed this frickin' place?";
		"It didn't look that high up.";
		"I can withstand a fall from any hei...";
		// 0 = eneour name
	} //end type

	type "death_lava" //initiated when the bot dies in lava
	{
		"Pffft! I've felt hotter.";
		"I'm melting! I'm melllttti...";
		//"They didn't tell me I'd have to deal with this crap!";
		// 0 = eneour name
	} //end type

	type "death_slime" //initiated when the bot dies in slime
	{
		"Ewwww!";
		"Mmmmm ... Tastes like green jello!";
		"What the hell is this green sh...";
		// 0 = eneour name
	} //end type

	type "death_drown" //initiated when the bot drowns
	{
		"Oooops! I forgot robot girls can't swim.";
		"Dammit! My joints will rust.";
		"Someone over-filled the kiddie pool.";
		"I was thirsty anyhow.";
		// 0 = eneour name
	} //end type

	type "death_suicide" //initiated when bot blows self up with a weapon or craters
	{
		"Malfunction, Boss?";
		"Brainwave hard-drive must've malfunctioned.";
		"I MEANT to do that. :p";
		"I figured y'all could use some help fragging me. :p";
		// 0 = eneour name
	} //end type

	type "death_gauntlet" //initiated when the bot is killed by a gauntlet attack
	{
		"Try that again and you'll be pulling back a bloody stump ", 0, "!";
		"Ohhh. You wanna get physical with me ", 0, "?";
		"I'm gonna rip that arm off and beat you with it, ", 0, "!";
		"Watch your back, ", 0, "!";
		// 0 = eneour name
	} //end type

	type "death_rail" //initiated when the bot is killed by a rail gun shot
	{
		"Afraid to face Adrenalynn, ", 0, "?";
		"Get out here and fight like a robot ", 0, "!";
		"Did you remember to pack your sleeping bag, ", 0, "?";
		"Rail-Whore!";
		// 0 = eneour name
	} //end type

	type "death_bfg" //initiated when the bot died by a BFG
	{
		"Now THAT is a big gun.";
		"Where can I get me one of those, ", 0, "?";
		"Couldn't you find a bigger gun to hide behind ", 0, "?";
		// 0 = eneour name
	} //end type

	type "death_insult" //insult initiated when the bot died
	{
		"Watch your back, worm!";
		"One thing's for sure ... I'm more human than you, ", 0, "!";
		"You just made the top of my hit list, ", 0, ".";
		"Target: ", 0, " - Prioritized.";
		"Lucky shot, ", 0, ".";
		"You'll be eatin' through a straw before this is over, ", 0, ".";
		// 0 = eneour name
	} //end type

	type "death_praise" //praise initiated when the bot died
	{
		"You CAN fight like a robot, ", 0, ".";
		"Ouch! That's gonna leave a mark...";
		0, ", you're a heck of a lot brighter than you look.";
		"Congratulations! You just made the top of my hit list, ", 0, ".";
		"Dang ", 0, ", you CAN shoot straight.";
		"You da man, ", 0, ".";
		"Impressive, perhaps ... to Hunter ... but not to Adrenalynn!";
		"Whoa, ", 0, " ... you been practicin' or what?";
		// 0 = eneour name
	} //end type

	//======================================================
	//======================================================

	type "kill_rail" //initiated when the bot kills someone with rail gun
	{
		"How'd that feel ", 0, "?";
		"Target: ", 0, "- Eliminated.";
		// 0 = eneour name
	} //end type

	type "kill_gauntlet" //initiated when the bot kills someone with gauntlet
	{
		"Now you know just how physical I can be, ", 0, ".";
		"Target: ", 0, "- Eliminated.";
		// 0 = eneour name
	} //end type

	type "kill_telefrag" //initiated when the bot telefragged someone
	{
		"The sooner you get out of my way and let me do my business ", 0, ", the sooner I'll stop killing you.";
		"Ewwwww! Gross!";
		// 0 = eneour name
	} //end type

	type "kill_suicide" //initiated when the players kills self
	{
		"You should really get that checked out, ", 0, ".";
		"Now THAT was graceful, ", 0, ".";
		"Did that hurt ", 0, "?";
		"Take that home, work on it, and bring it back ", 0, ".";
		"Ha! Ha! I saw that ", 0, ".";
		// 0 = eneour name
	} //end type

	type "kill_insult" //insult initiated when the bot killed someone
	{
		"Ka-Blamn!";
		"Target eliminated.";
		"Target: ", 0, "- Eliminated.";
		"Be sure to say hello to Rhazes Darkk for me, ", 0, "!";
		"Who's next?";
		"Just doing my part to strengthen the gene pool, ", 0, ".";
		"A ", weapon, " is a robot girl's best friend.";
		"Ha!Ha! Made ya die.";
		"Schooled ya ", 0, ".";
		"When ya gonna learn ", 0, "?";
		"Awwww... Does it hurt ", 0, "?";
		"You musta come here for ass kickin and lollipops, ", 0, ". Trouble is - I'm fresh outta lollipops.";
		// 0 = eneour name
	} //end type

	type "kill_praise" //praise initiated when the bot killed someone
	{
		"Target eliminated.";
		"Target: ", 0, "- Eliminated.";
		"You almost got away from me there, ", 0, " ... almost.";
		"They can rebuild you, ", 0,". They have the technology.";
		"Hey, ", 0, " - duck next time!";
		"Ashes to ashes, dust to dust.";
		// 0 = eneour name
	} //end type

	//======================================================
	//======================================================

	type "random_insult" //insult initiated randomly (just when the bot feels like it)
	{
		"You have been targeted for elimination, ", 0, ".";
		"Who wants some?";
		"School is in session, ", 0, ".";
		"Your ass is mine ", 0, ".";
		"One thing's for sure ... I'm more human than you ", 0, ".";
		"I got your number ... I got ALL your numbers.";
		"I got a ", weapon, " with your name on it, ", 0, ".";
		MISC12;
		// 0 = name of randomly chosen player
		// 1 = bot name
	} //end type

	type "random_misc" //miscellanous chats initiated randomly
	{
		"Who wants some?";
		"I may not be a complete human being, but I got a real soul.";
		"I got your number ... I got ALL your numbers.";
		"00101 011 0100101 1110101"; 
		"Welcome to ", 4, " ... Your final resting place, ", 0, ".";
		"We are Adrenalynn. A cybergenetic fighting being.";
		"Come get some ", 0, "!";
		// 0 = name of randomly chosen player
		// 1 = bot name
	} //end type
} //end adrenalynn chat

