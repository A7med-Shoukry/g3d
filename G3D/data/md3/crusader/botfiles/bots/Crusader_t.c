//===========================================================================
//
// Name:			crusader_c.c
// Function:		chat lines for crusader
// Programmer:		MrElusive (MrElusive@idsoftware.com)
// Last update:		1999-09-08
// Tab Size:		3 (real tabs)
//===========================================================================

//example initial chats
chat "crusader"
{
	//the teamplay.h file is included for all kinds of teamplay chats
	#include "teamplay.h"
	//======================================================
	//======================================================
	type "game_enter" //initiated when the bot enters the game
	{
		"Borne forth unto a world of darkness...";
		"What is this hellish place?";
		"Many a heretic has persished at my blade";
		"Evil doers, prepare to die";
		"The Lord has plans for me here i see...";
		"Ye Gods, the stench..";
		HELLO7;
		"Hmm.";
		"The ", 4, "? what manner of castle is this?";
		// 1= random opponent
		// 4= Level's title
		// 0 = bot name
	} //end type

	type "game_exit" //initiated when the bot exits the game
	{
		"I must depart, on a more important mission";
		GOODBYE1;
		"Ah ", 1, ", I enjoyed that melee.";
		"I shall adjourn. But i will be back.";
		// 1= random opponent
		// 0 = bot name
	} //end type

	type "level_start" //initiated when a new level starts
	{
		"You wish for more? I can only give you more pain";
		"Ahh, my favourite arena";
		"Watch how I mock thee";
		"Again? dear boy you have persistence if nothing else.";
		LEVEL_START2;
		// 0 = bot name
	} //end type


	type "level_end" //initiated when a level ends and the bot is not first and not last in the rankings
	{
		"Ahh, a minor setback..";
		"Again, my blade yearns for more blood";
		"Heathen, the dark side shall never triumph";
		2, ", maybe if we have a discussion about faith?";
		3, " is obviously a pawn of the devil.";
		2, " is a true warrior";
	
		// 2 = opponent in first place
		// 3 = opponent in last place
		// 0 = bot name
	} //end type

	type "level_end_victory" //initiated when a level ends and the bot is first in the rankings
	{	
		"Good will always triumph";
		"VICTORIOUS, such is the way of the lord";
		"See ", 3, ", devilspawn filth will be crushed.";
		"Death to all heretics, in a sea of blood!";
		LEVEL_END_VICTORY4;
		"It was prophecised as so.";
		"Learn the ways of good";
		"Hark", 3, ", you will feel more pain by the flames of hell.";
		1, " do you see how easy it is?";
		// 1 = random opponent
		// 3 = opponent in last place
		// 0 = bot name
	} //end type

	type "level_end_lose" //initiated when a level ends and the bot is last in the rankings
	{
		"Ahem.. some more training is needed at the guild methinks..";
		"Fair play to ",2,", a valiant knight";
		LEVEL_END_LOSE2;
		2, "Come join our cause, we need people like you.";
		// 2 = opponent in first place
		// 0 = bot name
	} //end type


	//======================================================
	//======================================================

	type "hit_talking" //bot is hit while chat balloon is visible; lecture attacker on poor sportsmanship
	{
		"Hah! ", 0, ", take the easy option, you'll need it!";
		"Did I feel a ", 1, " shot from ", 0, "?  How unsportsmanlike..";
		"Why", 0, ", do you hit ladies aswell?";
		"Sirrah, the heathen hits below the belt!";
		//1 = weapon used by shooter
		//0 = shooter
	} //end type

	type "hit_nodeath" //bot is hit by an opponent's weapon attack; either praise or insult
	{
		"HaHAhh!, missed!";
		"Ouch";
		0, "if you cant hit me with a ", 1, ", your not going to last long";
		"'tis merely a flesh wound";
		"Nurse!";
		"I do believe you have dented my chainmail";
		"Hmm.. that stung";
		//1 = weapon used by shooter		
		//0 = shooter
	} //end type


	type "hit_nokill" //bot hits an opponent but does not kill it
	{
		"Run peasant, I shall find you sooner or later";
		"You cannot escape from the eyes of the lord!";		
		"Heh, methinks", 0, " runs like a girl";
		"Come back and fight like a man";
		"Ach.. you cannot find the knights these days";
		TAUNT;
		TAUNT7;
		"I can see you ", 0, "..";
		//0 = opponent	
	} //end type

	//======================================================
	//======================================================

	type "death_telefrag" //initiated when the bot is killed by a telefrag
	{
		"What manner of powers do you have, ",0,"?";
		"Satanic power! ", 0, " must be stopped!";
		"That wasn't very nice..";
		DEATH_TELEFRAGGED0;
		// 0 = enemy name
	} //end type

	type "death_cratered" //initiated when the bot is killed by taking "normal" falling damage
	{	
		"A horse, a horse..my kingdom for a horse";
		"I've never tasted my own kneecap before.";
		"Gravity, one of the dark powers!";
		// 0 = enemy name
	} //end type

	type "death_lava" //initiated when the bot dies in lava
	{
		"Hellfire?, NO, I should be in Heaven!";
		"Purifying flames, cleanse the soul";
		// 0 = enemy name
	} //end type

	type "death_slime" //initiated when the bot dies in slime
	{	
		"Ack.. demonic goo!";
		"gulp";
		DEATH_SLIME0;
		"Gag";
		// 0 = enemy name
	} //end type

	type "death_drown" //initiated when the bot drowns
	{
		"Zounds, my armour is too heavy!";
		"Faith will let you breath?";
		"Hmm I never did learn to swim";
		"Sinking? that means that I am a witch!";
		// 0 = enemy name
	} //end type

	type "death_suicide" //initiated when bot blows self up with a weapon or craters
	{
		"Hmm.. maybe i should just stick to my blade";
		"These strange devices are new to me";
		"Okay, point open end forward first, THEN pull lever";
		DEATH_SUICIDE3;
		DEATH_SUICIDE4;
		"Immortal Sacrifice!";
		"Put these strange contraptions away and we'll fight like men!";
		// 0 = enemy name
	} //end type

	type "death_gauntlet" //initiated when the bot is killed by a gauntlet attack
	{	
		"Ahaa a warrior skilled in unarmed combat!";
		"Slapped by a glove? does this mean you wish to duel?";
		"The insult, beaten by an unarmed man";
		DEATH_INSULT4;
		"Tis truly a glove of Satan";
		//1 = weapon used by shooter	
		// 0 = enemy name
	} //end type

	type "death_rail" //initiated when the bot is killed by a rail gun shot
	{	
		"Straight through me.. what manner of trickery?";
		DEATH_INSULT0;
		DEATH_RAIL2;
		"I felt the touch of the lord!";
		"Holey?";
		// 0 = enemy name
	} //end type

	type "death_bfg" //initiated when the bot died by a BFG
	{
		"Bathed in a purifying light...";
		"Nice engine, ", 0, " what other trickery can it do?";
		"AAGHH, it burns!";
		"Bee-eff-gee?  is this a mantra";
		DEATH_BFG2;
		// 1 = weapon used by shooter
		// 0 = enemy name
	} //end type

	type "death_insult" //insult initiated when the bot died
	{
		"A pause to consider my strategy..";
		"Evil villain, ", 0, " You will PAY!";
		0, " ..you will feel the wrath of my blade";
		DEATH_INSULT3;
		"Okay, now you shall be destroyed";
		"You will perish by my sword";
		"Dost thou feel better for that? for you shall feel pain heareafter";
		// 0 = enemy name
	} //end type


	type "death_praise" //praise initiated when the bot died
	{	
		"Your mastery with the ", 1, " is unparallelled.";
		"Maybe i could learn some lessons from you ", 0, ".";
		D_PRAISE5;
		"Maybe I should practice with my ", 1, "..?";
		// 1 = weapon used by shooter
		// 0 = enemy name
	} //end type

	//======================================================
	//======================================================

	type "kill_rail" //initiated when the bot kills someone with rail gun
	{
		"Feel the fury of my siege engine";
		KILL_RAIL1;
		"Instant death, I think I like this gun";
		"Like a bolt of fury";
		"The word of god says... SPLAT!";
		"Instant Judgement, instant punishment";
		// 0 = enemy name
	} //end type

	type "kill_gauntlet" //initiated when the bot kills someone with gauntlet
	{
		"Ahh the satisfaction of a melee fight..";
		"I waited until i could see your pleading eyes, ", 0, "...";
		"Put down these sissy weapons and fight close!";
		"I Smite thee down like the scum you are!";
		"This glove suits me well.";
		"You should be happy I did not use my blade..";
		"Bah, my blade is better than this thing..";
		DEATH_GAUNTLET2;
		// 0 = enemy name
	} //end type

	type "kill_telefrag" //initiated when the bot telefragged someone
	{
		"Where am I , and why am i covered in... ", 0, "?";
		TELEFRAGGED3;
		"Ahem, pardon me sirrah.";
		"Reminds me of morning gruel..";
		"Such an easy kill, I feel ashamed.";
		// 0 = enemy name
	} //end type

	type "kill_suicide" //initiated when the player kills self
	{
		KILL_EXTREME_INSULT;
		KILL_INSULT34;
		"Sir, If you contine like that I shall be forced to laugh";
		"Maybe a game of chess would be more your cup of tea?";
		"Am I needed here?, ", 0, " seems to be killing himself fine";
		// 0 = enemy name
	} //end type

	type "kill_insult" //insult initiated when the bot killed someone
	{
		"You are not worthy enough to exist.";
		"I pity you";
		"Poor fool, do you not see the error of your ways?";
		"If you would only accept the light, I would not have to do this";
		"Idiot, you force my hand this way!";
		"Take a lesson from ", 0, ", chaps..";
		"Tut Tut.. must I continue beating you?";
		"Thou art a fool to be here";
		KILL_INSULT19;
		KILL_INSULT24;
		// 0 = enemy name
	} //end type

	type "kill_praise" //praise initiated when the bot killed someone
	{
		PRAISE4;
		"Come come", 0, ", If you just yield then you could join me";
		"Join my side ", 0, ", im sure I could train you to be better";
		// 0 = enemy name
	} //end type	

	//======================================================
	//======================================================

	type "random_insult" //insult initiated randomly (just when the bot feels like it)
	{
		"Hmm.. methinks ", 0, " needs some punishment.";
		"A tiresome sqabble.  But, my work needs to be done.";
		"'Kill more' the voice said and 'hurt them.' The voices don't like you!";
		0," screams louder than a school girl.";
		1, " has an odd fascination with the pavement.";
		"'", 1, " loses in ", 4, ". Perfect for your tombstone, ", 1, "!";	
		// 0 = name of randomly chosen player
		// 1 = bot name
		// 4 = level's title 
	} //end type


	type "random_misc" //miscellanous chats initiated randomly
	{
		"So on and so forth...";
		"Merely a flesh wound";
		"Ho hum";
		" "
		" ";
		" ";
		" ";
		" ";
		MISC1;
		MISC0;
		one_liners;
		" ";
		" ";
		// 0 = name of randomly chosen player
		// 1 = bot name
		// 5 = random weapon from weapons list
	} //end type
} //end chat


