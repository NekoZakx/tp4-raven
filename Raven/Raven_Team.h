#ifndef RAVEN_TEAM_H
#define RAVEN_TEAM_H

#include "Raven_Bot.h"
#include <string>
#include <vector>

class Raven_Team
{
	private:
		std::string team_name;
		int ID;
		std::vector<Raven_Bot*> teamMember;

	public:
		Raven_Team(std::string name, int ID);
		~Raven_Team();

		void AddBot(Raven_Bot*);

		void RemoveBot(Raven_Bot*);

		bool IsInTeam(Raven_Bot*);

		int GetTeamSize();
		int GetID();
		std::string GetName();
};

#endif