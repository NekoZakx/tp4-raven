#include "Raven_WeaponSystem.h"
#include "armory/Weapon_RocketLauncher.h"
#include "armory/Weapon_RailGun.h"
#include "armory/Weapon_ShotGun.h"
#include "armory/Weapon_Blaster.h"
#include "Raven_Bot.h"
#include "misc/utils.h"
#include "lua/Raven_Scriptor.h"
#include "Raven_Game.h"
#include "Raven_UserOptions.h"
#include "2D/transformations.h"
#include <cstring>
#include <string.h>
#include <cstringt.h>


//------------------------- ctor ----------------------------------------------
//-----------------------------------------------------------------------------
Raven_WeaponSystem::Raven_WeaponSystem(Raven_Bot* owner,
                                       double ReactionTime,
                                       double AimAccuracy,
                                       double AimPersistance):m_pOwner(owner),
                                                          m_dReactionTime(ReactionTime),
                                                          m_dAimAccuracy(AimAccuracy),
                                                          m_dAimPersistance(AimPersistance)
{
  Initialize();
}

//------------------------- dtor ----------------------------------------------
//-----------------------------------------------------------------------------
Raven_WeaponSystem::~Raven_WeaponSystem()
{
  for (unsigned int w=0; w<m_WeaponMap.size(); ++w)
  {
    delete m_WeaponMap[w];
  }
}

//------------------------------ Initialize -----------------------------------
//
//  initializes the weapons
//-----------------------------------------------------------------------------
void Raven_WeaponSystem::Initialize()
{
  //delete any existing weapons
  WeaponMap::iterator curW;
  for (curW = m_WeaponMap.begin(); curW != m_WeaponMap.end(); ++curW)
  {
    delete curW->second;
  }

  m_WeaponMap.clear();

  //set up the container
  m_pCurrentWeapon = new Blaster(m_pOwner);

  m_WeaponMap[type_blaster]         = m_pCurrentWeapon;
  m_WeaponMap[type_shotgun]         = 0;
  m_WeaponMap[type_rail_gun]        = 0;
  m_WeaponMap[type_rocket_launcher] = 0;

  InitializeFuzzyModule();
}

//-------------------------------- SelectWeapon -------------------------------
//
//-----------------------------------------------------------------------------
void Raven_WeaponSystem::SelectWeapon()
{ 
  //if a target is present use fuzzy logic to determine the most desirable 
  //weapon.
  if (m_pOwner->GetTargetSys()->isTargetPresent())
  {
    //calculate the distance to the target
    double DistToTarget = Vec2DDistance(m_pOwner->Pos(), m_pOwner->GetTargetSys()->GetTarget()->Pos());

    //for each weapon in the inventory calculate its desirability given the 
    //current situation. The most desirable weapon is selected
    double BestSoFar = MinDouble;

    WeaponMap::const_iterator curWeap;
    for (curWeap=m_WeaponMap.begin(); curWeap != m_WeaponMap.end(); ++curWeap)
    {
      //grab the desirability of this weapon (desirability is based upon
      //distance to target and ammo remaining)
      if (curWeap->second)
      {
        double score = curWeap->second->GetDesirability(DistToTarget);

        //if it is the most desirable so far select it
        if (score > BestSoFar)
        {
          BestSoFar = score;

          //place the weapon in the bot's hand.
          m_pCurrentWeapon = curWeap->second;
        }
      }
    }
  }

  else
  {
    m_pCurrentWeapon = m_WeaponMap[type_blaster];
  }
}

//--------------------  AddWeapon ------------------------------------------
//
//  this is called by a weapon affector and will add a weapon of the specified
//  type to the bot's inventory.
//
//  if the bot already has a weapon of this type then only the ammo is added
//-----------------------------------------------------------------------------
void  Raven_WeaponSystem::AddWeapon(unsigned int weapon_type)
{
  //create an instance of this weapon
  Raven_Weapon* w = 0;

  switch(weapon_type)
  {
  case type_rail_gun:

    w = new RailGun(m_pOwner); break;

  case type_shotgun:

    w = new ShotGun(m_pOwner); break;

  case type_rocket_launcher:

    w = new RocketLauncher(m_pOwner); break;

  }//end switch
  

  //if the bot already holds a weapon of this type, just add its ammo
  Raven_Weapon* present = GetWeaponFromInventory(weapon_type);

  if (present)
  {
    present->IncrementRounds(w->NumRoundsRemaining());

    delete w;
  }
  
  //if not already holding, add to inventory
  else
  {
    m_WeaponMap[weapon_type] = w;
  }
}


//------------------------- GetWeaponFromInventory -------------------------------
//
//  returns a pointer to any matching weapon.
//
//  returns a null pointer if the weapon is not present
//-----------------------------------------------------------------------------
Raven_Weapon* Raven_WeaponSystem::GetWeaponFromInventory(int weapon_type)
{
  return m_WeaponMap[weapon_type];
}

//----------------------- ChangeWeapon ----------------------------------------
void Raven_WeaponSystem::ChangeWeapon(unsigned int type)
{
  Raven_Weapon* w = GetWeaponFromInventory(type);

  if (w) m_pCurrentWeapon = w;
}

//Initialisation des variables pour le FuzzyModule
void Raven_WeaponSystem::InitializeFuzzyModule()
{
	FuzzyVariable& DistToTarget = m_FuzzyModule.CreateFLV("DistToTarget");
	FzSet& Target_Close = DistToTarget.AddLeftShoulderSet("Target_Close",0,30,60);
	FzSet& Target_Medium = DistToTarget.AddTriangularSet("Target_Medium",30,60,150);
	FzSet& Target_Far = DistToTarget.AddRightShoulderSet("Target_Far",60,150,1000);

	FuzzyVariable& Precision = m_FuzzyModule.CreateFLV("Precision");
	FzSet& VeryLowPrecision = Precision.AddLeftShoulderSet("VeryLowPrecision", 0, 0, 0.2);
	FzSet& LowPrecision = Precision.AddTriangularSet("LowPrecision", 0, 0.2, 0.4);
	FzSet& MediumPrecision = Precision.AddTriangularSet("MediumPrecision", 0.2, 0.4, 0.6);
	FzSet& HighPrecision = Precision.AddTriangularSet("HighPrecision", 0.4, 0.6, 0.8);
	FzSet& VeryHighPrecision = Precision.AddRightShoulderSet("VeryHighPrecision", 0.6, 0.8, 1);

	FuzzyVariable& Velocity = m_FuzzyModule.CreateFLV("Velocity");
	FzSet& LowVelocity = Velocity.AddLeftShoulderSet("LowVelocity", 0, 0, 0.70);
	FzSet& MediumVelocity = Velocity.AddTriangularSet("MediumVelocity", 0, 0.70, 0.90);
	FzSet& HighVelocity = Velocity.AddRightShoulderSet("HighVelocity", 0.70, 0.90, 2);

	FuzzyVariable& TimeVisible = m_FuzzyModule.CreateFLV("TimeVisible");
	FzSet& LowTimeVisible = TimeVisible.AddLeftShoulderSet("LowTimeVisible", 0, 0, 2);
	FzSet& MediumTimeVisible = TimeVisible.AddTriangularSet("MediumTimeVisible", 0, 2, 5);
	FzSet& HighTimeVisible = TimeVisible.AddRightShoulderSet("HighTimeVisible", 2, 5, 100);

	//Définition des règles
	m_FuzzyModule.AddRule(FzAND(Target_Close, LowTimeVisible, LowVelocity), VeryHighPrecision);
	m_FuzzyModule.AddRule(FzAND(Target_Close, LowTimeVisible, MediumVelocity), HighPrecision);
	m_FuzzyModule.AddRule(FzAND(Target_Close, LowTimeVisible, HighVelocity), MediumPrecision);

	m_FuzzyModule.AddRule(FzAND(Target_Close, MediumTimeVisible, LowVelocity), VeryHighPrecision);
	m_FuzzyModule.AddRule(FzAND(Target_Close, MediumTimeVisible, MediumVelocity), VeryHighPrecision);
	m_FuzzyModule.AddRule(FzAND(Target_Close, MediumTimeVisible, HighVelocity), HighPrecision);

	m_FuzzyModule.AddRule(FzAND(Target_Close, HighTimeVisible, LowVelocity), VeryHighPrecision);
	m_FuzzyModule.AddRule(FzAND(Target_Close, HighTimeVisible, MediumVelocity), VeryHighPrecision);
	m_FuzzyModule.AddRule(FzAND(Target_Close, HighTimeVisible, HighVelocity), VeryHighPrecision);

	m_FuzzyModule.AddRule(FzAND(Target_Medium, LowTimeVisible, LowVelocity), HighPrecision);
	m_FuzzyModule.AddRule(FzAND(Target_Medium, LowTimeVisible, MediumVelocity), MediumPrecision);
	m_FuzzyModule.AddRule(FzAND(Target_Medium, LowTimeVisible, HighVelocity), LowPrecision);

	m_FuzzyModule.AddRule(FzAND(Target_Medium, MediumTimeVisible, LowVelocity), HighPrecision);
	m_FuzzyModule.AddRule(FzAND(Target_Medium, MediumTimeVisible, MediumVelocity), HighPrecision);
	m_FuzzyModule.AddRule(FzAND(Target_Medium, MediumTimeVisible, HighVelocity), MediumPrecision);

	m_FuzzyModule.AddRule(FzAND(Target_Medium, HighTimeVisible, LowVelocity), HighPrecision);
	m_FuzzyModule.AddRule(FzAND(Target_Medium, HighTimeVisible, MediumVelocity), HighPrecision);
	m_FuzzyModule.AddRule(FzAND(Target_Medium, HighTimeVisible, HighVelocity), HighPrecision);

	m_FuzzyModule.AddRule(FzAND(Target_Far, LowTimeVisible, LowVelocity), MediumPrecision);
	m_FuzzyModule.AddRule(FzAND(Target_Far, LowTimeVisible, MediumVelocity), LowPrecision);
	m_FuzzyModule.AddRule(FzAND(Target_Far, LowTimeVisible, HighVelocity), VeryLowPrecision);

	m_FuzzyModule.AddRule(FzAND(Target_Far, MediumTimeVisible, LowVelocity), MediumPrecision);
	m_FuzzyModule.AddRule(FzAND(Target_Far, MediumTimeVisible, MediumVelocity), MediumPrecision);
	m_FuzzyModule.AddRule(FzAND(Target_Far, MediumTimeVisible, HighVelocity), LowPrecision);

	m_FuzzyModule.AddRule(FzAND(Target_Far, HighTimeVisible, LowVelocity), MediumPrecision);
	m_FuzzyModule.AddRule(FzAND(Target_Far, HighTimeVisible, MediumVelocity), MediumPrecision);
	m_FuzzyModule.AddRule(FzAND(Target_Far, HighTimeVisible, HighVelocity), MediumPrecision);

}

//---------------------------- Precision -----------------------------------
//
//-----------------------------------------------------------------------------
double Raven_WeaponSystem::GetPrecision(double DistToTarget, double Velocity, double TimeVisible)
{
	/*TCHAR   szBuffer[32]; 
	sprintf( szBuffer, "%f", DistToTarget );
	OutputDebugString ("DistToTarget: ");
	OutputDebugString (szBuffer);

	sprintf( szBuffer, "%f", Velocity );
	OutputDebugString (" === Velocity: ");
	OutputDebugString (szBuffer);

	sprintf( szBuffer, "%f", TimeVisible );
	OutputDebugString (" === TimeVisible: ");
	OutputDebugString (szBuffer);*/

    //fuzzify distance velocity and time visible
    m_FuzzyModule.Fuzzify("DistToTarget", DistToTarget);
    m_FuzzyModule.Fuzzify("Velocity", Velocity);
	m_FuzzyModule.Fuzzify("TimeVisible", TimeVisible);

    return m_FuzzyModule.DeFuzzify("Precision", FuzzyModule::max_av);
}


//--------------------------- TakeAimAndShoot ---------------------------------
//
//  this method aims the bots current weapon at the target (if there is a
//  target) and, if aimed correctly, fires a round
//-----------------------------------------------------------------------------
void Raven_WeaponSystem::TakeAimAndShoot()
{
  //aim the weapon only if the current target is shootable or if it has only
  //very recently gone out of view (this latter condition is to ensure the 
  //weapon is aimed at the target even if it temporarily dodges behind a wall
  //or other cover)
  if (m_pOwner->GetTargetSys()->isTargetShootable() ||
      (m_pOwner->GetTargetSys()->GetTimeTargetHasBeenOutOfView() < 
       m_dAimPersistance) )
  {
    //the position the weapon will be aimed at
	  int random = rand() % 100;
	int direction = 1;
	double theta = 0.2; //Tweak the angle of a shot, the angle will be more revelant if far of target
	
	if(random <= 50)
	{
		direction = -1;
	}

	double precision = GetPrecision(Vec2DDistance(m_pOwner->Pos(), m_pOwner->GetTargetSys()->GetTarget()->Pos()), m_pOwner->GetTargetSys()->GetTarget()->Velocity().Length(), m_pOwner->GetTargetSys()->GetTimeTargetHasBeenVisible());
	
	/*TCHAR   szBuffer[32]; 
	sprintf( szBuffer, "%f", (1- precision ));
	OutputDebugString (" === PRECISION: ");
	OutputDebugString (szBuffer);

	OutputDebugString ("\n");*/

	theta = theta * (1 - precision) * direction;
	
	double s = sin(theta);
	double c = cos(theta);

	double x = m_pOwner->GetTargetBot()->Pos().x;
	double y = m_pOwner->GetTargetBot()->Pos().y;

	double x_origin = x - m_pOwner->Pos().x;
	double y_origin = y - m_pOwner->Pos().y;

	double px = x_origin * c - y_origin * s;
	double py = x_origin * s + y_origin * c;

	px = px + m_pOwner->Pos().x;
	py = py + m_pOwner->Pos().y;
	
	/*TCHAR   szBuffer[32]; 
	sprintf( szBuffer, "%f", theta );
	OutputDebugString ("\n------------------------------------------\nTHETA: ");
	OutputDebugString (szBuffer);

	OutputDebugString ("\n");
	
	sprintf( szBuffer, "%f", m_pOwner->GetTargetBot()->Pos().x );
	OutputDebugString ("NOR_AIM_X: ");
	OutputDebugString (szBuffer);

	sprintf( szBuffer, "%f", m_pOwner->GetTargetBot()->Pos().y );
	OutputDebugString (" NOR_AIM_Y: ");
	OutputDebugString (szBuffer);
	
	OutputDebugString ("\n");

	sprintf( szBuffer, "%f", px );
	OutputDebugString ("NEW_AIM_X: ");
	OutputDebugString (szBuffer);

	sprintf( szBuffer, "%f", py );
	OutputDebugString (" NEW_AIM_Y: ");
	OutputDebugString (szBuffer);*/


	Vector2D AimingPos = Vector2D(px, py);
	//Vector2D AimingPos = m_pOwner->GetTargetBot()->Pos();
    
    //if the current weapon is not an instant hit type gun the target position
    //must be adjusted to take into account the predicted movement of the 
    //target
    if (GetCurrentWeapon()->GetType() == type_rocket_launcher ||
        GetCurrentWeapon()->GetType() == type_blaster)
    {
      AimingPos = PredictFuturePositionOfTarget();

      //if the weapon is aimed correctly, there is line of sight between the
      //bot and the aiming position and it has been in view for a period longer
      //than the bot's reaction time, shoot the weapon
      if ( m_pOwner->RotateFacingTowardPosition(AimingPos) &&
           (m_pOwner->GetTargetSys()->GetTimeTargetHasBeenVisible() >
            m_dReactionTime) &&
           m_pOwner->hasLOSto(AimingPos) )
      {
        AddNoiseToAim(AimingPos);

        GetCurrentWeapon()->ShootAt(AimingPos);
      }
    }

    //no need to predict movement, aim directly at target
    else
    {
      //if the weapon is aimed correctly and it has been in view for a period
      //longer than the bot's reaction time, shoot the weapon
      if ( m_pOwner->RotateFacingTowardPosition(AimingPos) &&
           (m_pOwner->GetTargetSys()->GetTimeTargetHasBeenVisible() >
            m_dReactionTime) )
      {
        AddNoiseToAim(AimingPos);
        
        GetCurrentWeapon()->ShootAt(AimingPos);
      }
    }

  }
  
  //no target to shoot at so rotate facing to be parallel with the bot's
  //heading direction
  else
  {
    m_pOwner->RotateFacingTowardPosition(m_pOwner->Pos()+ m_pOwner->Heading());
  }
}

//---------------------------- AddNoiseToAim ----------------------------------
//
//  adds a random deviation to the firing angle not greater than m_dAimAccuracy 
//  rads
//-----------------------------------------------------------------------------
void Raven_WeaponSystem::AddNoiseToAim(Vector2D& AimingPos)const
{
  Vector2D toPos = AimingPos - m_pOwner->Pos();

  Vec2DRotateAroundOrigin(toPos, RandInRange(-m_dAimAccuracy, m_dAimAccuracy));

  AimingPos = toPos + m_pOwner->Pos();
}

//-------------------------- PredictFuturePositionOfTarget --------------------
//
//  predicts where the target will be located in the time it takes for a
//  projectile to reach it. This uses a similar logic to the Pursuit steering
//  behavior.
//-----------------------------------------------------------------------------
Vector2D Raven_WeaponSystem::PredictFuturePositionOfTarget()const
{
  double MaxSpeed = GetCurrentWeapon()->GetMaxProjectileSpeed();
  
  //if the target is ahead and facing the agent shoot at its current pos
  Vector2D ToEnemy = m_pOwner->GetTargetBot()->Pos() - m_pOwner->Pos();
 
  //the lookahead time is proportional to the distance between the enemy
  //and the pursuer; and is inversely proportional to the sum of the
  //agent's velocities
  double LookAheadTime = ToEnemy.Length() / 
                        (MaxSpeed + m_pOwner->GetTargetBot()->MaxSpeed());
  
  //return the predicted future position of the enemy
  return m_pOwner->GetTargetBot()->Pos() + 
         m_pOwner->GetTargetBot()->Velocity() * LookAheadTime;
}


//------------------ GetAmmoRemainingForWeapon --------------------------------
//
//  returns the amount of ammo remaining for the specified weapon. Return zero
//  if the weapon is not present
//-----------------------------------------------------------------------------
int Raven_WeaponSystem::GetAmmoRemainingForWeapon(unsigned int weapon_type)
{
  if (m_WeaponMap[weapon_type])
  {
    return m_WeaponMap[weapon_type]->NumRoundsRemaining();
  }

  return 0;
}

//---------------------------- ShootAt ----------------------------------------
//
//  shoots the current weapon at the given position
//-----------------------------------------------------------------------------
void Raven_WeaponSystem::ShootAt(Vector2D pos)const
{
  GetCurrentWeapon()->ShootAt(pos);
}

//-------------------------- RenderCurrentWeapon ------------------------------
//-----------------------------------------------------------------------------
void Raven_WeaponSystem::RenderCurrentWeapon()const
{
  GetCurrentWeapon()->Render();
}

void Raven_WeaponSystem::RenderDesirabilities()const
{
  Vector2D p = m_pOwner->Pos();

  int num = 0;
  
  WeaponMap::const_iterator curWeap;
  for (curWeap=m_WeaponMap.begin(); curWeap != m_WeaponMap.end(); ++curWeap)
  {
    if (curWeap->second) num++;
  }

  int offset = 15 * num;

    for (curWeap=m_WeaponMap.begin(); curWeap != m_WeaponMap.end(); ++curWeap)
    {
      if (curWeap->second)
      {
        double score = curWeap->second->GetLastDesirabilityScore();
        std::string type = GetNameOfType(curWeap->second->GetType());

        gdi->TextAtPos(p.x+10.0, p.y-offset, ttos(score) + " " + type);

        offset+=15;
      }
    }
}
