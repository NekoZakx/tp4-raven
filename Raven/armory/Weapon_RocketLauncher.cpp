#include "Weapon_RocketLauncher.h"
#include "../Raven_Bot.h"
#include "misc/Cgdi.h"
#include "../Raven_Game.h"
#include "../Raven_Map.h"
#include "../lua/Raven_Scriptor.h"
#include "fuzzy/FuzzyOperators.h"


//--------------------------- ctor --------------------------------------------
//-----------------------------------------------------------------------------
RocketLauncher::RocketLauncher(Raven_Bot*   owner):

                      Raven_Weapon(type_rocket_launcher,
                                   script->GetInt("RocketLauncher_DefaultRounds"),
                                   script->GetInt("RocketLauncher_MaxRoundsCarried"),
                                   script->GetDouble("RocketLauncher_FiringFreq"),
                                   script->GetDouble("RocketLauncher_IdealRange"),
                                   script->GetDouble("Rocket_MaxSpeed"),
                                   owner)
{
    //setup the vertex buffer
  const int NumWeaponVerts = 8;
  const Vector2D weapon[NumWeaponVerts] = {Vector2D(0, -3),
                                           Vector2D(6, -3),
                                           Vector2D(6, -1),
                                           Vector2D(15, -1),
                                           Vector2D(15, 1),
                                           Vector2D(6, 1),
                                           Vector2D(6, 3),
                                           Vector2D(0, 3)
                                           };
  for (int vtx=0; vtx<NumWeaponVerts; ++vtx)
  {
    m_vecWeaponVB.push_back(weapon[vtx]);
  }

  //setup the fuzzy module
  InitializeFuzzyModule();

}


//------------------------------ ShootAt --------------------------------------
//-----------------------------------------------------------------------------
inline void RocketLauncher::ShootAt(Vector2D pos)
{ 
  if (NumRoundsRemaining() > 0 && isReadyForNextShot())
  {
    //fire off a rocket!
    m_pOwner->GetWorld()->AddRocket(m_pOwner, pos);

    m_iNumRoundsLeft--;

    UpdateTimeWeaponIsNextAvailable();

    //add a trigger to the game so that the other bots can hear this shot
    //(provided they are within range)
    m_pOwner->GetWorld()->GetMap()->AddSoundTrigger(m_pOwner, script->GetDouble("RocketLauncher_SoundRange"));
  }
}

//---------------------------- Desirability -----------------------------------
//
//-----------------------------------------------------------------------------
double RocketLauncher::GetDesirability(double DistToTarget)
{
  if (m_iNumRoundsLeft == 0)
  {
    m_dLastDesirabilityScore = 0;
  }
  else
  {
    //fuzzify distance and amount of ammo
    m_FuzzyModule.Fuzzify("DistToTarget", DistToTarget);
    m_FuzzyModule.Fuzzify("AmmoStatus", (double)m_iNumRoundsLeft);

    m_dLastDesirabilityScore = m_FuzzyModule.DeFuzzify("Desirability", FuzzyModule::max_av);
  }

  return m_dLastDesirabilityScore;
}

//-------------------------  InitializeFuzzyModule ----------------------------
//
//  set up some fuzzy variables and rules
//-----------------------------------------------------------------------------
void RocketLauncher::InitializeFuzzyModule()
{
  FuzzyVariable& DistToTarget = m_FuzzyModule.CreateFLV("DistToTarget");

  FzSet& Target_VeryClose = DistToTarget.AddLeftShoulderSet("Target_VeryClose",0,0,75);
  FzSet& Target_Close = DistToTarget.AddTriangularSet("Target_Close",0,75,150);
  FzSet& Target_Medium = DistToTarget.AddTriangularSet("Target_Medium",25,150,300);
  FzSet& Target_Far = DistToTarget.AddTriangularSet("Target_Far",150,300,450);
  FzSet& Target_FarFarAway = DistToTarget.AddRightShoulderSet("Target_FarFarAway",300,450,1000);

  FuzzyVariable& Desirability = m_FuzzyModule.CreateFLV("Desirability");
  FzSet& ExtraDesirable = Desirability.AddRightShoulderSet("ExtraDesirable", 60, 80, 100);
  FzSet& VeryDesirable = Desirability.AddTriangularSet("VeryDesirable", 40, 60, 80);
  FzSet& Desirable = Desirability.AddTriangularSet("Desirable", 20, 40, 60);
  FzSet& Undesirable = Desirability.AddTriangularSet("Undesirable", 0, 20, 40);
  FzSet& VeryUndesirable = Desirability.AddTriangularSet("VeryUndesirable", 0, 0, 20);

  FuzzyVariable& AmmoStatus = m_FuzzyModule.CreateFLV("AmmoStatus");
  FzSet& Ammo_Overloads = AmmoStatus.AddRightShoulderSet("Ammo_Overloads", 50, 70, 100);
  FzSet& Ammo_Loads = AmmoStatus.AddTriangularSet("Ammo_Loads", 20, 50, 70);
  FzSet& Ammo_Okay = AmmoStatus.AddTriangularSet("Ammo_Okay", 10, 20, 50);
  FzSet& Ammo_Low = AmmoStatus.AddTriangularSet("Ammo_Low", 0, 10, 20);
  FzSet& Ammo_VeryLow = AmmoStatus.AddTriangularSet("Ammo_VeryLow", 0, 0, 10);

  //D�finition des nouvelles r�gles
  m_FuzzyModule.AddRule(FzAND(Target_VeryClose, Ammo_Overloads), VeryUndesirable);
  m_FuzzyModule.AddRule(FzAND(Target_VeryClose, Ammo_Loads), VeryUndesirable);
  m_FuzzyModule.AddRule(FzAND(Target_VeryClose, Ammo_Okay), VeryUndesirable);
  m_FuzzyModule.AddRule(FzAND(Target_VeryClose, Ammo_Low), VeryUndesirable);
  m_FuzzyModule.AddRule(FzAND(Target_VeryClose, Ammo_VeryLow), VeryUndesirable);

  m_FuzzyModule.AddRule(FzAND(Target_Close, Ammo_Overloads), Undesirable);
  m_FuzzyModule.AddRule(FzAND(Target_Close, Ammo_Loads), Undesirable);
  m_FuzzyModule.AddRule(FzAND(Target_Close, Ammo_Okay), Undesirable);
  m_FuzzyModule.AddRule(FzAND(Target_Close, Ammo_Low), Undesirable);
  m_FuzzyModule.AddRule(FzAND(Target_Close, Ammo_VeryLow), Undesirable);

  m_FuzzyModule.AddRule(FzAND(Target_Medium, Ammo_Overloads), ExtraDesirable);
  m_FuzzyModule.AddRule(FzAND(Target_Medium, Ammo_Loads), VeryDesirable);
  m_FuzzyModule.AddRule(FzAND(Target_Medium, Ammo_Okay), VeryDesirable);
  m_FuzzyModule.AddRule(FzAND(Target_Medium, Ammo_Low), Desirable);
  m_FuzzyModule.AddRule(FzAND(Target_Medium, Ammo_VeryLow), Undesirable);

  m_FuzzyModule.AddRule(FzAND(Target_Far, Ammo_Overloads), VeryDesirable);
  m_FuzzyModule.AddRule(FzAND(Target_Far, Ammo_Loads), Desirable);
  m_FuzzyModule.AddRule(FzAND(Target_Far, Ammo_Okay), Undesirable);
  m_FuzzyModule.AddRule(FzAND(Target_Far, Ammo_Low), Undesirable);
  m_FuzzyModule.AddRule(FzAND(Target_Far, Ammo_VeryLow), VeryUndesirable);

  m_FuzzyModule.AddRule(FzAND(Target_FarFarAway, Ammo_Overloads), Undesirable);
  m_FuzzyModule.AddRule(FzAND(Target_FarFarAway, Ammo_Loads), VeryUndesirable);
  m_FuzzyModule.AddRule(FzAND(Target_FarFarAway, Ammo_Okay), VeryUndesirable);
  m_FuzzyModule.AddRule(FzAND(Target_FarFarAway, Ammo_Low), VeryUndesirable);
  m_FuzzyModule.AddRule(FzAND(Target_FarFarAway, Ammo_VeryLow), VeryUndesirable);

}


//-------------------------------- Render -------------------------------------
//-----------------------------------------------------------------------------
void RocketLauncher::Render()
{
    m_vecWeaponVBTrans = WorldTransform(m_vecWeaponVB,
                                   m_pOwner->Pos(),
                                   m_pOwner->Facing(),
                                   m_pOwner->Facing().Perp(),
                                   m_pOwner->Scale());

  gdi->RedPen();

  gdi->ClosedShape(m_vecWeaponVBTrans);
}