/*
  Package:    		QGDev/qgMiniTuple
  Class:     		qgMiniTuple
  Original Author:  	Tom Cornelis
 
  Description: 		Create small ntuple of QG-Likelihood variables and binning variables

*/

#include <memory>

#include "FWCore/Framework/interface/Frameworkfwd.h"
#include "FWCore/Framework/interface/EDAnalyzer.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/ServiceRegistry/interface/Service.h"
#include "CommonTools/UtilAlgos/interface/TFileService.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"

#include "DataFormats/VertexReco/interface/Vertex.h"
#include "DataFormats/VertexReco/interface/VertexFwd.h"
#include "DataFormats/HepMCCandidate/interface/GenParticle.h"
#include "DataFormats/ParticleFlowCandidate/interface/PFCandidate.h"
#include "DataFormats/JetReco/interface/PFJet.h"
#include "DataFormats/BTauReco/interface/JetTag.h"
#include "SimDataFormats/JetMatching/interface/JetFlavourInfo.h"
#include "SimDataFormats/JetMatching/interface/JetFlavourInfoMatching.h"
#include "DataFormats/Common/interface/ValueMap.h"
#include "DataFormats/Math/interface/deltaR.h"

#include "TFile.h"
#include "TTree.h"


class qgMiniTuple : public edm::EDAnalyzer{
   public:
      explicit qgMiniTuple(const edm::ParameterSet&);
      ~qgMiniTuple(){};
      static void fillDescriptions(edm::ConfigurationDescriptions& descriptions);

   private:
      virtual void beginJob() override;
      virtual void analyze(const edm::Event&, const edm::EventSetup&) override;
      bool jetId(const reco::PFJet *jet, bool tight = false, bool medium = false);
      template <class jetClass> void calcVariables(const jetClass *jet, float& axis2_, float& ptD_, int& mult_, edm::Handle<reco::VertexCollection> vC, bool qualityCuts = true);
      virtual void endJob() override {};

      edm::EDGetTokenT<double> rhoToken;
      edm::EDGetTokenT<reco::VertexCollection> vertexToken;
      edm::EDGetTokenT<reco::PFJetCollection> jetsToken;
      edm::EDGetTokenT<reco::JetFlavourInfoMatchingCollection> jetFlavourToken;
/*    edm::InputTag qgVariablesInputTag;
      edm::EDGetTokenT<edm::ValueMap<float>> qgToken, axis2Token, ptDToken;
      edm::EDGetTokenT<edm::ValueMap<int>> multToken;*/
      edm::EDGetTokenT<reco::JetTagCollection> bTagToken;
      const double minJetPt;

      edm::Service<TFileService> fs;
      TTree *tree;

      bool jetIdLoose, jetIdMedium, jetIdTight;
      float rho, pt, eta, axis2, axis2_NoQC, laxis2, laxis2_NoQC, ptD, ptD_NoQC, bTag;
      int nRun, nLumi, nEvent, mult, mult_NoQC, partonId;
};


qgMiniTuple::qgMiniTuple(const edm::ParameterSet& iConfig) :
  rhoToken( 		consumes<double>(					iConfig.getParameter<edm::InputTag>("rhoInputTag"))),
  vertexToken(    	consumes<reco::VertexCollection>(			iConfig.getParameter<edm::InputTag>("vertexInputTag"))),
  jetsToken(    	consumes<reco::PFJetCollection>(			iConfig.getParameter<edm::InputTag>("jetsInputTag"))),
  jetFlavourToken(	consumes<reco::JetFlavourInfoMatchingCollection>( 	iConfig.getParameter<edm::InputTag>("jetFlavourInputTag"))),
  bTagToken(		consumes<reco::JetTagCollection>(       		iConfig.getParameter<edm::InputTag>("csvInputTag"))),
//qgVariablesInputTag(  							iConfig.getParameter<edm::InputTag>("qgVariablesInputTag")),
  minJetPt(									iConfig.getUntrackedParameter<double>("minJetPt", 10.))
{
/*qgToken	= 	consumes<edm::ValueMap<float>>(		edm::InputTag(qgVariablesInputTag.label(), "qgLikelihood"));
  axis2Token	= 	consumes<edm::ValueMap<float>>(		edm::InputTag(qgVariablesInputTag.label(), "axis2Likelihood"));
  multToken	= 	consumes<edm::ValueMap<int>>(		edm::InputTag(qgVariablesInputTag.label(), "multLikelihood"));
  ptDToken	= 	consumes<edm::ValueMap<float>>(		edm::InputTag(qgVariablesInputTag.label(), "ptDLikelihood"));*/
}


void qgMiniTuple::analyze(const edm::Event& iEvent, const edm::EventSetup& iSetup){
  nRun 		= (int) iEvent.id().run();
  nLumi 	= (int) iEvent.id().luminosityBlock();
  nEvent	= (int) iEvent.id().event();

  edm::Handle<double> rho_;
  iEvent.getByToken(rhoToken, rho_);
  rho = (float) *rho_;

  edm::Handle<reco::PFJetCollection> jets;
  iEvent.getByToken(jetsToken, jets);

  edm::Handle<reco::JetFlavourInfoMatchingCollection> jetFlavours;
  iEvent.getByToken(jetFlavourToken, jetFlavours);

  edm::Handle<reco::VertexCollection> vertexCollection;
  iEvent.getByToken(vertexToken, vertexCollection);

  edm::Handle<reco::JetTagCollection> bTagHandle;
  iEvent.getByToken(bTagToken, bTagHandle);
/*
  edm::Handle<edm::ValueMap<float>> qgHandle, axis2Handle, ptDHandle;
  edm::Handle<edm::ValueMap<int>> multHandle;
  iEvent.getByToken(qgToken, qgHandle);
  iEvent.getByToken(axis2Token, axis2Handle);
  iEvent.getByToken(multToken, multHandle);
  iEvent.getByToken(ptDToken, ptDHandle);
*/
  for(auto jet = jets->begin();  jet != jets->end(); ++jet){
    if(jet->pt() < minJetPt) continue;
    edm::RefToBase<reco::Jet> jetRef(edm::Ref<reco::PFJetCollection>(jets, (jet - jets->begin())));

    partonId	= (*jetFlavours)[jetRef].getPartonFlavour();
    if(partonId == 0) continue;

    pt		= jet->pt();
    eta		= jet->eta();
/*  qg		= (*qgHandle)[jetRef];
    axis2	= (*axis2Handle)[jetRef];
    mult	= (*multHandle)[jetRef];
    ptD		= (*ptDHandle)[jetRef];*/
    bTag	= (*bTagHandle)[jetRef];

    calcVariables(&*jet, axis2, ptD, mult, vertexCollection);
    calcVariables(&*jet, axis2_NoQC, ptD_NoQC, mult_NoQC, vertexCollection, false);
    laxis2 	= -std::log(axis2);
    laxis2_NoQC = -std::log(axis2_NoQC);

    jetIdLoose	= jetId(&*jet); 
    jetIdMedium	= jetId(&*jet, false, true); 
    jetIdTight  = jetId(&*jet, true); 
    tree->Fill();
  }
}

void qgMiniTuple::beginJob(){
  tree = fs->make<TTree>("qgMiniTuple","qgMiniTuple");
  tree->Branch("nRun" ,		&nRun, 		"nRun/I");
  tree->Branch("nLumi" ,	&nLumi, 	"nLumi/I");
  tree->Branch("nEvent" ,	&nEvent, 	"nEvent/I");
  tree->Branch("rho" ,		&rho, 		"rho/F");
  tree->Branch("pt" ,		&pt,		"pt/F");
  tree->Branch("eta",		&eta,		"eta/F");
  tree->Branch("axis2",		&axis2,		"axis2/F");
  tree->Branch("laxis2",	&laxis2,	"laxis2/F");
  tree->Branch("ptD",		&ptD,		"ptD/F");
  tree->Branch("mult",		&mult,		"mult/I");
  tree->Branch("axis2_NoQC",	&axis2_NoQC,	"axis2_NoQC/F");
  tree->Branch("laxis2_NoQC",	&laxis2_NoQC,	"laxis2_NoQC/F");
  tree->Branch("ptD_NoQC",	&ptD_NoQC,	"ptD_NoQC/F");
  tree->Branch("mult_NoQC",	&mult_NoQC,	"mult_NoQC/I");
  tree->Branch("bTag",		&bTag,		"bTag/F");
  tree->Branch("partonId",	&partonId,	"partonId/I");
  tree->Branch("jetIdLoose",	&jetIdLoose,	"jetIdLoose/O");
  tree->Branch("jetIdMedium",	&jetIdMedium,	"jetIdMedium/O");
  tree->Branch("jetIdTight",	&jetIdTight,	"jetIdTight/O");
}


/// Calculation of axis2, mult and ptD
template <class jetClass> void qgMiniTuple::calcVariables(const jetClass *jet, float& axis2_, float& ptD_, int& mult_, edm::Handle<reco::VertexCollection> vC, bool qualityCuts){
  auto vtxLead = vC->begin();

  float sum_weight = 0., sum_deta = 0., sum_dphi = 0., sum_deta2 = 0., sum_dphi2 = 0., sum_detadphi = 0., sum_pt = 0.;
  int nChg_QC = 0, nNeutral_ptCut = 0;

  //Loop over the jet constituents
  for(auto part : jet->getPFConstituents()){
    if(!part.isNonnull()) continue;

    reco::TrackRef itrk = part->trackRef();
    if(itrk.isNonnull()){
      auto vtxClose = vC->begin();
      for(auto vtx = vC->begin(); vtx != vC->end(); ++vtx){
        if(fabs(itrk->dz(vtx->position())) < fabs(itrk->dz(vtxClose->position()))) vtxClose = vtx;
      }

      if(vtxClose == vtxLead){
        if(qualityCuts){
          float dz = itrk->dz(vtxClose->position());
          float dz_sigma = sqrt(pow(itrk->dzError(),2) + pow(vtxClose->zError(),2));

          if(itrk->quality(reco::TrackBase::qualityByName("highPurity")) && fabs(dz/dz_sigma) < 5.){
            float d0 = itrk->dxy(vtxClose->position());
            float d0_sigma = sqrt(pow(itrk->d0Error(),2) + pow(vtxClose->xError(),2) + pow(vtxClose->yError(),2));
            if(fabs(d0/d0_sigma) < 5.) nChg_QC++;
          } else continue;
        } else if(itrk->quality(reco::TrackBase::qualityByName("highPurity"))) nChg_QC++;
        else continue;
      } else continue;

    } else {
      if(part->pt() > 1.0) nNeutral_ptCut++;
    }

    float deta = part->eta() - jet->eta();
    float dphi = reco::deltaPhi(part->phi(), jet->phi());
    float partPt = part->pt();
    float weight = partPt*partPt;

    sum_weight += weight;
    sum_pt += partPt;
    sum_deta += deta*weight;
    sum_dphi += dphi*weight;
    sum_deta2 += deta*deta*weight;
    sum_detadphi += deta*dphi*weight;
    sum_dphi2 += dphi*dphi*weight;
  }

  //Calculate axis2 and ptD
  float a = 0., b = 0., c = 0.;
  float ave_deta = 0., ave_dphi = 0., ave_deta2 = 0., ave_dphi2 = 0.;
  if(sum_weight > 0){
    ptD_ = sqrt(sum_weight)/sum_pt;
    ave_deta = sum_deta/sum_weight;
    ave_dphi = sum_dphi/sum_weight;
    ave_deta2 = sum_deta2/sum_weight;
    ave_dphi2 = sum_dphi2/sum_weight;
    a = ave_deta2 - ave_deta*ave_deta;
    b = ave_dphi2 - ave_dphi*ave_dphi;
    c = -(sum_detadphi/sum_weight - ave_deta*ave_dphi);
  } else ptD_ = 0;
  float delta = sqrt(fabs((a-b)*(a-b)+4*c*c));
  if(a+b-delta > 0) axis2_ = sqrt(0.5*(a+b-delta));
  else axis2_ = 0.;

  mult_ = (nChg_QC + nNeutral_ptCut);
}



bool qgMiniTuple::jetId(const reco::PFJet *jet, bool tight, bool medium){
  float jetEnergyUncorrected 		= jet->chargedHadronEnergy() + jet->neutralHadronEnergy() + jet->photonEnergy() +
  					  jet->electronEnergy() + jet->muonEnergy() + jet->HFHadronEnergy() + jet->HFEMEnergy();
  float neutralHadronEnergyFraction 	= (jet->neutralHadronEnergy() + jet->HFHadronEnergy())/jetEnergyUncorrected;
  float neutralEmEnergyFraction 	= (jet->neutralEmEnergy())/jetEnergyUncorrected;
  float chargedHadronEnergyFraction 	= (jet->chargedHadronEnergy())/jetEnergyUncorrected;
  float chargedEmEnergyFraction 	= (jet->chargedEmEnergy())/jetEnergyUncorrected;

  if(!(neutralHadronEnergyFraction 	< (tight ? 0.90 : (medium ? 0.95 : .99)))) 	return false;
  if(!(neutralEmEnergyFraction 		< (tight ? 0.90 : (medium ? 0.95 : .99)))) 	return false;
  if(!((jet->chargedMultiplicity() + jet->neutralMultiplicity()) > 1)) 			return false;
  if(fabs(jet->eta()) < 2.4){
    if(!(chargedHadronEnergyFraction > 0)) 						return false;
    if(!(chargedEmEnergyFraction < .99)) 						return false;
    if(!(jet->chargedMultiplicity() > 0)) 						return false;
  }
  return true;
}


void qgMiniTuple::fillDescriptions(edm::ConfigurationDescriptions& descriptions){
  edm::ParameterSetDescription desc;
  desc.addUntracked<std::string>("fileName","qgMiniTuple.root");
  desc.add<edm::InputTag>("rhoInputTag");
  desc.add<edm::InputTag>("vertexInputTag");
  desc.add<edm::InputTag>("jetsInputTag");
  desc.add<edm::InputTag>("jetFlavourInputTag");
//desc.add<edm::InputTag>("qgVariablesInputTag");
  desc.addUntracked<double>("minJetPt", 10.);
  desc.setUnknown();
  descriptions.addDefault(desc);
}


DEFINE_FWK_MODULE(qgMiniTuple);
