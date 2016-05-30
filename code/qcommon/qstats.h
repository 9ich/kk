/*
 * Stat tracking indices.
 * Changes to QSTAT_MAX must be reflected in the stats server.
 */
enum
{
	QSTAT_KILLS,
	QSTAT_DEATHS,
	QSTAT_DAMAGE,
	QSTAT_ACCURACY,
	QSTAT_TIME,	
	QSTAT_AWARD_IMPRESSIVE,	// two railgun hits in a row
	QSTAT_AWARD_CAPTURE,		// captured the flag
	QSTAT_AWARD_DEFEND,		// defended base
	QSTAT_AWARD_ASSIST,		// assisted carrier
	QSTAT_AWARD_DENIED,		// denied flag
	QSTAT_AWARD_GAUNTLET,		// killed melee
	QSTAT_AWARD_HUMILIATED,	// receiving end of melee
	QSTAT_AWARD_KILLINGSPREE,	// 5 kills in one life
	QSTAT_AWARD_DOMINATING,	// 10
	QSTAT_AWARD_RAMPAGE,		// 15
	QSTAT_AWARD_UNSTOPPABLE,	// 20
	QSTAT_AWARD_GODLIKE,		// 25
	QSTAT_AWARD_WICKEDSICK,	// 30
	QSTAT_AWARD_DOUBLEKILL,	// 2 kills in a few seconds
	QSTAT_AWARD_MULTIKILL,	// 3
	QSTAT_AWARD_MEGAKILL,		// 4
	QSTAT_AWARD_ULTRAKILL,	// 5
	QSTAT_AWARD_MONSTERKILL,	// 6
	QSTAT_AWARD_LUDICROUSKILL,	// 7
	QSTAT_AWARD_HOLYSHIT,		// 8
	QSTAT_AWARD_FIRSTBLOOD,	// first kill
	QSTAT_AWARD_SADDAY,		// died >=3 times without killing anyone

	QSTAT_MAX
};

