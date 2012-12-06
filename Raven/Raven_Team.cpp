#include "./Raven_Team.h"

//Create a Team instance with the name
Raven_Team::Raven_Team( std::string name ) {
	team_name = name;
}

//Destroy the team
Raven_Team::~Raven_Team() {
	//delete team_name;
	teamMember.clear();
}

//Add bot to the team
void Raven_Team::AddBot( Raven_Bot* bot) {
	teamMember.push_back(bot);
}

//Remove bot to the team
void Raven_Team::RemoveBot( Raven_Bot* bot) {
	for(int i = 0; i < teamMember.size(); i++) {
		if(teamMember[i] == bot){
			teamMember[i] = NULL;
			teamMember.erase(teamMember.begin()+i);
			break;
		}
	}	
}

//Get if the bot is in team
bool Raven_Team::IsInTeam( Raven_Bot* bot) {
	for(int i = 0; i < teamMember.size(); i++) {
		if(teamMember[i] == bot){
			return true;
		}
	}	
	return false;
}