#if !defined(__CINT__) || defined(__MAKECINT_)
#include "AliFemtoManager.h"
#include "AliFemtoModelManager.h"
#include "AliFemtoEventReaderAODMultSelection.h"
#include "AliFemtoSimpleAnalysis.h"
#include "AliFemtoBasicEventCut.h"
#include "AliFemtoESDTrackCut.h"
#include "AliFemtoCorrFctn.h"
#include "AliFemtoQinvCorrFctn.h"
#include "AliFemtoVertexMultAnalysis.h"
#include "AliFemtoV0PairCut.h"
#include "AliFemtoV0TrackPairCut.h"
#include "AliFemtoV0TrackCut.h"
#include "AliFemtoPairCutRadialDistance.h"
#include "AliFemtoCorrFctnNonIdDR.h"
#include "AliFemtoAvgSepCorrFctn.h"
#include "AliFemtoModelCorrFctn.h"
#include "AliESDtrack.h"
#include "AliFemtoCutMonitorEventMult.h"
#endif

enum ESys { kLL , kALAL , kLAL , kPL , kAPL , kPAL , kAPAL , kPP , kPAP , kAPAP, nSys };
enum EPart { kELambda , kEAntiLambda , kEProton , kEAntiProton };

const char *sysNames[nSys] = { "V0LL", "V0ALAL", "V0LAL", "V0PL", "V0APL", "V0PAL", "V0APAL","PP","PAP","APAP" };

const int nMult = 10;
int runMult[nMult] = {1, 1, 1, 1, 1, 1, 0, 0, 0, 0};
int multBins[nMult+1] = {0, 50, 100, 200, 300, 400, 500, 600, 700, 800, 900};

int runSys[nSys] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1};

bool separationCuts;

AliFemtoEventReaderAODMultSelection* GetReader2015(bool mcAnalysis);
AliFemtoEventReaderAODChain* GetReader2011(bool mcAnalysis);
AliFemtoVertexMultAnalysis* GetAnalysis(double multMin, double multMax);
AliFemtoBasicEventCut* GetEventCut();
AliFemtoV0TrackCut* GetV0TrackCut(EPart particle);
AliFemtoESDTrackCut* GetESDTrackCut(EPart particle);
AliFemtoV0PairCut* GetV0PairCut(ESys system);
AliFemtoESDTrackCut* GetESDTrackCut(EPart particle);
AliFemtoV0PairCut* GetV0PairCut(ESys system);
AliFemtoV0TrackPairCut* GetV0TrackPairCut(ESys system);
AliFemtoPairCutRadialDistance* GetTracksPairCut(ESys system);
void GetParticlesForSystem(ESys system, EPart &firstParticle, EPart &secondParticle);

//________________________________________________________________________
AliFemtoManager* ConfigFemtoAnalysis(bool mcAnalysis=false, bool sepCuts=false, int year=2015)
{
  separationCuts = sepCuts;
  
  // create analysis managers
  AliFemtoManager* Manager=new AliFemtoManager();
  AliFemtoModelManager *modelMgr = new AliFemtoModelManager();
  
  // add event reader
  if(year==2015)
  {
    AliFemtoEventReaderAODMultSelection* Reader2015 = GetReader2015(mcAnalysis);
    Manager->SetEventReader(Reader2015);
  }
  else if(year==2011)
  {
    AliFemtoEventReaderAODChain* Reader2011 = GetReader2011(mcAnalysis);
    Manager->SetEventReader(Reader2011);
  }
  
  // declare necessary objects
  AliFemtoVertexMultAnalysis    *femtoAnalysis[nSys*nMult];
  AliFemtoBasicEventCut         *eventCut[nSys*nMult];
  
  AliFemtoAvgSepCorrFctn        *avgSepCF[nSys*nMult];
  AliFemtoCorrFctnNonIdDR       *nonIdenticalCF[nSys*nMult];
  AliFemtoQinvCorrFctn          *identicalCF[nSys*nMult];
  AliFemtoModelCorrFctn         *modelCF[nSys*nMult];
  AliFemtoPairOriginMonitor     *pairOriginPass[nSys*nMult];
  AliFemtoPairOriginMonitor     *pairOriginFail[nSys*nMult];
  
  AliFemtoCutMonitorEventMult   *eventMultMonitorPass[nSys*nMult];
  AliFemtoCutMonitorEventMult   *eventMultMonitorFail[nSys*nMult];
  
  // setup analysis
  int anIter = 0;
  for (int imult=0; imult<nMult; imult++)
  {
    if (!runMult[imult]) continue;
    
    for (int iSys=0; iSys<nSys; iSys++)
    {
      if (!runSys[iSys]) continue;
      
      anIter = imult * nSys + iSys;
      
      // create new analysis
      femtoAnalysis[anIter] = GetAnalysis(multBins[imult], multBins[imult+1]);
      
      // get event cut
      eventCut[anIter] = GetEventCut();
      
      // get particle cuts
      EPart firstParticle, secondParticle;
      GetParticlesForSystem((ESys)iSys,firstParticle,secondParticle);
      
      AliFemtoV0TrackCut  *firstV0TrackCut    = GetV0TrackCut(firstParticle);
      AliFemtoV0TrackCut  *secondV0TrackCut   = GetV0TrackCut(secondParticle);
      AliFemtoESDTrackCut *firstESDTrackCut   = GetESDTrackCut(firstParticle);
      AliFemtoESDTrackCut *secondESDTrackCut  = GetESDTrackCut(secondParticle);
      
      // get pair cut
      AliFemtoV0PairCut               *V0pairCut      = GetV0PairCut((ESys)iSys);
      AliFemtoV0TrackPairCut          *V0trackPairCut = GetV0TrackPairCut((ESys)iSys);
      AliFemtoPairCutRadialDistance   *tracksPairCut  = GetTracksPairCut((ESys)iSys);
      
      // create monitors
      if(mcAnalysis)
      {
        pairOriginPass[anIter] = new AliFemtoPairOriginMonitor(Form("Pass%stpcM%i", sysNames[iSys], imult));
        pairOriginFail[anIter] = new AliFemtoPairOriginMonitor(Form("Fail%stpcM%i", sysNames[iSys], imult));
      }
      eventMultMonitorPass[anIter] = new AliFemtoCutMonitorEventMult(Form("Pass%sM%i",sysNames[iSys],imult));
      eventMultMonitorFail[anIter] = new AliFemtoCutMonitorEventMult(Form("Fail%sM%i",sysNames[iSys],imult));
      eventCut[anIter]->AddCutMonitor(eventMultMonitorPass[anIter],eventMultMonitorFail[anIter]);
      
      // setup anallysis cuts
      femtoAnalysis[anIter]->SetEventCut(eventCut[anIter]);
      femtoAnalysis[anIter]->SetV0SharedDaughterCut(true);
      femtoAnalysis[anIter]->SetEnablePairMonitors(false);
      
      if(firstV0TrackCut)
        femtoAnalysis[anIter]->SetFirstParticleCut(firstV0TrackCut);
      else
        femtoAnalysis[anIter]->SetFirstParticleCut(firstESDTrackCut);
      if(secondV0TrackCut)
        femtoAnalysis[anIter]->SetSecondParticleCut(secondV0TrackCut);
      else
        femtoAnalysis[anIter]->SetSecondParticleCut(secondESDTrackCut);
      
      if(V0pairCut){
        if(mcAnalysis) V0pairCut->AddCutMonitor(pairOriginPass[anIter], pairOriginFail[anIter]);
        femtoAnalysis[anIter]->SetPairCut(V0pairCut);
        
      }
      else if(V0trackPairCut){
        if(mcAnalysis) V0trackPairCut->AddCutMonitor(pairOriginPass[anIter], pairOriginFail[anIter]);
        femtoAnalysis[anIter]->SetPairCut(V0trackPairCut);
      }
      else
      {
        if(mcAnalysis) tracksPairCut->AddCutMonitor(pairOriginPass[anIter], pairOriginFail[anIter]);
        femtoAnalysis[anIter]->SetPairCut(tracksPairCut);
      }
      
      if(iSys==kLL || iSys == kALAL || iSys == kLAL || iSys == kPL || iSys == kAPL || iSys == kPAL || iSys == kAPAL)
      {
        femtoAnalysis[anIter]->SetV0SharedDaughterCut(true);
      }
      
      // add Average Separation correlation function
      avgSepCF[anIter] = new AliFemtoAvgSepCorrFctn(Form("Avgsep%stpcM%iPsi6", sysNames[iSys], imult),5000,0,500);
      
      if(iSys == kLL || iSys == kALAL || iSys == kLAL)
        avgSepCF[anIter]->SetPairType(AliFemtoAvgSepCorrFctn::kV0s);
      else if(iSys == kPL || iSys == kAPL || iSys == kPAL || iSys == kAPAL)
        avgSepCF[anIter]->SetPairType(AliFemtoAvgSepCorrFctn::kTrackV0);
      else if(iSys ==kPP || iSys ==kPAP || iSys == kAPAP)
        avgSepCF[anIter]->SetPairType(AliFemtoAvgSepCorrFctn::kTracks);
      
      femtoAnalysis[anIter]->AddCorrFctn(avgSepCF[anIter]);
      
      // add femtoscopic correlation function (identical or non-identical masses)
      if(iSys==kPL || iSys==kAPL || iSys==kPAL || iSys==kAPAL)
      {
        // this is in k*
        nonIdenticalCF[anIter] = new AliFemtoCorrFctnNonIdDR(Form("CF_kstar_%sM%i", sysNames[iSys], imult),500,0.0,5.0);
        femtoAnalysis[anIter]->AddCorrFctn(nonIdenticalCF[anIter]);
      }
      else // pairs with the same mass of particles
      {
        // this is in q_inv
        identicalCF[anIter] = new AliFemtoQinvCorrFctn(Form("CF_qinv_%sM%i", sysNames[iSys], imult),500,0.0,10.0);
        femtoAnalysis[anIter]->AddCorrFctn(identicalCF[anIter]);
      }
      
      // add correlation function on model data
      if(mcAnalysis)
      {
        modelCF[anIter] = new AliFemtoModelCorrFctn(Form("CF_qinv_Model_%sM%i", sysNames[iSys],imult),500,0.0,10.0);
        modelCF[anIter]->ConnectToManager(modelMgr);
        femtoAnalysis[anIter]->AddCorrFctn(modelCF[anIter]);
        femtoAnalysis[anIter]->SetEnablePairMonitors(true);
      }
      
      Manager->AddAnalysis(femtoAnalysis[anIter]);
    }
    
  }
  return Manager;
}

AliFemtoEventReaderAODMultSelection* GetReader2015(bool mcAnalysis)
{
  AliFemtoEventReaderAODMultSelection* Reader = new AliFemtoEventReaderAODMultSelection();
  Reader->SetFilterMask(128);
  Reader->SetReadV0(1);
  Reader->SetUseMultiplicity(AliFemtoEventReaderAOD::kCentrality);
  Reader->SetEPVZERO(kTRUE);
  Reader->SetCentralityFlattening(kTRUE);
  if(mcAnalysis) Reader->SetReadMC(kTRUE);
  
  return Reader;
}

AliFemtoEventReaderAODChain* GetReader2011(bool mcAnalysis)
{
  AliFemtoEventReaderAODChain* Reader = new AliFemtoEventReaderAODChain();
  Reader->SetFilterBit(7);
  Reader->SetReadV0(1);
  Reader->SetUseMultiplicity(AliFemtoEventReaderAOD::kCentrality);
  Reader->SetEPVZERO(kTRUE);
  Reader->SetCentralityFlattening(kTRUE);
  if(mcAnalysis) Reader->SetReadMC(kTRUE);
  
  return Reader;
}

AliFemtoVertexMultAnalysis* GetAnalysis(double multMin, double multMax)
{
  AliFemtoVertexMultAnalysis *analysis = new AliFemtoVertexMultAnalysis(8, -8.0, 8.0, 4, multMin, multMax);
  analysis->SetNumEventsToMix(10);
  analysis->SetMinSizePartCollection(1);
  analysis->SetVerboseMode(kFALSE);
  
  return analysis;
}

AliFemtoBasicEventCut* GetEventCut()
{
  AliFemtoBasicEventCut *eventCut = new AliFemtoBasicEventCut();
  eventCut->SetEventMult(0,100000);
  eventCut->SetVertZPos(-8,8);
  eventCut->SetEPVZERO(-TMath::Pi()/2.,TMath::Pi()/2.);
  
  return eventCut;
}

AliFemtoV0TrackCut* GetV0TrackCut(EPart particle)
{
  if(particle != kELambda && particle != kEAntiLambda) return 0;
  double LambdaMass = 1.115683;
  
  AliFemtoV0TrackCut *particleCut = new AliFemtoV0TrackCut();
  particleCut->SetMass(LambdaMass);
  particleCut->SetEta(0.8);
  particleCut->SetPt(0.5, 5.0);
  particleCut->SetEtaDaughters(0.8);
  particleCut->SetTPCnclsDaughters(80);
  particleCut->SetNdofDaughters(4.0);
  particleCut->SetStatusDaughters(AliESDtrack::kTPCrefit);
  particleCut->SetOnFlyStatus(kFALSE);
  particleCut->SetMaxDcaV0Daughters(0.4);
  particleCut->SetMaxDcaV0(0.5);
  particleCut->SetMaxV0DecayLength(60.0);
  particleCut->SetMaxCosPointingAngle(0.9993);
  
  if(particle == kELambda)
  {
    particleCut->SetParticleType(0);
    particleCut->SetPtPosDaughter(0.5, 4.0);
    particleCut->SetPtNegDaughter(0.16, 4.0);
    particleCut->SetMinDaughtersToPrimVertex(0.1, 0.3);
    particleCut->SetInvariantMassLambda(LambdaMass-0.0038, LambdaMass+0.0043);
  }
  else if(particle == kEAntiLambda)
  {
    particleCut->SetParticleType(1);
    particleCut->SetPtPosDaughter(0.16, 4.0);
    particleCut->SetPtNegDaughter(0.3, 4.0);
    particleCut->SetMinDaughtersToPrimVertex(0.3, 0.1);
    particleCut->SetInvariantMassLambda(LambdaMass-0.0036, LambdaMass+0.0041);
  }
  return particleCut;
}

AliFemtoESDTrackCut* GetESDTrackCut(EPart particle)
{
  if(particle != kEProton && particle != kEAntiProton) return 0;
  
  AliFemtoESDTrackCut *particleCut = new AliFemtoESDTrackCut();
  
  particleCut->SetMostProbableProton();
  particleCut->SetMass(0.938272013);
  particleCut->SetEta(-0.8, 0.8);
  particleCut->SetStatus(AliESDtrack::kTPCin);
  particleCut->SetminTPCncls(80);
  particleCut->SetRemoveKinks(kTRUE);
  particleCut->SetLabel(kFALSE);
  particleCut->SetMaxTPCChiNdof(4.0);
  particleCut->SetMaxImpactXY(2.8);
  particleCut->SetMaxImpactZ(3.2);
  particleCut->SetNsigma(3.0);
  particleCut->SetNsigmaTPCTOF(kTRUE);
  particleCut->SetCharge(particle == kEProton ? 1.0 : -1.0);
  particleCut->SetPt(0.7, particle == kEProton ? 4.0 : 5.0);
  
  return particleCut;
}

AliFemtoV0PairCut* GetV0PairCut(ESys system)
{
  AliFemtoV0PairCut *pairCut = 0;
  
  if(system == kLL || system == kLAL || system == kALAL)
  {
    pairCut = new AliFemtoV0PairCut();
    pairCut->SetDataType(AliFemtoPairCut::kAOD);
    pairCut->SetTPCEntranceSepMinimum(0.00001);
    pairCut->SetTPCExitSepMinimum(-1.);
    if(separationCuts)
    {
      pairCut->SetMinAvgSeparation(0, 5.5); //proton-pion+
      pairCut->SetMinAvgSeparation(1, 5.5); //proton-antiproton
      pairCut->SetMinAvgSeparation(2, 0); //pion- - pion+
      pairCut->SetMinAvgSeparation(3, 5.5); //antiproton - pion-
    }
  }
  return pairCut;
}

AliFemtoV0TrackPairCut* GetV0TrackPairCut(ESys system)
{
  AliFemtoV0TrackPairCut *pairCut = 0;
  
  if(system == kPL || system == kAPAL)
  {
    pairCut = new AliFemtoV0TrackPairCut(); //lambda-proton
    pairCut->SetShareQualityMax(1.0); //between V0 daughter and track
    pairCut->SetShareFractionMax(0.05);
    pairCut->SetTPCOnly(kTRUE);
    pairCut->SetDataType(AliFemtoPairCut::kAOD);
    
    if(system == kPL)
    {
      pairCut->SetKstarCut(0.04,AliFemtoV0TrackPairCut::kLambda,AliFemtoV0TrackPairCut::kProton); //0 - lambda, 2 - proton
    }
    else if(system == kAPAL)
    {
      pairCut->SetKstarCut(0.04,AliFemtoV0TrackPairCut::kAntiLambda,AliFemtoV0TrackPairCut::kAntiProton); //1 - antilambda, 3 - antiproton
    }
    
    if(separationCuts)
    {
      pairCut->SetTPCEntranceSepMinimum(0.00001);
      pairCut->SetTPCExitSepMinimum(-1.);
      pairCut->SetMinAvgSeparation(0, 5); //0 - track-pos, 1 - track-neg
      pairCut->SetMinAvgSeparation(1, 5);
    }
  }
  else if(system == kAPL || system == kPAL)
  {
    pairCut = new AliFemtoV0TrackPairCut(); //lambda-antiproton, antilambda-proton
    pairCut->SetShareQualityMax(1.0); //between V0 daughter and track
    pairCut->SetShareFractionMax(0.05);
    pairCut->SetTPCOnly(kTRUE);
    pairCut->SetDataType(AliFemtoPairCut::kAOD);
    if(separationCuts)
    {
      pairCut->SetTPCEntranceSepMinimum(0.00001);
      pairCut->SetTPCExitSepMinimum(-1.);
      if(system == kAPL)
      {
        pairCut->SetMinAvgSeparation(0, 5);  // antiproton - proton
        pairCut->SetMinAvgSeparation(1, 8); // antiproton - pion-
      }
      else if(system == kPAL)
      {
        pairCut->SetMinAvgSeparation(0, 8); // proton - pion+
        pairCut->SetMinAvgSeparation(1, 5); // proton - antiproton-
      }
    }
  }
  return pairCut;
}

AliFemtoPairCutRadialDistance* GetTracksPairCut(ESys system)
{
  AliFemtoPairCutRadialDistance *pairCut = 0;
  
  if(system == kPP || system == kPAP || system == kAPAP)
  {
    pairCut = new AliFemtoPairCutRadialDistance();
    pairCut->SetPhiStarDifferenceMinimum(0.012);
    pairCut->SetEtaDifferenceMinimum(0.017);
    pairCut->SetShareQualityMax(1.0);
    pairCut->SetShareFractionMax(0.05);
    pairCut->SetRemoveSameLabel(kFALSE);
    pairCut->SetMaxEEMinv(0.002);
    pairCut->SetMaxThetaDiff(0.008);
    pairCut->SetDataType(AliFemtoPairCut::kAOD);
    if(separationCuts)
    {
      pairCut->SetTPCEntranceSepMinimum(0.00001);
      pairCut->SetAvgsepMinimum(5.0);
    }
    
  }
  return pairCut;
}

void GetParticlesForSystem(ESys system, EPart &firstParticle, EPart &secondParticle)
{
  if(system == kLL)   {firstParticle = kELambda;       secondParticle = kELambda;}
  if(system == kLAL)  {firstParticle = kELambda;       secondParticle = kEAntiLambda;}
  if(system == kALAL) {firstParticle = kEAntiLambda;   secondParticle = kEAntiLambda;}
  if(system == kPL)   {firstParticle = kELambda;       secondParticle = kEProton;}
  if(system == kAPL)  {firstParticle = kELambda;       secondParticle = kEAntiProton;}
  if(system == kPAL)  {firstParticle = kEAntiLambda;   secondParticle = kEProton;}
  if(system == kAPAL) {firstParticle = kEAntiLambda;   secondParticle = kEAntiProton;}
  if(system == kPP)   {firstParticle = kEProton;       secondParticle = kEProton;}
  if(system == kPAP)  {firstParticle = kEProton;       secondParticle = kEAntiProton;}
  if(system == kAPAP) {firstParticle = kEAntiProton;   secondParticle = kEAntiProton;}
}










