#ifndef RAVEN_TEAM_H
#define RAVEN_TEAM_H

#include "Raven_Bot.h"
#include <string>
#include <vector>

class Raven_Team
{
	private:
		string team_name;
		vector<Raven_Bot*> teamMember;

	public:
		Raven_Team(string name);
		~Raven_Team();

		void AddBot(Raven_Bot*);

		void RemoveBot(Raven_Bot*);

		bool IsInTeam(Raven_Bot*);
};

#endif